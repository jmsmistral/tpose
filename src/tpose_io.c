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

unsigned char rowDelimiter = '\n';
char* prefixGlobal;
char* suffixGlobal;
extern int errno;

	

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
	
	assert(outputFile->fd != NULL);

	return outputFile;
	
}



/** 
 ** Free memory for a TposeOutputFile 
 **/
void tposeIOOutputFileFree(
    TposeOutputFile** outputFilePtr
) {

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
		if((tposeHeader->fields = (char**) malloc(maxFields * sizeof(char*))) == NULL ) {
			fprintf(stderr, "Error: Cannot allocate header fields memory\n");
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
	if(idVar != -1) tposeQuery->id = idVar;
	if(groupVar != -1) tposeQuery->group = groupVar;
	if(numericVar != -1) tposeQuery->numeric = numericVar;
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
	
	// Close if not standard output
	if(outputFile->fd != stdout) {  
		if(ferror(outputFile->fd))
			fprintf(stderr, "Error: Output file closed with errors - check data (Error number = %d - %s)\n", errno, strerror(errno));
		fclose(outputFile->fd);
	}

	// Free TposeOutputFile memory
	tposeIOOutputFileFree(&outputFile);

	return 0;

}



/** 
 ** Reads the first line (header) of an input file
 ** tpose only accepts data files with column headers on first line
 **/
TposeHeader* tposeIOReadInputHeader(
	TposeInputFile* inputFile
	,unsigned int mutateHeader
) {
    
	char* rowtok;
	char* fieldtok;
	char* fieldSavePtr;
	
	char* tempString;

	unsigned int fieldCount = 0;
	unsigned int length = 0;
	unsigned int curField = 0;

	// Test if we have a good TposeInputFile*
	if(!inputFile) return NULL;

	// Count number of fields
	while( *(inputFile->fileAddr+length) != rowDelimiter) {
		if( *(inputFile->fileAddr+length) == (inputFile->fieldDelimiter))
			fieldCount++; length++;
	}
	
	if(fieldCount > 0)
		fieldCount++; // Quick hack to get real number of fields

	TposeHeader* header = tposeIOHeaderAlloc(fieldCount, mutateHeader); // Allocate the needed memory

	if(mutateHeader) {
		// Find index of first row delimiter
		inputFile->dataAddr = strchr(inputFile->fileAddr, rowDelimiter);
		*inputFile->dataAddr = '\0';
		inputFile->dataAddr+=1; // Make sure we're not pointing at the NULL 

		// Create a copy of the NULL terminated string (as strsep/strtok_r modifies this)
		rowtok = strdup(inputFile->fileAddr);
		
		// Read header fields
		fieldtok = strtok_r(rowtok, &(inputFile->fieldDelimiter), &fieldSavePtr);
		if(fieldtok == NULL) return NULL;
		tempString = malloc(strlen(fieldtok) * sizeof(char));
		strcpy(tempString, fieldtok);
		*(header->fields) = tposeIOLowerCase(tempString);
		for(fieldCount = 1; (fieldtok = strtok_r(NULL, &(inputFile->fieldDelimiter), &fieldSavePtr)) != NULL; ) {
			tempString = malloc(strlen(fieldtok) * sizeof(char));
			strcpy(tempString, fieldtok);
			*(header->fields+(fieldCount++)) = tposeIOLowerCase(tempString);
		}
		
		// Clean-up
		free(rowtok);
	}

	return header;

}



/** 
 ** Returns a unique list of GROUP variable values 
 **/
void tposeIOUniqueGroups(
	TposeQuery* tposeQuery
	,BTree* btree
) {

	// Flags & static vars
	unsigned int mutateHeader = 1; // Allow for header row to be modified
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	
	// Temp allocs
	TposeHeader* header = tposeIOHeaderAlloc(TPOSE_IO_MAX_FIELDS, mutateHeader); // Allocate the needed memory
	BTreeKey* key = btreeKeyAlloc();
	BTreeKey* resultKey;
	char tempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* allocString;
	char* fieldSavePtr; // Points at start of each field after every loop

	// Counters & limits
	off_t rowCount = 2; // For debugging only
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	off_t uniqueGroupCount = 0; // Used to index array of header ptrs
	off_t groupCharCount = 0; 
	off_t hashCharCount = 0; 
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	off_t hashValue = 0; 
	off_t fileSize = (tposeQuery->inputFile)->fileSize; 
	off_t rFileSize = fileSize; // Remaining file size
	unsigned int chunks = 1; // Number of file chunks


	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)


	// Calculate file chunks
	while(rFileSize >= TPOSE_IO_CHUNK_SIZE){
		++chunks;
		rFileSize -= TPOSE_IO_CHUNK_SIZE;
	}


	// Split large files into chunks
	int chunkCtr;
	off_t iFileSize;
	for(chunkCtr = 1; chunkCtr <= chunks; chunkCtr++) {
		
		// Set file size limit for current chunk
		if(chunkCtr == chunks)
			iFileSize = rFileSize;
		else
			iFileSize = TPOSE_IO_CHUNK_SIZE;

		// Reset vars for each chunk
		totalCharCount = 0;
			
		// Process each chunk
		while(totalCharCount <= iFileSize) {

			// FIELD DELIMITER
			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				if(++totalCharCount == iFileSize) break;
				continue;
			}

			// ROW DELIMITER
			if(*fieldSavePtr == rowDelimiter) {
				fieldCount = 0;
				++rowCount;
				++fieldSavePtr;
				if(++totalCharCount == iFileSize) break;
				continue;
			}
			
			// GROUP FIELD
			if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					tempString[groupCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == fileSize) break;
				}
				tempString[groupCharCount] = '\0'; // Null-terminate string

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= groupCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) tempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) == NULL) {

					// Check for collisions - print only when we add a unique group
					debug_print("tposeIOgetUniqueGroups(): New group value found = '");
					debug_print("tposeIOgetUniqueGroups(): row %u = %s\t%ld\t%u\n", rowCount, tempString, hashValue, uniqueGroupCount);

					btreeSetKeyValue(key, hashValue, uniqueGroupCount, 0);
					if(btreeInsert(btree, key) == -1)
						fprintf(stderr, "Error: Cannot insert value into btree\n");

					// Insert into TposeHeader object
					allocString = malloc(strlen(tempString) * sizeof(char));
					strcpy(allocString, tempString);
					*(header->fields+(uniqueGroupCount++)) = allocString;
					header->numFields = uniqueGroupCount; // Update number of fields in header
				}

				// Reset variables (
				groupCharCount = 0;
				hashValue = 0;
				
				if(*fieldSavePtr == fieldDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}

			// If not a delimiter or group field, then ++
			++fieldSavePtr;
			if(++totalCharCount == iFileSize) break;

		}

	} // End for-loop over chunks 

	btreeKeyFree(&key);

	// Assign groups to output file header 
	(tposeQuery->outputFile)->fileGroupHeader = header; 
	
}



