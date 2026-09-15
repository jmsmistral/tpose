/* tpose_io.c -- tpose input/output implementation. 

   Copyright 2015 Jonathan Sacramento.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
*/

#include "tpose_io.h"
#include <limits.h>

unsigned char rowDelimiter = '\n';
BTree* btreeGlobal;
TposeThreadData** threadDataArray;
TposeThreadData* threadData;
TposeThreadAggregator** threadAggregatorArray;
TposeThreadAggregator* threadAggregator;
unsigned int fileChunks;
off_t partitions[1000];
TposeOutputFile* tempFileArray[1000];

/* Check the content length before copying, reserving the terminating zero. */
static void tposeIOCheckFieldWidth(size_t length, const char* field) {
	if(length >= TPOSE_IO_MAX_FIELD_WIDTH) {
		fprintf(stderr, "Error: %s exceeds the maximum field width of %u bytes\n",
		        field, (unsigned int) TPOSE_IO_MAX_FIELD_WIDTH - 1);
		exit(EXIT_FAILURE);
	}
}

/* Called only for new groups: duplicates do not consume another slot. */
static void tposeIOCheckGroupCapacity(off_t count, const TposeHeader* header) {
	if(count >= header->maxFields) {
		fprintf(stderr, "Error: Input exceeds the maximum of %u distinct groups\n",
		        header->maxFields);
		exit(EXIT_FAILURE);
	}
}

/* Copy the terminating zero as well as the field contents. */
static char* tposeIODuplicateString(const char* value) {
	char* copy = strdup(value);
	if(copy == NULL) {
		fprintf(stderr, "Error: Cannot allocate field string memory\n");
		exit(EXIT_FAILURE);
	}
	return copy;
}


/* All ranges are half-open: end is never dereferenced. A final record does
   not need a newline, and a trailing newline does not create another record. */
typedef struct {
    const char* begin;
    const char* end;
} TposeRecord;

static int tposeIONextRecord(const char** cursor, const char* end, TposeRecord* record) {
    if(*cursor == end) return 0;
    record->begin = *cursor;
    const char* newline = memchr(*cursor, rowDelimiter, (size_t) (end - *cursor));
    record->end = newline ? newline : end;
    *cursor = newline ? newline + 1 : end;
    return 1;
}

static void tposeIOCopyField(char* destination, const char* begin,
                             const char* end, const char* label) {
    size_t length = (size_t) (end - begin);
    tposeIOCheckFieldWidth(length, label);
    memcpy(destination, begin, length);
    destination[length] = '\0';
}

/* Locate one field without reading past the record, including empty fields. */
static void tposeIOReadField(TposeRecord record, unsigned char delimiter,
                             int index, char* destination, const char* label) {
    destination[0] = '\0';
    if(index < 0) return;
    const char* cursor = record.begin;
    for(int field = 0; ; ++field) {
        const char* separator = memchr(cursor, delimiter, (size_t) (record.end - cursor));
        const char* end = separator ? separator : record.end;
        if(field == index) {
            tposeIOCopyField(destination, cursor, end, label);
            return;
        }
        if(!separator) return;
        cursor = separator + 1;
    }
}

typedef struct {
    char id[TPOSE_IO_MAX_FIELD_WIDTH];
    char group[TPOSE_IO_MAX_FIELD_WIDTH];
    char numeric[TPOSE_IO_MAX_FIELD_WIDTH];
} TposeValues;

static void tposeIOReadValues(TposeRecord record, TposeQuery* query, TposeValues* values) {
    values->id[0] = values->group[0] = values->numeric[0] = '\0';
    const char* cursor = record.begin;
    for(size_t field = 0; ; ++field) {
        const char* separator = memchr(cursor, query->inputFile->fieldDelimiter,
                                       (size_t) (record.end - cursor));
        const char* end = separator ? separator : record.end;
        if(query->id >= 0 && field == (size_t) query->id)
            tposeIOCopyField(values->id, cursor, end, "ID field");
        if(query->group >= 0 && field == (size_t) query->group)
            tposeIOCopyField(values->group, cursor, end, "Group field");
        if(query->numeric >= 0 && field == (size_t) query->numeric)
            tposeIOCopyField(values->numeric, cursor, end, "Numeric field");
        if(!separator) break;
        cursor = separator + 1;
    }
}

static off_t tposeIODataSize(const TposeInputFile* input) {
    return input->fileSize - (input->dataAddr - input->fileAddr);
}

static TposeRecord tposeIOPartitionRange(TposeQuery* query, unsigned int threadId) {
    off_t start = partitions[threadId];
    off_t end = partitions[threadId + 1];
    if(start < 0 || end < start || end > tposeIODataSize(query->inputFile)) {
        fprintf(stderr, "Error: Partition is outside the input data\n");
        exit(EXIT_FAILURE);
    }
    TposeRecord range = {query->inputFile->dataAddr + start, query->inputFile->dataAddr + end};
    return range;
}

static void tposeIODiscoverGroups(TposeQuery* query, BTree* tree, TposeHeader* header,
                                  const char* cursor, const char* end) {
    TposeRecord record;
    char group[TPOSE_IO_MAX_FIELD_WIDTH];
    while(tposeIONextRecord(&cursor, end, &record)) {
        tposeIOReadField(record, query->inputFile->fieldDelimiter, query->group, group, "Group field");
        if(!group[0] || btreeSearch(tree, tree->root, group)) continue;
        tposeIOCheckGroupCapacity(header->numFields, header);
        char* name = tposeIODuplicateString(group);
        BTreeKey key = {0};
        btreeSetKeyValue(&key, name, header->numFields, 0);
        if(btreeInsert(tree, &key) == -1) {
            fprintf(stderr, "Error: Cannot insert value into btree\n");
            exit(EXIT_FAILURE);
        }
        header->fields[header->numFields++] = name;
    }
}

static void tposeIORequireGroups(TposeHeader* header) {
    if(!header->numFields) {
        fprintf(stderr, "Error: Input contains no nonempty group values\n");
        exit(EXIT_FAILURE);
    }
}

static void tposeIOCalculateAverages(TposeAggregator* aggregator) {
    for(unsigned int i = 0; i < aggregator->numFields; ++i)
        aggregator->avgs[i] = aggregator->aggregates[i] / aggregator->counts[i];
}