/** 
 ** "Simple" tranpose of rows-to-columns (naive algorithm)
 **/
void tposeIOTransposeSimple(
	TposeQuery* tposeQuery
) {

	// Flags & static vars
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	off_t numFields = ((tposeQuery->inputFile)->fileHeader)->maxFields;
	off_t fileSize = (tposeQuery->inputFile)->fileSize; 

	// Temp allocs
	char fieldTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* fieldSavePtr; 

	// Counters & limits
	unsigned int fieldCount = 0; // points at first field when loop starts 
	unsigned int fieldCharCount = 0; 
	unsigned int currentField = 0;
	off_t totalCharCount = 0; // mmap files are page aligned, so we end-up reading garbage after file data ends


	// Process each field at a time
	for(currentField = 0; currentField < numFields; ++currentField) {

		fieldSavePtr = (tposeQuery->inputFile)->fileAddr; // Re-initialise pointer to data 
		totalCharCount = 0;

		// Scan the file
		while(totalCharCount <= (tposeQuery->inputFile)->fileSize) {

			// FIELD DELIMITER
			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				if(++totalCharCount == fileSize) break;
				continue;
			}

			// ROW DELIMITER
			if(*fieldSavePtr == rowDelimiter) {
				if((fieldCount != 0) && (fieldCount == numFields - 1)) {
					// Print output
					fprintf((tposeQuery->outputFile)->fd, "%s%c", fieldTempString, fieldDelimiter);
					fflush((tposeQuery->outputFile)->fd); 
				}
					
				// Reset flags for next row
				fieldCount = 0;

				++fieldSavePtr;
				if(++totalCharCount == fileSize) break;
				continue;
			}
			
			// CURRENT FIELD BEING PROCESSED
			if((totalCharCount != (tposeQuery->inputFile)->fileSize) && (fieldCount == currentField))  { // if group value is empty string we ignore

				// Get current field value
				while((*fieldSavePtr != fieldDelimiter) && (*fieldSavePtr != rowDelimiter)) {
					fieldTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == fileSize) break;
				}
				fieldTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Reset variables
				fieldCharCount = 0;
				
				if(*fieldSavePtr == fieldDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}
			
			// If not a delimiter or the counted field, then ++
			++fieldSavePtr;
			if(++totalCharCount == fileSize) break;

		} // End while-loop

		fprintf((tposeQuery->outputFile)->fd, "%c", rowDelimiter); // Output a new line after each iteration
		fflush((tposeQuery->outputFile)->fd);
		
	
	} // End for-loop

}



/** 
 ** Transposes numeric values for each unique group value
 **/