static void tposeIOFinishId(TposeQuery* query, TposeAggregator* aggregator,
                           char* id, int threadId) {
    tposeIOCalculateAverages(aggregator);
    if(threadId < 0) tposeIOPrintGroupIdData(id, query);
    else tposeIOPrintGroupIdDataParallel(id, query, aggregator, (unsigned int) threadId);
}

/* Serial and parallel paths commit each bounded record exactly once. */
static void tposeIOAggregateRange(TposeQuery* query, BTree* tree, TposeAggregator* aggregator,
                                  const char* cursor, const char* end, int threadId) {
    TposeRecord record;
    TposeValues values;
    char currentId[TPOSE_IO_MAX_FIELD_WIDTH] = "";
    int haveId = 0;
    while(tposeIONextRecord(&cursor, end, &record)) {
        tposeIOReadValues(record, query, &values);
        if(!values.group[0] || !values.numeric[0] || (query->id >= 0 && !values.id[0])) continue;
        BTreeKey* key = btreeSearch(tree, tree->root, values.group);
        if(!key) continue;
        if(query->id >= 0) {
            if(haveId && strcmp(currentId, values.id)) {
                tposeIOFinishId(query, aggregator, currentId, threadId);
                memset(aggregator->aggregates, 0, aggregator->numFields * sizeof(double));
                memset(aggregator->counts, 0, aggregator->numFields * sizeof(double));
            }
            strcpy(currentId, values.id); /* same-sized, checked field buffers */
            haveId = 1;
        }
        aggregator->aggregates[key->dataOffset] += atof(values.numeric);
        aggregator->counts[key->dataOffset]++;
    }
    if(haveId) tposeIOFinishId(query, aggregator, currentId, threadId);
}

	

/** 
 ** Return lower-case string (modifies string)
 **/
char* tposeIOLowerCase(
	char* string
) {
	
	if(string == NULL)
		return NULL;

	int i;
	for(i=0; (*(string+i) = tolower(*(string+i))) != '\0'; ++i);
	
	return string;

}



/** 
 ** Return the integer index of the field parameter 
 ** returns -1 if field passed doesn't match any of the header fields
 **/
int tposeIOGetFieldIndex(
	TposeHeader* tposeHeader
   ,char* field
) {

	if(field == NULL)
		return -1;
	
	int numFields;
	if(!tposeHeader->numFields)
		numFields = tposeHeader->maxFields;
	else
		numFields = tposeHeader->numFields; // To limit iterating more than needed


	int foundFlag = 0;
	int i;
	for(i = 0; i < numFields; i++) {
		if(!strcmp(field, *(tposeHeader->fields+i))) {
			foundFlag = 1;
			break;
		}
	}

	if(!foundFlag && (i==numFields))
		return -1; // Not found


	debug_print("tposeIOGetFieldIndex(): found flag =  %d\n", foundFlag);
	debug_print("tposeIOGetFieldIndex(): search field =  %s\n", field);
	debug_print("tposeIOGetFieldIndex(): numFields = %d\n", numFields);
	debug_print("tposeIOGetFieldIndex(): broken out @ i = %d\n", i);

	return i; 
	
}



/** 
 ** Allocates memory for the input file
 **/
TposeInputFile* tposeIOInputFileAlloc(
	int fd
	,char* fileAddr
	,off_t fileSize
	,unsigned char fieldDelimiter
) {

	TposeInputFile* inputFile;

	// Allocate memory for the field names
	if((inputFile = (TposeInputFile*) malloc(sizeof(TposeInputFile))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate input file memory\n");
		return NULL;
	}

	inputFile->fd = fd;
	inputFile->fileAddr = fileAddr;
	inputFile->fileSize = fileSize;
	inputFile->fieldDelimiter = fieldDelimiter;
	inputFile->fileHeader = NULL;
	
	assert(inputFile->fd > 0);
	assert(inputFile->fileAddr != NULL);
	assert(inputFile->fileSize != 0);
	
	return inputFile;
	
}



/** 
 ** Free memory for a TposeInputFile 
 **/
void tposeIOInputFileFree(
    TposeInputFile** inputFilePtr
) {

	tposeIOHeaderFree(&((*inputFilePtr)->fileHeader));
    
   if(*inputFilePtr != NULL) {
       free(*inputFilePtr);
       *inputFilePtr = NULL;
   }

   assert(*inputFilePtr == NULL);

}



/** 
 ** Allocates memory for the output file
 **/
TposeOutputFile* tposeIOOutputFileAlloc(
	FILE* fd
	,unsigned char fieldDelimiter
) {

	TposeOutputFile* outputFile;

	// Allocate memory for the field names
	if((outputFile = (TposeOutputFile*) malloc(sizeof(TposeOutputFile))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate output file memory\n");
		return NULL;
	}

	outputFile->fd = fd;
	outputFile->fieldDelimiter = fieldDelimiter;
	outputFile->fileIdHeader = NULL;
	outputFile->fileGroupHeader = NULL;
	outputFile->transaction = NULL;
	
	assert(outputFile->fd != NULL);

	return outputFile;
	
}



/** 
 ** Free memory for a TposeOutputFile 
 **/
void tposeIOOutputFileFree(
    TposeOutputFile** outputFilePtr
) {
	tposeIODiscardOutput(*outputFilePtr);

	if( (*outputFilePtr)->fileIdHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileIdHeader));

	if( (*outputFilePtr)->fileGroupHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileGroupHeader));
    
   if(*outputFilePtr != NULL) {
       free(*outputFilePtr);
       *outputFilePtr = NULL;
   }

   assert(*outputFilePtr == NULL);

}



/** 
 ** Allocates memory for the input file headers arrays 
 **/