void tposeIOTransposeGroup(
	TposeQuery* tposeQuery
	,BTree* btree
) {


	// Flags & static vars
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)
	unsigned int numericFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)

	// Temp allocs
	tposeQuery->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields);
	BTreeKey* resultKey;
	char groupTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char numericTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* fieldSavePtr;
	
	// Counters & limits
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned int fieldCharCount = 0;
	unsigned int hashCharCount = 0; 
	off_t hashValue = 0; 
	off_t totalCharCount = 0; // mmap files are page aligned, so we end-up reading garbage after file data ends
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct
	off_t fileSize = (tposeQuery->inputFile)->fileSize;
	off_t rFileSize = fileSize;
	unsigned int chunks = 1; // Number of file chunks


	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)


	// Calculate file chunks
	while(rFileSize >= TPOSE_IO_CHUNK_SIZE){
		++chunks;
		rFileSize -= TPOSE_IO_CHUNK_SIZE;
	}


	// Split large files into chunks
	int chunkCtr;
	off_t iFileSize;
	for(chunkCtr = 1; chunkCtr <= chunks; chunkCtr++) {
		
		// Set file size limit for current chunk
		if(chunkCtr == chunks)
			iFileSize = rFileSize;
		else
			iFileSize = TPOSE_IO_CHUNK_SIZE;

		totalCharCount = 0; // Reset count

		// Process each chunk
		while(totalCharCount <= iFileSize) {

			// FIELD DELIMITER
			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				if(++totalCharCount == iFileSize) break;
				continue;
			}

			// ROW DELIMITER
			if(*fieldSavePtr == rowDelimiter) {

				// Aggregate value for each group
				if((groupFoundFlag == 1) && (numericFoundFlag == 1)) {
					(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);
					(tposeQuery->aggregator)->counts[groupFieldIndex]++;
				}
					
				// Reset flags for next row
				groupFoundFlag = 0;
				numericFoundFlag = 0;

				fieldCount = 0;
				++fieldSavePtr;
				if(++totalCharCount == iFileSize) break;
				continue;

			}

			// GROUP FIELD
			if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

				// Get group field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					groupTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == iFileSize) break;
				}
				groupTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= fieldCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) groupTempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) != NULL) {
					groupFoundFlag = 1; // Flag group field as found
					groupFieldIndex = resultKey->dataOffset;
				}

				// Reset variables
				fieldCharCount = 0;
				hashValue = 0;
				
				if(*fieldSavePtr == fieldDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}
			
			// NUMERIC FIELD
			if(fieldCount == (tposeQuery->numeric))  { // if group value is empty string we ignore

				numericFoundFlag = 1; // Flag numeric field as found
				
				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					numericTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == iFileSize) break;
				}
				numericTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Reset vars
				fieldCharCount = 0;

				if(*fieldSavePtr == fieldDelimiter) {
					--totalCharCount;
					--fieldSavePtr;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}

			// If not a delimiter or group field, then ++
			++fieldSavePtr;
			if(++totalCharCount == iFileSize) break;

		}

	} // End for-loop
	
	// Calculate averges
	int ctr;
	for(ctr = 0; ctr < ((tposeQuery->outputFile)->fileGroupHeader)->numFields; ++ctr)
		(tposeQuery->aggregator)->avgs[ctr] = (tposeQuery->aggregator)->aggregates[ctr] / (tposeQuery->aggregator)->counts[ctr];

	tposeIOPrintOutput(tposeQuery);

}



/** 
 ** Transposes numeric values for each unique group and id value
 **/
void tposeIOTransposeGroupId(
	TposeQuery* tposeQuery
	,BTree* btree
) {

	// Flags & static vars
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;

	// Temp allocs
	tposeQuery->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields); // Aggregates values
	BTreeKey* resultKey; // Used for searching the btree
	char idCurrentString[TPOSE_IO_MAX_FIELD_WIDTH]; // Holds current id value being aggregated
	char idTempString[TPOSE_IO_MAX_FIELD_WIDTH]; // Holds id value for each row
	char groupTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char numericTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* fieldSavePtr;

	// Counters & limits
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned int fieldCharCount = 0;
	unsigned int firstId = 1;
	unsigned int idFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)
	unsigned int numericFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)
	int ctr; // Iterates over group fields to calculate average values
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	off_t hashCharCount = 0; 
	off_t hashValue = 0; 
	off_t fileSize = (tposeQuery->inputFile)->fileSize;
	off_t rFileSize = fileSize;
	unsigned int chunks = 1; // Number of file chunks

	// Print output header
	tposeIOPrintGroupIdHeader(tposeQuery); 


	// Calculate file chunks
	while(rFileSize >= TPOSE_IO_CHUNK_SIZE){
		++chunks;
		rFileSize -= TPOSE_IO_CHUNK_SIZE;
	}

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)


	// Split large files into chunks
	int chunkCtr;
	off_t iFileSize;
	for(chunkCtr = 1; chunkCtr <= chunks; chunkCtr++) {
		
		// Set file size limit for current chunk
		if(chunkCtr == chunks)
			iFileSize = rFileSize;
		else
			iFileSize = TPOSE_IO_CHUNK_SIZE;

		totalCharCount = 0; // Reset count

		// Process each chunk
		while(totalCharCount <= iFileSize) {

			// FIELD DELIMITER
			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				if(++totalCharCount == iFileSize) break;
				continue;
			}

			// ROW DELIMITER
			if(*fieldSavePtr == rowDelimiter) {

				if((idFoundFlag == 1) && (groupFoundFlag == 1) && (numericFoundFlag == 1)) {

					if(!strcmp(idCurrentString, idTempString)) {
						// Aggregate value for each group
						(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);
						(tposeQuery->aggregator)->counts[groupFieldIndex]++;
					}
					else {
						// 0 Compute averages
						for(ctr = 0; ctr < ((tposeQuery->outputFile)->fileGroupHeader)->numFields; ++ctr)
							(tposeQuery->aggregator)->avgs[ctr] = (tposeQuery->aggregator)->aggregates[ctr] / (tposeQuery->aggregator)->counts[ctr];
						
						// 1 Print out current aggregates for id
						tposeIOPrintGroupIdData(idCurrentString, tposeQuery);
						// 2 Set new string as current id
						strcpy(idCurrentString, idTempString); // Set current id to aggregate values for
						// 3 Reset aggregates
						memset((tposeQuery->aggregator)->aggregates, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
						memset((tposeQuery->aggregator)->counts, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
						memset((tposeQuery->aggregator)->avgs, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
						// 4 Aggregate value for new id 
						(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);
						(tposeQuery->aggregator)->counts[groupFieldIndex]++;

					}

				}
					
				// Reset flags for next row
				idFoundFlag = 0;
				groupFoundFlag = 0;
				numericFoundFlag = 0;

				fieldCount = 0;
				++fieldSavePtr;
				if(++totalCharCount == iFileSize) break;
				continue;
			}
			
			// GROUP FIELD
			if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

				// Get group field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					groupTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == iFileSize) break;
				}
				groupTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= fieldCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) groupTempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) != NULL) {
					groupFoundFlag = 1; // Flag group field as found
					groupFieldIndex = resultKey->dataOffset; // Is used to correctly order aggregates
				}

				// Reset variables
				fieldCharCount = 0;
				hashValue = 0;
				
				if(*fieldSavePtr == fieldDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}
			
			
			// NUMERIC FIELD
			if(fieldCount == (tposeQuery->numeric))  { // if group value is empty string we ignore

				numericFoundFlag = 1; // Flag numeric field as found
				
				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					numericTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == iFileSize) break;
				}
				numericTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Reset variables
				fieldCharCount = 0;

				if(*fieldSavePtr == fieldDelimiter) {
					--totalCharCount;
					--fieldSavePtr;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}
			
			// ID FIELD
			if(fieldCount == (tposeQuery->id))  { // if group value is empty string we ignore

				idFoundFlag = 1; // Flag numeric field as found
				
				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					idTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == iFileSize) break;
				}
				idTempString[fieldCharCount] = '\0'; // Null-terminate string

				if(firstId) {
					strcpy(idCurrentString, idTempString); // Set current id to aggregate values for
					firstId = 0;
				}
					
				// Reset variables
				fieldCharCount = 0;

				if(*fieldSavePtr == fieldDelimiter) {
					--totalCharCount;
					--fieldSavePtr;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}

			// If not a delimiter or the group field, then ++
			++fieldSavePtr;
			if(++totalCharCount == iFileSize) break;

		}

	} // End for-loop

	for(ctr = 0; ctr < ((tposeQuery->outputFile)->fileGroupHeader)->numFields; ++ctr)
		(tposeQuery->aggregator)->avgs[ctr] = (tposeQuery->aggregator)->aggregates[ctr] / (tposeQuery->aggregator)->counts[ctr];

	// Print last line
	tposeIOPrintGroupIdData(idCurrentString, tposeQuery);

}



/** 
 ** Paritions file into *correct* chunks for parallel-processing
 ** Multi-threaded only
 **/