TposeHeader* tposeIOHeaderAlloc(
	unsigned int maxFields
	,unsigned int mutateHeader
) {
	
	TposeHeader* tposeHeader;

	// Allocate memory for the field names
	if((tposeHeader = (TposeHeader*) malloc(sizeof(TposeHeader))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate header memory\n");
		return NULL;
	}

	if(mutateHeader) {
		// Unused slots must be NULL because cleanup visits the full capacity.
		if((tposeHeader->fields = calloc(maxFields, sizeof(char*))) == NULL ) {
			fprintf(stderr, "Error: Cannot allocate header fields memory\n");
			free(tposeHeader);
			return NULL;
		}
	}
	else {
		tposeHeader->fields = NULL;
	}

	tposeHeader->maxFields = maxFields;
	tposeHeader->numFields = 0;

	if(tposeHeader->maxFields == 0) {
		fprintf(stderr, "Error: Possibly wrong delimiter specified!\n");
		exit(EXIT_FAILURE);
	}
	
	assert(tposeHeader->maxFields != 0);
	assert(tposeHeader->numFields == 0);

	return tposeHeader;
	
}



/** 
 ** Free memory for a TposeHeader 
 **/
void tposeIOHeaderFree(
    TposeHeader** tposeHeaderPtr
) {

	// Free all unique groups first
	int field;
	if((*tposeHeaderPtr)->fields != NULL) {
		for(field = 0; field < (*tposeHeaderPtr)->maxFields; field++) {
			free((*tposeHeaderPtr)->fields[field]);
		} 

		free((*tposeHeaderPtr)->fields);
		(*tposeHeaderPtr)->fields = NULL;
	}
    
	assert((*tposeHeaderPtr)->fields == NULL);

	if(*tposeHeaderPtr != NULL) {
		free(*tposeHeaderPtr);
		*tposeHeaderPtr = NULL;
	}

	assert(*tposeHeaderPtr == NULL);

}



/** 
 ** Aggregator for each group/id 
 **/
TposeAggregator* tposeIOAggregatorAlloc(unsigned int numFields)
{

	TposeAggregator* tposeAggregator;

	// Allocate memory for the floating-point numeric variables being transposed
	if((tposeAggregator = (TposeAggregator*) calloc(1, sizeof(TposeAggregator))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		return NULL;
	}

	if((tposeAggregator->aggregates = (double*) calloc(numFields, sizeof(double))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		return NULL;
	}

	if((tposeAggregator->counts = (double*) calloc(numFields, sizeof(double))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		return NULL;
	}

	if((tposeAggregator->avgs = (double*) calloc(numFields, sizeof(double))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		return NULL;
	}

	tposeAggregator->numFields = numFields;
	
	assert(tposeAggregator->aggregates != NULL);
	assert(tposeAggregator->counts != NULL);
	assert(tposeAggregator->avgs != NULL);
	assert(tposeAggregator->numFields != 0);

	return tposeAggregator;
	
}



/** 
 ** Free memory for a TposeAggregator
 **/
void tposeIOAggregatorFree(
    TposeAggregator** tposeAggregatorPtr
) {

	int value;
	if((*tposeAggregatorPtr)->aggregates != NULL) {
		free((*tposeAggregatorPtr)->aggregates);
		(*tposeAggregatorPtr)->aggregates = NULL;
	}
	if((*tposeAggregatorPtr)->counts != NULL) {
		free((*tposeAggregatorPtr)->counts);
		(*tposeAggregatorPtr)->counts = NULL;
	}
	if((*tposeAggregatorPtr)->avgs != NULL) {
		free((*tposeAggregatorPtr)->avgs);
		(*tposeAggregatorPtr)->avgs = NULL;
	}
    
    assert((*tposeAggregatorPtr)->aggregates == NULL);
    assert((*tposeAggregatorPtr)->counts == NULL);
    assert((*tposeAggregatorPtr)->avgs == NULL);

    if(*tposeAggregatorPtr != NULL) {
        free(*tposeAggregatorPtr);
        *tposeAggregatorPtr = NULL;
    }

	assert(*tposeAggregatorPtr == NULL);

}



/** 
 ** Allocates memory for the transpose parameters
 ** Note: Matches field names
 **/
TposeQuery* tposeIOQueryAlloc(
	TposeInputFile* inputFile
	,TposeOutputFile* outputFile
	,char* idVar
	,char* groupVar
	,char* numericVar
	,char* aggregateType
) {

	// Check parameters passed
	if((idVar != NULL) && (tposeIOGetFieldIndex(inputFile->fileHeader, idVar) == -1)) return NULL;
	if((groupVar != NULL) && (tposeIOGetFieldIndex(inputFile->fileHeader, groupVar) == -1)) return NULL;
	if((numericVar != NULL) && (tposeIOGetFieldIndex(inputFile->fileHeader, numericVar) == -1)) return NULL;

	TposeQuery* tposeQuery;

	// Allocate memory for the query parameters
	if((tposeQuery = (TposeQuery*) malloc(sizeof(TposeQuery))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate memory for query\n");
		exit(EXIT_FAILURE);
	}
	
	tposeQuery->inputFile = inputFile;
	tposeQuery->outputFile = outputFile;
	tposeQuery->aggregator = NULL;
	tposeQuery->id = -1;
	tposeQuery->group = -1;
	tposeQuery->numeric = -1;
	tposeQuery->aggregateType = 0;
	if(idVar != NULL) tposeQuery->id = tposeIOGetFieldIndex(inputFile->fileHeader, idVar);
	if(groupVar != NULL) tposeQuery->group = tposeIOGetFieldIndex(inputFile->fileHeader, groupVar);
	if(numericVar != NULL) tposeQuery->numeric = tposeIOGetFieldIndex(inputFile->fileHeader, numericVar);
	if(aggregateType != NULL) {
		if(!strcmp("sum", tposeIOLowerCase(aggregateType)))
			tposeQuery->aggregateType = 0; //TPOSE_IO_AGGREGATION_SUM;

		if(!strcmp("count", tposeIOLowerCase(aggregateType)))
			tposeQuery->aggregateType = TPOSE_IO_AGGREGATION_COUNT;

		if(!strcmp("avg", tposeIOLowerCase(aggregateType)))
			tposeQuery->aggregateType = TPOSE_IO_AGGREGATION_AVG;
	}

	debug_print("tposeIOQueryAlloc(): id = %d\n", tposeQuery->id);
	debug_print("tposeIOQueryAlloc(): group = %d\n", tposeQuery->group);
	debug_print("tposeIOQueryAlloc(): numeric = %d\n", tposeQuery->numeric);
	assert(tposeQuery->aggregateType >= TPOSE_IO_AGGREGATION_SUM && tposeQuery->aggregateType <= TPOSE_IO_AGGREGATION_AVG);

	return tposeQuery;
	
}



/** 
 ** Allocates memory for the transpose parameters
 ** Note: Matches field indexes
 **/
TposeQuery* tposeIOQueryIndexedAlloc(
	TposeInputFile* inputFile
	,TposeOutputFile* outputFile
	,int idVar
	,int groupVar
	,int numericVar
	,char* aggregateType
) {

	// Correct field indexes so they're zero-based
	if(idVar != -1) --idVar;
	if(groupVar != -1) --groupVar;
	if(numericVar != -1) --numericVar;

	int minIndex = 0;
	int maxIndex;
	if(!((inputFile->fileHeader)->numFields))
		maxIndex = (inputFile->fileHeader)->maxFields - 1;
	else
		maxIndex = (inputFile->fileHeader)->numFields - 1;

	debug_print("tposeIOQueryIndexedAlloc(): idArg = %d\n", idVar);
	debug_print("tposeIOQueryIndexedAlloc(): groupArg = %d\n", idVar);
	debug_print("tposeIOQueryIndexedAlloc(): numericArg = %d\n", idVar);

	// Check parameters passed
	if( (idVar != -1) && ((idVar < minIndex) || (idVar > maxIndex) )) return NULL;
	if( (groupVar != -1) && ((groupVar < minIndex) || (groupVar > maxIndex) )) return NULL;
	if( (numericVar != -1) && ((numericVar < minIndex) || (numericVar > maxIndex) )) return NULL;

	// Allocate memory for the query parameters
	TposeQuery* tposeQuery;
	if((tposeQuery = (TposeQuery*) malloc(sizeof(TposeQuery))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate memory for query\n");
		exit(EXIT_FAILURE);
	}
	
	tposeQuery->inputFile = inputFile;
	tposeQuery->outputFile = outputFile;
	tposeQuery->aggregator = NULL;
	tposeQuery->id = idVar;
	tposeQuery->group = groupVar;
	tposeQuery->numeric = numericVar;
	tposeQuery->aggregateType = TPOSE_IO_AGGREGATION_SUM;
	if(aggregateType != NULL) {
		if(!strcmp("sum", tposeIOLowerCase(aggregateType)))
			tposeQuery->aggregateType = TPOSE_IO_AGGREGATION_SUM;

		if(!strcmp("count", tposeIOLowerCase(aggregateType)))
			tposeQuery->aggregateType = TPOSE_IO_AGGREGATION_COUNT;

		if(!strcmp("avg", tposeIOLowerCase(aggregateType)))
			tposeQuery->aggregateType = TPOSE_IO_AGGREGATION_AVG;
	}
	assert(tposeQuery->aggregateType >= TPOSE_IO_AGGREGATION_SUM && tposeQuery->aggregateType <= TPOSE_IO_AGGREGATION_AVG);

	return tposeQuery;
	
}



/** 
 ** Free memory for a TposeQuery 
 **/
void tposeIOQueryFree(
    TposeQuery** tposeQueryPtr
) {

	if((*tposeQueryPtr)->aggregator != NULL) tposeIOAggregatorFree( &((*tposeQueryPtr)->aggregator) );
    
	// No need to free the inputFile/outputFile,
	// as this is done in the tposeIOCloseFile() call
   if(*tposeQueryPtr != NULL) { 
       free(*tposeQueryPtr);
       *tposeQueryPtr = NULL;
   }

   assert(*tposeQueryPtr == NULL);

}



/** 
 ** Open input file
 **/
TposeInputFile* tposeIOOpenInputFile(
	char* filePath
	,unsigned char fieldDelimiter
	,unsigned int mutateHeader
) {

	int fd;
	char* fileAddr;
	off_t fileSize;
	struct stat statBuffer;

	if((fd = open(filePath, O_RDONLY)) < 0) {
		fprintf(stderr, "Error: Can not open input file %s\n", filePath);
		return NULL;
	}

	if(fstat(fd, &statBuffer) < 0) {
		fprintf(stderr, "Error: Can not stat input file %s\n", filePath);
		return NULL;
	}

	if((fileSize = statBuffer.st_size) == 0) {
		fprintf(stderr, "Error: No data found in input file %s\n", filePath);
		return NULL;
	}
	

	if((fileAddr = mmap(0, statBuffer.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED ) {
		fprintf(stderr, "Error: Can not map input file %s\n", filePath);
		return NULL;
	}

	// Might speed-up via aggressive read-ahead caching
	if(madvise(fileAddr, fileSize, MADV_SEQUENTIAL) == -1) {
		fprintf(stderr, "Warning: Cannot advise kernel on file %s\n", filePath);
	}

	TposeInputFile* inputFile = tposeIOInputFileAlloc(fd, fileAddr, fileSize, fieldDelimiter); // Creates the file handle 
	inputFile->fileHeader = tposeIOReadInputHeader(inputFile, mutateHeader); // Opening a file also creates the TposeHeader struct

	return inputFile;

}



/** 
 ** Close an input file and unmap any memory
 **/
int tposeIOCloseInputFile(
	TposeInputFile* inputFile
) {

   if((munmap(inputFile->fileAddr, inputFile->fileSize)) < 0) {
       fprintf(stderr, "Error: can not unmap input file\n");
       return -1;
   }

   if(close(inputFile->fd) < 0) {
       fprintf(stderr, "Error: can not close input file\n");
       return -1;
   }

	tposeIOInputFileFree(&inputFile);

	return 0;

}



/** 
 ** Open output file
 **/
TposeOutputFile* tposeIOOpenOutputFile(
	char* filePath
	,const char* mode
	,unsigned char fieldDelimiter
) {

	FILE* fd;
	if(strcmp(filePath, "stdout")) {
		if((fd = fopen(filePath, mode)) == NULL) {
			fprintf(stderr, "Error: Can not open output file %s\n", filePath);
			return NULL;
		}
	}
	else
		fd = stdout; // If no output file is passed, print to std output

	return tposeIOOutputFileAlloc(fd, fieldDelimiter); // Creates the file handle 

}



/** 
 ** Close an output file
 **/
int tposeIOCloseOutputFile(
	TposeOutputFile* outputFile
) {
	
	int failed = 0;
	if(outputFile->fd) {
		failed = ferror(outputFile->fd) != 0;
		if(fflush(outputFile->fd) != 0) failed = 1;
		if(outputFile->fd != stdout && fclose(outputFile->fd) != 0) failed = 1;
	}
	outputFile->fd = NULL;
	if(failed) fprintf(stderr, "Error: Cannot flush or close output stream\n");

	// Free TposeOutputFile memory
	tposeIOOutputFileFree(&outputFile);

	return failed ? -1 : 0;

}

void tposeIOFlushOutput(FILE* stream) {
    int failed = ferror(stream) != 0;
    if(fflush(stream) != 0) failed = 1;
    if(failed) {
        fprintf(stderr, "Error: Cannot write output stream\n");
        exit(EXIT_FAILURE);
    }
}



/** 
 ** Reads the first line (header) of an input file
 ** tpose only accepts data files with column headers on first line
 **/
TposeHeader* tposeIOReadInputHeader(TposeInputFile* inputFile, unsigned int mutateHeader) {
    if(!inputFile) return NULL;
    const char* cursor = inputFile->fileAddr;
    const char* end = cursor + inputFile->fileSize;
    TposeRecord record;
    if(!tposeIONextRecord(&cursor, end, &record)) return NULL;
    inputFile->dataAddr = (char*) cursor;
    unsigned int fieldCount = 1;
    for(const char* p = record.begin; p < record.end; ++p) {
        if(*p == inputFile->fieldDelimiter) {
            if(fieldCount == INT_MAX) {
                fprintf(stderr, "Error: Too many input columns\n");
                exit(EXIT_FAILURE);
            }
            ++fieldCount;
        }
    }
    TposeHeader* header = tposeIOHeaderAlloc(fieldCount, mutateHeader);
    if(!header) exit(EXIT_FAILURE);
    if(mutateHeader) {
        if(cursor == end) {
            fprintf(stderr, "Error: Input contains a header but no data rows\n");
            exit(EXIT_FAILURE);
        }
        char* row = strndup(record.begin, (size_t) (record.end - record.begin));
        if(!row) {
            fprintf(stderr, "Error: Cannot allocate header string memory\n");
            exit(EXIT_FAILURE);
        }
        char delimiters[] = {(char) inputFile->fieldDelimiter, '\0'};
        char* save;
        char* field = strtok_r(row, delimiters, &save);
        if(!field) {
            fprintf(stderr, "Error: Input header is empty\n");
            exit(EXIT_FAILURE);
        }
        /* Empty header-name semantics remain a separate validation issue. */
        while(field) {
            header->fields[header->numFields++] = tposeIOLowerCase(tposeIODuplicateString(field));
            field = strtok_r(NULL, delimiters, &save);
        }
        free(row);
    }
    return header;
}




/** 
 ** Returns a unique list of GROUP variable values 
 **/
void tposeIOUniqueGroups(TposeQuery* query, BTree* tree) {
    TposeHeader* header = tposeIOHeaderAlloc(TPOSE_IO_MAX_FIELDS, TPOSE_IO_MODIFY_HEADER);
    if(!header) exit(EXIT_FAILURE);
    tposeIODiscoverGroups(query, tree, header, query->inputFile->dataAddr,
                          query->inputFile->fileAddr + query->inputFile->fileSize);
    tposeIORequireGroups(header);
    query->outputFile->fileGroupHeader = header;
}




/** 
 ** "Simple" tranpose of rows-to-columns (naive algorithm)
 **/
void tposeIOTransposeSimple(TposeQuery* query) {
    const char* end = query->inputFile->fileAddr + query->inputFile->fileSize;
    unsigned int columns = query->inputFile->fileHeader->maxFields;
    const char* cursor = query->inputFile->fileAddr;
    TposeRecord record;
    size_t row = 0;
    /* Validate the whole table before emitting any transposed data. Empty
       fields count, including the field after a trailing delimiter. */
    while(tposeIONextRecord(&cursor, end, &record)) {
        size_t fields = 1;
        for(const char* p = record.begin; p < record.end; ++p)
            if(*p == query->inputFile->fieldDelimiter) ++fields;
        ++row;
        if(fields != columns) {
            fprintf(stderr, "Error: Row %zu has %zu field%s; expected %u\n",
                    row, fields, fields == 1 ? "" : "s", columns);
            exit(EXIT_FAILURE);
        }
    }

    char field[TPOSE_IO_MAX_FIELD_WIDTH];
    for(unsigned int column = 0; column < columns; ++column) {
        cursor = query->inputFile->fileAddr;
        int first = 1;
        while(tposeIONextRecord(&cursor, end, &record)) {
            tposeIOReadField(record, query->inputFile->fieldDelimiter, (int) column, field, "Field");
            if(!first) fputc(query->outputFile->fieldDelimiter, query->outputFile->fd);
            fputs(field, query->outputFile->fd);
            first = 0;
        }
        fputc(rowDelimiter, query->outputFile->fd);
    }
}




/** 
 ** Transposes numeric values for each unique group value
 **/
void tposeIOTransposeGroup(TposeQuery* query, BTree* tree) {
    query->aggregator = tposeIOAggregatorAlloc(query->outputFile->fileGroupHeader->numFields);
    if(!query->aggregator) exit(EXIT_FAILURE);
    tposeIOAggregateRange(query, tree, query->aggregator, query->inputFile->dataAddr,
                          query->inputFile->fileAddr + query->inputFile->fileSize, -1);
    tposeIOCalculateAverages(query->aggregator);
    tposeIOPrintOutput(query);
}




/** 
 ** Transposes numeric values for each unique group and id value
 **/
void tposeIOTransposeGroupId(TposeQuery* query, BTree* tree) {
    query->aggregator = tposeIOAggregatorAlloc(query->outputFile->fileGroupHeader->numFields);
    if(!query->aggregator) exit(EXIT_FAILURE);
    tposeIOPrintGroupIdHeader(query);
    tposeIOAggregateRange(query, tree, query->aggregator, query->inputFile->dataAddr,
                          query->inputFile->fileAddr + query->inputFile->fileSize, -1);
}




/** 
 ** Paritions file into *correct* chunks for parallel-processing
 ** Multi-threaded only
 **/
int tposeIOBuildPartitions(TposeQuery* query, unsigned int mode) {
    if(mode != TPOSE_IO_PARTITION_GROUP && mode != TPOSE_IO_PARTITION_ID) return -1;
    const char* data = query->inputFile->dataAddr;
    off_t length = tposeIODataSize(query->inputFile);
    const char* end = data + length;
    fileChunks = 0;
    partitions[0] = 0;
    /* Retain the existing remainder-first chunk layout, using data bytes only. */
    off_t candidate = length % TPOSE_IO_CHUNK_SIZE;
    if(!candidate) candidate = TPOSE_IO_CHUNK_SIZE;
    while(candidate < length) {
        if(candidate > partitions[fileChunks]) {
            const char* cursor = data + candidate;
            if(cursor[-1] != rowDelimiter) {
                const char* newline = memchr(cursor, rowDelimiter, (size_t) (end - cursor));
                cursor = newline ? newline + 1 : end;
            }
            if(mode == TPOSE_IO_PARTITION_ID) {
                char currentId[TPOSE_IO_MAX_FIELD_WIDTH] = "";
                char id[TPOSE_IO_MAX_FIELD_WIDTH];
                TposeRecord record;
                while(tposeIONextRecord(&cursor, end, &record)) {
                    tposeIOReadField(record, query->inputFile->fieldDelimiter, query->id, id, "ID field");
                    if(!id[0]) continue;
                    if(currentId[0] && strcmp(currentId, id)) {
                        cursor = record.begin;
                        break;
                    }
                    strcpy(currentId, id);
                }
            }
            off_t boundary = cursor - data;
            if(boundary == length) break;
            if(boundary > partitions[fileChunks]) {
                /* Retain the current partition limits, reserving the final endpoint. */
                unsigned int maxChunks = mode == TPOSE_IO_PARTITION_ID ? 100 : 999;
                if(fileChunks + 1 >= maxChunks) {
                    fprintf(stderr, "Error: Too many input partitions\n");
                    return -1;
                }
                partitions[++fileChunks] = boundary;
            }
        }
        if(length - candidate <= TPOSE_IO_CHUNK_SIZE) break;
        candidate += TPOSE_IO_CHUNK_SIZE;
    }
    partitions[++fileChunks] = length;
    return 0;
}




/** 
 ** Returns a unique list of GROUP variable values 
 ** Coordinator for multi-threaded version
 **/
void tposeIOUniqueGroupsParallel(
	TposeQuery* tposeQuery
) {

	extern unsigned int fileChunks; // Number of file chunks

	unsigned int threadsCtr = 0;
	pthread_t threads[fileChunks]; // Thread array

	// Allocate memory for threadDataArray (one for each file chunk)
	if((threadDataArray = (TposeThreadData**) calloc(1, fileChunks * sizeof(TposeThreadData*))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		exit(EXIT_FAILURE);
	}

	// Create threads
	for(threadsCtr = 0; threadsCtr < fileChunks; threadsCtr++) {

		if((threadData = (TposeThreadData*) calloc(1, sizeof(TposeThreadData))) == NULL ) {
			fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
			exit(EXIT_FAILURE);
		}
		
		// Assign thread arguments
		threadData->threadId = threadsCtr;
		threadData->query = tposeQuery; 
		threadData->header = (TposeHeader*) tposeIOHeaderAlloc(TPOSE_IO_MAX_FIELDS, TPOSE_IO_MODIFY_HEADER); // Allocate the needed memory
		threadDataArray[threadsCtr] = threadData; 

		if(pthread_create(&threads[threadsCtr], NULL, tposeIOUniqueGroupsMap, (void *) threadDataArray[threadsCtr])) {
			fprintf(stderr, "Error: Cannot create thread - attempt to run tpose in single-threaded mode\n");
			exit(EXIT_FAILURE);
		}
	}
	
	// Sync threads
	for(threadsCtr = 0; threadsCtr < fileChunks; threadsCtr++)
		(void) pthread_join(threads[threadsCtr], NULL);

	// Reduce output header
	tposeIOUniqueGroupsReduce(tposeQuery);

	// Clean-up
	for(threadsCtr = 0; threadsCtr < fileChunks; threadsCtr++) {
		free(threadDataArray[threadsCtr]);
	}
	free(threadDataArray);

}



/** 
 ** Returns a unique list of GROUP variable values 
 ** Maps file chunks to each thread
 **/
void* tposeIOUniqueGroupsMap(void* threadArg) {
    TposeThreadData* worker = threadArg;
    BTree* tree = btreeAlloc();
    TposeRecord range = tposeIOPartitionRange(worker->query, worker->threadId);
    tposeIODiscoverGroups(worker->query, tree, worker->header, range.begin, range.end);
    btreeFree(&tree);
    return NULL;
}




/** 
 ** Returns a unique list of GROUP variable values 
 ** Reduces thread results into final output
 **/
void tposeIOUniqueGroupsReduce(
	TposeQuery* tposeQuery
) {
	
	// Counters & limits
	unsigned int mutateHeader = 1; // Allow for header row to be modified
	off_t uniqueGroupCount = 0; // Used to index array of header ptrs
	off_t groupCharCount = 0; 
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)

	// Temp allocs
	BTreeKey* key = btreeKeyAlloc();
	BTreeKey* resultKey;
	char tempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* allocString;
	char* charSavePtr; // Points at start of each field after every loop


	// Reduced output header
	TposeHeader* header = tposeIOHeaderAlloc(TPOSE_IO_MAX_FIELDS, mutateHeader); // Allocate the needed memory*/


	// Reduce parallel headers to single output header
	int threadCtr, fieldCtr;
	for(threadCtr = 0; threadCtr < fileChunks; threadCtr++) {

		for(fieldCtr = 0; fieldCtr < (threadDataArray[threadCtr]->header)->numFields; fieldCtr++) {

				charSavePtr = (threadDataArray[threadCtr]->header)->fields[fieldCtr];
				
				// Copy field value
				while(*charSavePtr != '\0') {
					tposeIOCheckFieldWidth(groupCharCount + 1, "Group field");
					tempString[groupCharCount++] = *charSavePtr++;
				}
				tempString[groupCharCount] = '\0';


				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btreeGlobal, btreeGlobal->root, tempString)) == NULL) {

					tposeIOCheckGroupCapacity(uniqueGroupCount, header);

					// The header owns this stable string; the tree borrows it.
					allocString = tposeIODuplicateString(tempString);
					btreeSetKeyValue(key, allocString, uniqueGroupCount, 0);
					if(btreeInsert(btreeGlobal, key) == -1) {
						fprintf(stderr, "Error: Cannot insert value into btree\n");
					}

					// Insert into TposeHeader object
					*(header->fields+(uniqueGroupCount++)) = allocString;
				}

				// Reset variables (
				groupCharCount = 0;
		}
	}

	// Update count of unique groups
	header->numFields = uniqueGroupCount;
	tposeIORequireGroups(header);

	// Return header
	(tposeQuery->outputFile)->fileGroupHeader = header; 

}



/** 
 ** Transposes numeric values for each unique group value
 ** Coordinator for multi-threaded version
 **/
void tposeIOTransposeGroupParallel(
	TposeQuery* tposeQuery
) {

	extern unsigned int fileChunks; // Number of file chunks

	pthread_t threads[fileChunks]; // Thread array
	int threadCtr = 0;

	// Allocate memory for threadAggregatorArray (one for each file chunk)
	if((threadAggregatorArray = (TposeThreadAggregator**) calloc(1, fileChunks * sizeof(TposeThreadAggregator*))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		exit(EXIT_FAILURE);
	}

	// Create threads
	for(threadCtr = 0; threadCtr < fileChunks; threadCtr++) {

		if((threadAggregator = (TposeThreadAggregator*) calloc(1, sizeof(TposeThreadAggregator))) == NULL ) {
			fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
			exit(EXIT_FAILURE);
		}

		// Assign arguments to current thread
		threadAggregator->threadId = threadCtr;
		threadAggregator->query = tposeQuery; 
		threadAggregator->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields);
		threadAggregatorArray[threadCtr] = threadAggregator; 

		// Map input to threads
		if(pthread_create(&threads[threadCtr], NULL, tposeIOTransposeGroupMap, (void *) threadAggregatorArray[threadCtr])) {
			fprintf(stderr, "Error: cannot create thread - attempt to run tpose in single-threaded mode\n");
			exit(EXIT_FAILURE);
		}

	}
	
	// Sync threads
	for(threadCtr=0; threadCtr<fileChunks; threadCtr++)
		(void) pthread_join(threads[threadCtr], NULL);

	// Reduce output header
	tposeIOTransposeGroupReduce(tposeQuery);

	// Clean-up
	for(threadCtr=0; threadCtr<fileChunks; threadCtr++) {
		tposeIOAggregatorFree(&(threadAggregatorArray[threadCtr]->aggregator));
	}
	free(threadAggregatorArray);

}



/** 
 ** Transposes numeric values for each unique group value
 ** Maps file chunks to each thread
 **/
void* tposeIOTransposeGroupMap(void* threadArg) {
    TposeThreadAggregator* worker = threadArg;
    TposeRecord range = tposeIOPartitionRange(worker->query, worker->threadId);
    tposeIOAggregateRange(worker->query, btreeGlobal, worker->aggregator, range.begin, range.end,
                          (int) worker->threadId);
    return NULL;
}




/** 
 ** Transposes numeric values for each unique group value
 ** Reduces thread results into final output
 **/
void tposeIOTransposeGroupReduce(
	TposeQuery* tposeQuery
){

	// Aggregate thread results
	tposeQuery->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields);

	unsigned int threadCtr, fieldCtr;
	for(threadCtr=0; threadCtr < fileChunks; threadCtr++) {
		for(fieldCtr=0; fieldCtr < (threadAggregatorArray[threadCtr]->aggregator)->numFields; fieldCtr++) {
			(tposeQuery->aggregator)->aggregates[fieldCtr] += (threadAggregatorArray[threadCtr]->aggregator)->aggregates[fieldCtr];
			(tposeQuery->aggregator)->counts[fieldCtr] += (threadAggregatorArray[threadCtr]->aggregator)->counts[fieldCtr];
		}
	}

	// Calculate averages
	int ctr;
	for(ctr = 0; ctr < ((tposeQuery->outputFile)->fileGroupHeader)->numFields; ++ctr)
		(tposeQuery->aggregator)->avgs[ctr] = (tposeQuery->aggregator)->aggregates[ctr] / (tposeQuery->aggregator)->counts[ctr];

	// Print aggregates to final output
	tposeIOPrintOutput(tposeQuery);

	// Clean-up
	tposeIOAggregatorFree(&(tposeQuery->aggregator));
	
}



/** 
 ** Transposes numeric values for each unique group and id value
 ** Coordinator for multi-threaded version
 **/
void tposeIOTransposeGroupIdParallel(
	TposeQuery* tposeQuery
) {

	extern unsigned int fileChunks; // Number of file chunks

	pthread_t threads[fileChunks]; // Thread array
	int threadCtr = 0;

	// Allocate memory for threadAggregatorArray (one for each file chunk)
	if((threadAggregatorArray = (TposeThreadAggregator**) calloc(1, fileChunks * sizeof(TposeThreadAggregator*))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		exit(EXIT_FAILURE);
	}

	// Create threads
	for(threadCtr = 0; threadCtr < fileChunks; threadCtr++) {

		/* Private scratch storage cannot collide with input, output, another
		   invocation, or unrelated files in the working directory. */
		FILE* scratch = tmpfile();
		if(!scratch) {
			fprintf(stderr, "Error: Cannot create parallel temporary stream\n");
			exit(EXIT_FAILURE);
		}
		tempFileArray[threadCtr] = tposeIOOutputFileAlloc(scratch, tposeQuery->inputFile->fieldDelimiter);
		if(!tempFileArray[threadCtr]) {
			fclose(scratch);
			exit(EXIT_FAILURE);
		}

		if((threadAggregator = (TposeThreadAggregator*) calloc(1, sizeof(TposeThreadAggregator))) == NULL ) {
			fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
			exit(EXIT_FAILURE);
		}

		// Assign arguments to current thread
		threadAggregator->threadId = threadCtr;
		threadAggregator->query = tposeQuery; 
		threadAggregator->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields);
		threadAggregatorArray[threadCtr] = threadAggregator; 

		// Map input to threads
		if(pthread_create(&threads[threadCtr], NULL, tposeIOTransposeGroupIdMap, (void *) threadAggregatorArray[threadCtr])) {
			fprintf(stderr, "Error: Cannot create thread - attempt to run tpose in single-threaded mode\n");
			exit(EXIT_FAILURE);
		}
	}
	
	// Sync threads
	for(threadCtr=0; threadCtr<fileChunks; threadCtr++)
		(void) pthread_join(threads[threadCtr], NULL);

	
	// Reduce output header
	tposeIOTransposeGroupIdReduce(tposeQuery);

	// Clean-up
	for(threadCtr=0; threadCtr<fileChunks; threadCtr++) {
		tposeIOAggregatorFree(&(threadAggregatorArray[threadCtr]->aggregator));
		if(tposeIOCloseOutputFile(tempFileArray[threadCtr]) != 0) exit(EXIT_FAILURE);
	}
	free(threadAggregatorArray);

}



/** 
 ** Transposes numeric values for each unique group and id value
 ** Maps file chunks to each thread
 **/
void* tposeIOTransposeGroupIdMap(void* threadArg) {
    TposeThreadAggregator* worker = threadArg;
    TposeRecord range = tposeIOPartitionRange(worker->query, worker->threadId);
    if(worker->threadId == 0) tposeIOPrintGroupIdHeaderParallel(worker->query, 0);
    tposeIOAggregateRange(worker->query, btreeGlobal, worker->aggregator, range.begin, range.end,
                          (int) worker->threadId);
    return NULL;
}




/** 
 ** Transposes numeric values for each unique group and id value
 ** Reduces thread results into final output
 **/
void tposeIOTransposeGroupIdReduce(
	TposeQuery* tposeQuery
) {

	FILE* fdSrc; 
	FILE* fdDest = (tposeQuery->outputFile)->fd;
	int c;
	unsigned int threadCtr;

	// Write temp files to final output file
	for(threadCtr=0; threadCtr < fileChunks; threadCtr++) {
		fdSrc = tempFileArray[threadCtr]->fd;
		tposeIOFlushOutput(fdSrc);
		if(fseek(fdSrc, 0, SEEK_SET) != 0) {
			fprintf(stderr, "Error: Cannot seek parallel output stream\n");
			exit(EXIT_FAILURE);
		}
		while((c = getc(fdSrc)) != EOF) {
			if(putc(c, fdDest) == EOF) {
				fprintf(stderr, "Error: Cannot write output stream\n");
				exit(EXIT_FAILURE);
			}
		}
		if(ferror(fdSrc)) {
			fprintf(stderr, "Error: Cannot read parallel output stream\n");
			exit(EXIT_FAILURE);
		}
	}

}



/** 
 ** Iterates through unique groups and aggregates and formats output
 ** Used to output results from tposeIOTransposeGroup()
 **/
void tposeIOPrintOutput(
	TposeQuery* tposeQuery
) {

	unsigned char fieldDelimiter = (tposeQuery->outputFile)->fieldDelimiter;

	int i; // counter

	// Group Header
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == (((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1))
			fprintf((tposeQuery->outputFile)->fd, "%s%c", ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], rowDelimiter);
		else
			fprintf((tposeQuery->outputFile)->fd, "%s%c", ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], fieldDelimiter);
	}


	// Aggregates
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_SUM) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], fieldDelimiter);
		}

		tposeIOFlushOutput((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_COUNT) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%lld%c", (long long) (tposeQuery->aggregator)->counts[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%lld%c", (long long) (tposeQuery->aggregator)->counts[i], fieldDelimiter);
		}

		tposeIOFlushOutput((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_AVG) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], fieldDelimiter);
		}

		tposeIOFlushOutput((tposeQuery->outputFile)->fd);
	}

}



/** 
 ** Prints output header
 **/
void tposeIOPrintGroupIdHeader(
	TposeQuery* tposeQuery
) {

	unsigned char fieldDelimiter = (tposeQuery->outputFile)->fieldDelimiter;
	
	// Id Header
	fprintf((tposeQuery->outputFile)->fd, "%s%c", ((tposeQuery->inputFile)->fileHeader)->fields[tposeQuery->id], fieldDelimiter);

	int i;
	// Group Header
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == (((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1))
			fprintf((tposeQuery->outputFile)->fd, "%s%s%s%c", prefixGlobal, ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], suffixGlobal, rowDelimiter);
		else
			fprintf((tposeQuery->outputFile)->fd, "%s%s%s%c", prefixGlobal, ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], suffixGlobal, fieldDelimiter);
	}

}



/** 
 ** Prints current line to output
 **/
void tposeIOPrintGroupIdData(
	char* id
	,TposeQuery* tposeQuery
) {

	unsigned char fieldDelimiter = (tposeQuery->outputFile)->fieldDelimiter;

	int i; // Counter

	// Id
	fprintf((tposeQuery->outputFile)->fd, "%s%c", id, fieldDelimiter);

	// Aggregates
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_SUM) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], fieldDelimiter);
		}

		tposeIOFlushOutput((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_COUNT) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%lld%c", (long long) (tposeQuery->aggregator)->counts[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%lld%c", (long long) (tposeQuery->aggregator)->counts[i], fieldDelimiter);
		}

		tposeIOFlushOutput((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_AVG) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], fieldDelimiter);
		}

		tposeIOFlushOutput((tposeQuery->outputFile)->fd);
	}
}