int tposeIOBuildPartitions(
	TposeQuery* tposeQuery
	,unsigned int mode
) {

	extern unsigned int fileChunks; // Number of file chunks
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	char* fieldSavePtr; // Points at start of each field after every loop
	off_t fileSize = (tposeQuery->inputFile)->fileSize;
	off_t rFileSize = fileSize; // Remaining file size

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)

	// Calculate file chunks
	off_t prepartitions[1000];
	int threadCtr = 0;
	int reverseCtr = 0;
	prepartitions[0] = fileSize;


	while(rFileSize >= TPOSE_IO_CHUNK_SIZE){
		rFileSize -= TPOSE_IO_CHUNK_SIZE;
		prepartitions[fileChunks++] = rFileSize;
	}
	prepartitions[fileChunks] = 0; // First partition starts at byte 0
	
	// Reverse partitions to ease processing
	for(threadCtr=fileChunks; threadCtr>=0; threadCtr--) {
		partitions[reverseCtr++]=prepartitions[threadCtr];
	}


	// Correct partitions to start after new lines
	char* partSavePtr;
	off_t offset;
	if(mode == TPOSE_IO_PARTITION_GROUP) {

		// Don't need to modify first partition (0)
		for(threadCtr=1; threadCtr<fileChunks; threadCtr++) { 
			offset = 0;
			partSavePtr = fieldSavePtr + partitions[threadCtr];
			while(*(partSavePtr++) != '\n') {
				++offset;
			}
			++offset;
			partitions[threadCtr] += offset;
		}

	}
	// Partition file into chunks with mutually excluse set
	// of IDs (avoids more complex post-processing 'shuffle')
	else if(mode == TPOSE_IO_PARTITION_ID) { 

		off_t partitionStart;
		off_t partitionEnd;
		off_t partitionCharLimit;
		unsigned int idFoundFlag = 0; 
		unsigned int fieldCount = 0; 
		off_t fieldCharCount = 0; 
		off_t totalCharCount = 0; 
		char currentId[TPOSE_IO_MAX_FIELD_WIDTH];
		char tempString[TPOSE_IO_MAX_FIELD_WIDTH];
		unsigned int iterationCtr = 0;

		// Don't need to modify first partition (0)
		for(threadCtr=1; threadCtr<fileChunks; threadCtr++) { 
			offset = 0;
			totalCharCount = 0;
			idFoundFlag = 0; 
			iterationCtr = 0;

			// Starting point
			partSavePtr = fieldSavePtr + partitions[threadCtr];

			// Correct to point at start of line
			while(*(partSavePtr++) != '\n') {
				++offset;
			}
			++offset;
			partitions[threadCtr] += offset;
			offset = 0;

			// Set partition params
			partitionStart = partitions[threadCtr];
			partitionEnd = partitions[threadCtr+1];
			partitionCharLimit = partitionEnd - partitionStart;
			
			// Corrected starting point
			partSavePtr = fieldSavePtr + partitions[threadCtr];


			// Loop over file partition
			while(!idFoundFlag) {

				// FIELD DELIMITER
				if(*partSavePtr == fieldDelimiter) {
					++offset;
					++fieldCount;
					++partSavePtr;
					if(++totalCharCount == partitionCharLimit) break;
					continue;
				}

				// ROW DELIMITER
				if(*partSavePtr == rowDelimiter) {
					++offset;
					++partSavePtr;
					fieldCount = 0;
					if(++totalCharCount == partitionCharLimit) break;
					continue;
				}
				
				// ID FIELD
				if(fieldCount == (tposeQuery->id))  { 

					// Copy field value
					while(*partSavePtr != fieldDelimiter && *partSavePtr != rowDelimiter) {
						++offset;
						tempString[fieldCharCount++] = *partSavePtr++;
						if(++totalCharCount == partitionCharLimit) break;
					}
					tempString[fieldCharCount] = '\0'; // Null-terminate string
					
					// Reset variables (
					fieldCharCount = 0;

					// First iteration only - set first id as current
					if(iterationCtr == 0) {
						strcpy(currentId, tempString); 
						iterationCtr++;
					}

					if(!strcmp(currentId, tempString)) {
						debug_print("%u : EQUAL... continuing! currentId = %s | tempId = %s\n", threadCtr, currentId, tempString);
						continue;
					}
					else {
						debug_print("%u : NOT EQUAL... breaking! currentId = %s | tempId = %s\n", threadCtr, currentId, tempString);
						idFoundFlag = 1;
						strcpy(currentId, tempString); 

						// Go back to start of current line
						// to include it in partition
						while(*(partSavePtr--) != '\n') {
							--offset;
						}
						++offset;
						break;
					}
					
					if(*partSavePtr == fieldDelimiter) {
						--offset;
						--partSavePtr;
						--totalCharCount;
					}

					if(*partSavePtr == rowDelimiter) {
						--offset;
						--partSavePtr;
						--totalCharCount;
					}

				}

				// If not a delimiter or group field, then ++
				++partSavePtr;
				++offset;
				if(++totalCharCount == partitionCharLimit) break;

			} // End of while-loop

			// Record new offset
			partitions[threadCtr] += offset;
		}

	}
	else {
		return -1;
	}

	// Print updated partitions
	debug_print("File chunks = %d\n", fileChunks);
	debug_print("Final file partitions...\n");
	for(reverseCtr=0; reverseCtr<fileChunks; reverseCtr++) {
		debug_print("partitions[%u] = %u\n", reverseCtr, partitions[reverseCtr]);
	}

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
void* tposeIOUniqueGroupsMap(
	void* threadArg
) {
	
	// Flags & static vars
	TposeThreadData* threadData = (TposeThreadData*) threadArg;

	TposeQuery* tposeQuery = (TposeQuery*) threadData->query;
	TposeHeader* header = (TposeHeader*) threadData->header;
	unsigned int threadId = (unsigned int) threadData->threadId;
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	off_t partitionStart = partitions[threadId];
	off_t partitionEnd = partitions[threadId+1];
	off_t partitionCharLimit = partitionEnd - partitionStart;

	// Temp allocs
	BTree* btree = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
	BTreeKey* key = btreeKeyAlloc();
	BTreeKey* resultKey;
	char tempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* allocString;
	char* fieldSavePtr; // Points at start of each field after every loop

	// Counters & limits
	off_t rowCount = 2; // For debugging only
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	off_t uniqueGroupCount = 0; // Used to index array of header ptrs
	off_t groupCharCount = 0; 
	off_t hashCharCount = 0; 
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	off_t hashValue = 0; 


	// Init with ptr to second row (where data starts)
	fieldSavePtr = ((tposeQuery->inputFile)->dataAddr) + partitionStart; 


		// Loop over file partition
		while(totalCharCount <= partitionCharLimit) {

			// FIELD DELIMITER
			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				if(++totalCharCount == partitionCharLimit) break;
				continue;
			}

			// ROW DELIMITER
			if(*fieldSavePtr == rowDelimiter) {
				fieldCount = 0;
				++rowCount;
				++fieldSavePtr;
				if(++totalCharCount == partitionCharLimit) break;
				continue;
			}
			
			// GROUP FIELD
			if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					tempString[groupCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == partitionCharLimit) break;
				}
				tempString[groupCharCount] = '\0';

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= groupCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) tempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) == NULL) {

					// Check for collisions - print only when we add a unique group
					debug_print("tposeIOgetUniqueGroups(): New group found = '");
					debug_print("tposeIOgetUniqueGroups(): row=%u / field=%u = %s\thash=%ld\tuniqueGroupCount=%u\n", rowCount, fieldCount, tempString, hashValue, uniqueGroupCount);

					btreeSetKeyValue(key, hashValue, uniqueGroupCount, 0);
					if(btreeInsert(btree, key) == -1) {
						fprintf(stderr, "Error: Cannot insert value into btree\n");
					}

					// Insert into header
					allocString = malloc(strlen(tempString) * sizeof(char));
					strcpy(allocString, tempString);
					*(header->fields+(uniqueGroupCount++)) = allocString;
				}

				// Reset variables
				groupCharCount = 0;
				hashValue = 0;
				
				if(*fieldSavePtr == fieldDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}

			// If not a delimiter or group field, then ++
			++fieldSavePtr;
			if(++totalCharCount == partitionCharLimit) break;

		}

		// Write output header for partition
		header->numFields = uniqueGroupCount;

		// Clean-up
		btreeFree(&btree);
	
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
	off_t hashCharCount = 0; 
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	off_t hashValue = 0; 

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
					tempString[groupCharCount++] = *charSavePtr++;
				}
				tempString[groupCharCount] = '\0';

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= groupCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) tempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btreeGlobal, btreeGlobal->root, hashValue)) == NULL) {

					btreeSetKeyValue(key, hashValue, uniqueGroupCount, 0);
					if(btreeInsert(btreeGlobal, key) == -1) {
						fprintf(stderr, "Error: Cannot insert value into btree\n");
					}

					// Insert into TposeHeader object
					allocString = malloc(strlen(tempString) * sizeof(char));
					strcpy(allocString, tempString);
					*(header->fields+(uniqueGroupCount++)) = allocString;
				}

				// Reset variables (
				groupCharCount = 0;
				hashValue = 0;
		}
	}

	// Update count of unique groups
	header->numFields = uniqueGroupCount;

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
void* tposeIOTransposeGroupMap(
	void* threadArg
) {

	// Flags and static vars
	TposeThreadAggregator* threadAggregator = (TposeThreadAggregator*) threadArg;

	TposeQuery* tposeQuery = (TposeQuery*) threadAggregator->query;
	TposeAggregator* aggregator = (TposeAggregator*) threadAggregator->aggregator;
	unsigned int threadId = (unsigned int) threadAggregator->threadId;
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)
	unsigned int numericFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)
	off_t partitionStart = partitions[threadId];
	off_t partitionEnd = partitions[threadId+1];
	off_t partitionCharLimit = partitionEnd - partitionStart;

	// Temp allocs
	BTreeKey* key = btreeKeyAlloc();
	BTreeKey* resultKey;
	char* allocString;
	char* fieldSavePtr; // Points at start of each field after every loop
	char groupTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char numericTempString[TPOSE_IO_MAX_FIELD_WIDTH];

	// Counters & limits
	off_t rowCount = 2; // For debugging only
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned int fieldCharCount = 0;
	off_t hashCharCount = 0; 
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	off_t hashValue = 0; 
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct


	fieldSavePtr = ((tposeQuery->inputFile)->dataAddr) + partitionStart; // Init with ptr to second row (where data starts)


		/* Process each file chunk */
		while(totalCharCount <= partitionCharLimit) {

			// FIELD DELIMITER
			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				if(++totalCharCount == partitionCharLimit) break;
				continue;
			}

			// ROW DELIMITER
			if(*fieldSavePtr == rowDelimiter) {
				// Aggregate value for each group
				if((groupFoundFlag == 1) && (numericFoundFlag == 1)) {
					aggregator->aggregates[groupFieldIndex] += atof(numericTempString);
					aggregator->counts[groupFieldIndex]++;
				}
					
				// Reset flags for next row
				groupFoundFlag = 0;
				numericFoundFlag = 0;

				fieldCount = 0;
				++fieldSavePtr;
				if(++totalCharCount == partitionCharLimit) break;
				continue;

			}

			// GROUP FIELD
			if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

				// Get group field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					groupTempString[fieldCharCount++] = *fieldSavePtr++;
					++totalCharCount;
				}
				groupTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= fieldCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) groupTempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btreeGlobal, btreeGlobal->root, hashValue)) != NULL) {
					groupFoundFlag = 1; // Flag group field as found
					groupFieldIndex = resultKey->dataOffset;
				}

				// Reset variables
				fieldCharCount = 0;
				hashValue = 0;
				
				if(*fieldSavePtr == fieldDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}
			
			// NUMERIC FIELD
			if(fieldCount == (tposeQuery->numeric))  { // if group value is empty string we ignore

				numericFoundFlag = 1; // Flag numeric field as found
				
				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					numericTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == partitionCharLimit) break;
				}
				numericTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Reset vars
				fieldCharCount = 0;

				if(*fieldSavePtr == fieldDelimiter) {
					--totalCharCount;
					--fieldSavePtr;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}

			// If not a delimiter or group field, then ++
			++fieldSavePtr;
			if(++totalCharCount == partitionCharLimit) break;

		}

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
	char tempFilePath[100][15]; // Array of output filepaths
	int threadCtr = 0;

	// Allocate memory for threadAggregatorArray (one for each file chunk)
	if((threadAggregatorArray = (TposeThreadAggregator**) calloc(1, fileChunks * sizeof(TposeThreadAggregator*))) == NULL ) {
		fprintf(stderr, "Error: Cannot allocate aggregator memory\n");
		exit(EXIT_FAILURE);
	}

	// Create threads
	for(threadCtr = 0; threadCtr < fileChunks; threadCtr++) {

		sprintf(tempFilePath[threadCtr], "temp%u.txt", threadCtr);	
		debug_print("%u : temp file = %s\n", threadCtr, tempFilePath[threadCtr]);
		if((tempFileArray[threadCtr] = tposeIOOpenOutputFile(tempFilePath[threadCtr], "w+",(tposeQuery->inputFile)->fieldDelimiter)) == NULL) {
			fprintf(stderr, "Error: Cannot open temp file\n");
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
		tposeIOCloseOutputFile(tempFileArray[threadCtr]);
		remove(tempFilePath[threadCtr]);
	}
	free(threadAggregatorArray);

}