/** 
 ** Prints output header
 ** Multi-threaded version
 **/
void tposeIOPrintGroupIdHeaderParallel(
	TposeQuery* tposeQuery
	,unsigned int threadId
) {

	unsigned char fieldDelimiter = (tposeQuery->outputFile)->fieldDelimiter;
	FILE* fd = tempFileArray[threadId]->fd;
	
	// Id Header
	fprintf(fd, "%s%c", ((tposeQuery->inputFile)->fileHeader)->fields[tposeQuery->id], fieldDelimiter);

	int i;
	// Group Header
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == (((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1))
			fprintf(fd, "%s%s%s%c", prefixGlobal, ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], suffixGlobal, rowDelimiter);
		else
			fprintf(fd, "%s%s%s%c", prefixGlobal, ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], suffixGlobal, fieldDelimiter);
	}


}



/** 
 ** Prints current line to output
 ** Multi-threaded version
 **/
void tposeIOPrintGroupIdDataParallel(
	char* id
	,TposeQuery* tposeQuery
	,TposeAggregator* aggregator
	,unsigned int threadId
) {

	unsigned char fieldDelimiter = (tposeQuery->outputFile)->fieldDelimiter;
	FILE* fd = tempFileArray[threadId]->fd;

	int i; // Counter

	// Id
	fprintf(fd, "%s%c", id, fieldDelimiter);
	
	// Aggregates
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_SUM) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf(fd, "%.2f%c", aggregator->aggregates[i], rowDelimiter);
			else
				fprintf(fd, "%.2f%c", aggregator->aggregates[i], fieldDelimiter);
		}

		tposeIOFlushOutput(fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_COUNT) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf(fd, "%lld%c", (long long) aggregator->counts[i], rowDelimiter);
			else
				fprintf(fd, "%lld%c", (long long) aggregator->counts[i], fieldDelimiter);
		}

		tposeIOFlushOutput(fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_AVG) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf(fd, "%.2f%c", aggregator->avgs[i], rowDelimiter);
			else
				fprintf(fd, "%.2f%c", aggregator->avgs[i], fieldDelimiter);
		}

		tposeIOFlushOutput(fd);
	}

}