/** 
 ** Transposes numeric values for each unique group and id value
 ** Maps file chunks to each thread
 **/
void* tposeIOTransposeGroupIdMap(
	void* threadArg
) {

	// Flags & static vars
	TposeThreadAggregator* threadAggregator = (TposeThreadAggregator*) threadArg;

	TposeQuery* tposeQuery = (TposeQuery*) threadAggregator->query;
	TposeAggregator* aggregator = (TposeAggregator*) threadAggregator->aggregator;
	unsigned int threadId = (unsigned int) threadAggregator->threadId;
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	unsigned int idFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)
	unsigned int numericFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)
	off_t partitionStart = partitions[threadId];
	off_t partitionEnd = partitions[threadId+1];
	off_t partitionCharLimit = partitionEnd - partitionStart;

	// Temp allocs
	BTreeKey* key = btreeKeyAlloc();
	BTreeKey* resultKey;
	char idCurrentString[TPOSE_IO_MAX_FIELD_WIDTH]; // Holds current id value being aggregated
	char idTempString[TPOSE_IO_MAX_FIELD_WIDTH]; // Holds id value for each row
	char groupTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char numericTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* allocString;
	char* fieldSavePtr; // Points at start of each field after every loop

	// Counters & limits
	off_t rowCount = 2; // For debugging only
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned int fieldCharCount = 0;
	unsigned int firstId = 1;
	int ctr; // Iterates over group fields to calculate average values
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	off_t hashCharCount = 0; 
	off_t hashValue = 0; 


	// Init with ptr to second row (where data starts)
	fieldSavePtr = ((tposeQuery->inputFile)->dataAddr) + partitionStart; 

	
	// Print output header to first temp file
	if(threadId == 0)
		tposeIOPrintGroupIdHeaderParallel(tposeQuery, threadId); 

		// Process each chunk
		while(totalCharCount <= partitionCharLimit) {

			// FIELD DELIMITER
			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				if(++totalCharCount == partitionCharLimit) break;
				continue;
			}

			// ROW DELIMITER
			if(*fieldSavePtr == rowDelimiter) {

				if((idFoundFlag == 1) && (groupFoundFlag == 1) && (numericFoundFlag == 1)) {
					if(!strcmp(idCurrentString, idTempString)) {
						// Aggregate value for each group
						aggregator->aggregates[groupFieldIndex] += atof(numericTempString);
						aggregator->counts[groupFieldIndex]++;
					}
					else {

						// 0 Compute averages
						for(ctr = 0; ctr < ((tposeQuery->outputFile)->fileGroupHeader)->numFields; ++ctr)
							aggregator->avgs[ctr] = aggregator->aggregates[ctr] / aggregator->counts[ctr];

						// 1 Print out current aggregates for id
						tposeIOPrintGroupIdDataParallel(idCurrentString, tposeQuery, aggregator, threadId);
						// 2 Set new string as current id
						strcpy(idCurrentString, idTempString); // Set current id to aggregate values for
						// 3 Reset aggregates
						memset(aggregator->aggregates, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
						memset(aggregator->counts, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
						memset(aggregator->avgs, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
						// 4 Aggregate value for new id 
						aggregator->aggregates[groupFieldIndex] += atof(numericTempString);
						aggregator->counts[groupFieldIndex]++;

					}

				}
					
				// Reset flags for next row
				idFoundFlag = 0;
				groupFoundFlag = 0;
				numericFoundFlag = 0;

				fieldCount = 0;
				++fieldSavePtr;
				if(++totalCharCount == partitionCharLimit) break;
				continue;
			}
			
			// GROUP FIELD
			if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

				// Get group field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					groupTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == partitionCharLimit) break;
				}
				groupTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= fieldCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) groupTempString[hashCharCount];

				// Insert into btreeGlobal
				if( (resultKey = (BTreeKey*) btreeSearch(btreeGlobal, btreeGlobal->root, hashValue)) != NULL) {
					groupFoundFlag = 1; // Flag group field as found
					groupFieldIndex = resultKey->dataOffset; // Is used to correctly order aggregates
				}

				// Reset variables
				fieldCharCount = 0;
				hashValue = 0;
				
				if(*fieldSavePtr == fieldDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}
			
			
			// NUMERIC FIELD
			if(fieldCount == (tposeQuery->numeric))  { // if group value is empty string we ignore

				numericFoundFlag = 1; // Flag numeric field as found
				
				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					numericTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == partitionCharLimit) break;
				}
				numericTempString[fieldCharCount] = '\0'; // Null-terminate string

				// Reset variables
				fieldCharCount = 0;

				if(*fieldSavePtr == fieldDelimiter) {
					--totalCharCount;
					--fieldSavePtr;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}
			
			// ID FIELD
			if(fieldCount == (tposeQuery->id))  { // if group value is empty string we ignore

				idFoundFlag = 1; // Flag numeric field as found
				
				// Copy field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					idTempString[fieldCharCount++] = *fieldSavePtr++;
					if(++totalCharCount == partitionCharLimit) break;
				}
				idTempString[fieldCharCount] = '\0'; // Null-terminate string

				if(firstId) {
					strcpy(idCurrentString, idTempString); // Set current id to aggregate values for
					firstId = 0;
				}
					
				// Reset variables
				fieldCharCount = 0;

				if(*fieldSavePtr == fieldDelimiter) {
					--totalCharCount;
					--fieldSavePtr;
				}

				if(*fieldSavePtr == rowDelimiter) {
					--fieldSavePtr;
					--totalCharCount;
				}
			}

			// If not a delimiter or the group field, then ++
			++fieldSavePtr;
			if(++totalCharCount == partitionCharLimit) break;

		}

	// Compute averages
	for(ctr = 0; ctr < ((tposeQuery->outputFile)->fileGroupHeader)->numFields; ++ctr)
		aggregator->avgs[ctr] = aggregator->aggregates[ctr] / aggregator->counts[ctr];

	// Print last line
	tposeIOPrintGroupIdDataParallel(idCurrentString, tposeQuery, aggregator, threadId);

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
	char c;
	unsigned int threadCtr, fieldCtr;

	// Write temp files to final output file
	for(threadCtr=0; threadCtr < fileChunks; threadCtr++) {
		fdSrc = tempFileArray[threadCtr]->fd;
		fseek(fdSrc, 0, SEEK_SET);
		clearerr(fdSrc);
		while((c = getc(fdSrc)) != EOF) {
			putc(c, fdDest);
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

		fflush((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_COUNT) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%ld%c", (long long) (tposeQuery->aggregator)->counts[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%ld%c", (long long) (tposeQuery->aggregator)->counts[i], fieldDelimiter);
		}

		fflush((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_AVG) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], fieldDelimiter);
		}

		fflush((tposeQuery->outputFile)->fd);
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

		fflush((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_COUNT) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%ld%c", (long long) (tposeQuery->aggregator)->counts[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%ld%c", (long long) (tposeQuery->aggregator)->counts[i], fieldDelimiter);
		}

		fflush((tposeQuery->outputFile)->fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_AVG) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], rowDelimiter);
			else
				fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->avgs[i], fieldDelimiter);
		}

		fflush((tposeQuery->outputFile)->fd);
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

		fflush(fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_COUNT) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf(fd, "%ld%c", (long long) aggregator->counts[i], rowDelimiter);
			else
				fprintf(fd, "%ld%c", (long long) aggregator->counts[i], fieldDelimiter);
		}

		fflush(fd);
	}
	if(tposeQuery->aggregateType == TPOSE_IO_AGGREGATION_AVG) {
		for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
			if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
				fprintf(fd, "%.2f%c", aggregator->avgs[i], rowDelimiter);
			else
				fprintf(fd, "%.2f%c", aggregator->avgs[i], fieldDelimiter);
		}

		fflush(fd);
	}

}
