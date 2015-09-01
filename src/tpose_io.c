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
//#include "btree.h"

char* rowDelimiter = "\n";
extern int errno;

/** 
 ** Return the integer index of the field parameter 
 ** returns -1 if field passed doesn't match any of the header fields
 **/
int getFieldIndex(
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

//	printf("getFieldIndex numFields = %d\n", numFields);

	int i;
	for(i = 0; i < numFields; i++) {
		if(!strcmp(field, *(tposeHeader->fields+i))) {
//			printf("i = %d\n", i);
			break;
		}
	}

//	printf("broken out @ i = %d\n", i);

	return i; 
	
}



/** 
 ** Allocates memory for the input file
 **/
TposeInputFile* tposeIOInputFileAlloc(
	int fd
	,char* fileAddr
	,size_t fileSize
	,unsigned char fieldDelimiter
) {

	TposeInputFile* inputFile;

	/* Allocate memory for the field names */
	if((inputFile = (TposeInputFile*) malloc(sizeof(TposeInputFile))) == NULL ) {
		printf("tposeIOFileAlloc: malloc error\n");
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
	//assert(tposeFile->fieldDelimiter != '');

	return inputFile;
	
}



/** 
 ** Free memory for a TposeInputFile 
 **/
void tposeIOInputFileFree(
    TposeInputFile** inputFilePtr
) {

	//printf("tposeIOFileFree: Freeing TposeInputFile memory...\n");

	tposeIOHeaderFree(&((*inputFilePtr)->fileHeader));
    
   if(*inputFilePtr != NULL) {
       free(*inputFilePtr);
       *inputFilePtr = NULL;
   }

   assert(*inputFilePtr == NULL);
   //printf("tposeIOFileFree: TposeInputFile has been freed!\n");
}



/** 
 ** Allocates memory for the output file
 **/
TposeOutputFile* tposeIOOutputFileAlloc(
	//int fd
	FILE* fd
	,unsigned char fieldDelimiter
) {

	TposeOutputFile* outputFile;

	/* Allocate memory for the field names */
	if((outputFile = (TposeOutputFile*) malloc(sizeof(TposeOutputFile))) == NULL ) {
		printf("tposeIOOutputFileAlloc: malloc error\n");
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
	//printf("tposeIOFileFree: Freeing TposeInputFile memory...\n");

	if( (*outputFilePtr)->fileIdHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileIdHeader));

	if( (*outputFilePtr)->fileGroupHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileGroupHeader));
    
   if(*outputFilePtr != NULL) {
       free(*outputFilePtr);
       *outputFilePtr = NULL;
   }

   assert(*outputFilePtr == NULL);
   //printf("tposeIOFileFree: TposeInputFile has been freed!\n");
}



/** 
 ** Allocates memory for the input file headers arrays 
 **/
TposeHeader* tposeIOHeaderAlloc(
	unsigned int maxFields
) {
	//printf("tposeIOHeaderAlloc: numFields = %u\n", numFields);

	TposeHeader* tposeHeader;

	/* Allocate memory for the field names */
	if((tposeHeader = (TposeHeader*) malloc(sizeof(TposeHeader))) == NULL ) {
		printf("tposeIOHeaderAlloc: malloc error\n");
		return NULL;
	}

	if((tposeHeader->fields = (char**) malloc(maxFields * sizeof(char*))) == NULL ) {
		printf("tposeIOHeaderAlloc: malloc error\n");
		return NULL;
	}

	tposeHeader->maxFields = maxFields;
	tposeHeader->numFields = 0;
	
	assert(tposeHeader->fields != NULL);
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

	//printf("tposeIOHeaderFree: Freeing TposeHeader memory...\n");

/*	printf("field: %s\n", *(*tposeHeaderPtr)->fields);
	printf("field: %s\n", *((*tposeHeaderPtr)->fields+1));
	printf("field: %s\n", (*tposeHeaderPtr)->fields[2]);
	printf("field: %s\n", (*tposeHeaderPtr)->fields[3]);
*/

	int field;
	if((*tposeHeaderPtr)->fields != NULL) {
		for(field = 0; field < (*tposeHeaderPtr)->maxFields; field++) {
			free((*tposeHeaderPtr)->fields[field]);
		} 

		free((*tposeHeaderPtr)->fields);
		(*tposeHeaderPtr)->fields = NULL;
	}
    
    assert((*tposeHeaderPtr)->fields == NULL);
    //printf("tposeIOHeaderFree: TposeHeader variables have been freed!\n");

    if(*tposeHeaderPtr != NULL) {
        free(*tposeHeaderPtr);
        *tposeHeaderPtr = NULL;
    }

    assert(*tposeHeaderPtr == NULL);
    //printf("tposeIOHeaderFree: TposeHeader has been freed!\n");
    
}



/** 
 ** Aggregator for each group/id 
 **/
TposeAggregator* tposeIOAggregatorAlloc(unsigned int numFields)
{

	TposeAggregator* tposeAggregator;

	/* Allocate memory for the floating-point numeric variables being transposed */
	if((tposeAggregator = (TposeAggregator*) calloc(1, sizeof(TposeAggregator))) == NULL ) {
		printf("tposeIOAggregatorAlloc: calloc error\n");
		return NULL;
	}

	if((tposeAggregator->aggregates = (double*) calloc(numFields, sizeof(double))) == NULL ) {
		printf("tposeIOAggregatorAlloc: calloc error\n");
		return NULL;
	}

	tposeAggregator->numFields = numFields;
	
	assert(tposeAggregator->aggregates != NULL);
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
		//for(value = 0; value < (*tposeAggregatorPtr)->numFields; value++) {
			//free((*tposeAggregatorPtr)->aggregates[value]);
		//} 

		free((*tposeAggregatorPtr)->aggregates);
		(*tposeAggregatorPtr)->aggregates = NULL;
	}
    
    assert((*tposeAggregatorPtr)->aggregates == NULL);

    if(*tposeAggregatorPtr != NULL) {
        free(*tposeAggregatorPtr);
        *tposeAggregatorPtr = NULL;
    }

    assert(*tposeAggregatorPtr == NULL);
    
}



/** 
 ** Allocates memory for the transpose parameters 
 **/
TposeQuery* tposeIOQueryAlloc(
	TposeInputFile* inputFile
	,TposeOutputFile* outputFile
	,char* idVar
	,char* groupVar
	,char* numericVar
) {

	TposeQuery* tposeQuery;

	/* Allocate memory for the query parameters */
	if((tposeQuery = (TposeQuery*) malloc(sizeof(TposeQuery))) == NULL ) {
		printf("tposeIOQueryAlloc: malloc error\n");
		return NULL;
	}
	
	tposeQuery->inputFile = inputFile;
	tposeQuery->outputFile = outputFile;
	tposeQuery->aggregator = NULL;
	tposeQuery->id = getFieldIndex(inputFile->fileHeader, idVar);
	tposeQuery->group = getFieldIndex(inputFile->fileHeader, groupVar);
	tposeQuery->numeric = getFieldIndex(inputFile->fileHeader, numericVar);

	/*printf("*** Initialised query id = %d\n", tposeQuery->id);
	printf("*** Initialised query group = %d\n", tposeQuery->group);
	printf("*** Initialised query numeric = %d\n", tposeQuery->numeric);*/

	return tposeQuery;
	
}



/** 
 ** Free memory for a TposeQuery 
 **/
void tposeIOQueryFree(
    TposeQuery** tposeQueryPtr
) {

	//printf("tposeIOQueryFree: Freeing TposeQuery memory...\n");
	tposeIOAggregatorFree( &((*tposeQueryPtr)->aggregator) );
    
	// No need to free the inputFile, as this is done in the tposeIOCloseFile() call
	// No need to free the outputFile, as this is done in the tposeIOCloseFile() call
   if(*tposeQueryPtr != NULL) { 
       free(*tposeQueryPtr);
       *tposeQueryPtr = NULL;
   }

   assert(*tposeQueryPtr == NULL);
   //printf("tposeIOQueryFree: TposeQuery has been freed!\n");
}



/** 
 ** Open input file
 **/
TposeInputFile* tposeIOOpenInputFile(
	char* filePath
	,unsigned char fieldDelimiter
) {

	int fd;
	char* fileAddr;
	size_t fileSize;
	struct stat statBuffer;

	if((fd = open(filePath, O_RDONLY)) < 0) {
		printf("Error: tposeIOOpenInputFile - can not open file %s\n", filePath);
		return NULL;
	}

	if(fstat(fd, &statBuffer) < 0) {
		printf("Error: tposeIOOpenInputFile- can not stat input file %s\n", filePath);
		return NULL;
	}

	fileSize = statBuffer.st_size;
	//pageSize = (size_t) sysconf (_SC_PAGESIZE); // Get page size

	if((fileAddr = mmap(0, statBuffer.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED ) {
		printf("Error: tposeIOOpenFile - can not map inputfile %s\n", filePath);
		return NULL;
	}

	// might speed-up via aggressive read-ahead caching
	if(madvise(fileAddr, fileSize, MADV_SEQUENTIAL) == -1) {
		printf("Warning: tposeIOOpenFile - cannot advise kernel on file %s\n", filePath);
	}

	//printf("tposeIOOpenFile opened file %s\n", filePath);

	TposeInputFile* inputFile = tposeIOInputFileAlloc(fd, fileAddr, fileSize, fieldDelimiter); // Creates the file handle 
	inputFile->fileHeader = tposeIOReadInputHeader(inputFile); // Opening a file also creates the TposeHeader struct

	return inputFile;

}



/** 
 ** Close an input file and unmap any memory
 **/
int tposeIOCloseInputFile(
	TposeInputFile* tposeFile
) {

   if((munmap(tposeFile->fileAddr, tposeFile->fileSize)) < 0) {
       printf("Error: tposeIOCloseInputFile - can not unmap file\n");
       return -1;
   }

   if(close(tposeFile->fd) < 0) {
       printf("Error: tposeIOCloseInputFile - can not close file\n");
       return -1;
   }

	// Free TposeInputFile memory
	tposeIOInputFileFree(&tposeFile);
    
	return 0;

}



/** 
 ** Open output file
 **/
TposeOutputFile* tposeIOOpenOutputFile(
	char* filePath
	,unsigned char fieldDelimiter
) {

	//int fd;
	FILE* fd;
	if(strcmp(filePath, "stdout")) {
		//if((fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND )) < 0) {
		if((fd = fopen(filePath, "wa" )) == NULL) {
			printf("Error: tposeIOOpenOutputFile - can not open output file %s\n", filePath);
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

	if(outputFile->fd != stdout) { // Close if not standard output 
		if(ferror(outputFile->fd)) // Check for errors
			fprintf(stderr, "tposeIOCloseOutputFile(): errno = %d - %s\n", errno, strerror(errno));
		fclose(outputFile->fd);
	}

	// Free TposeOutputFile memory
	tposeIOOutputFileFree(&outputFile);
    
	return 0;

}



/** 
 ** Reads the first line (header) of an input file
 **/
TposeHeader* tposeIOReadInputHeader(
	TposeInputFile* inputFile
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
	while( *(inputFile->fileAddr+length) != *rowDelimiter) {
		if( *(inputFile->fileAddr+length) == (inputFile->fieldDelimiter))
			fieldCount++; length++;
	}
	
	if(fieldCount > 0)
		fieldCount++; // Quick hack to get real number of fields

//	printf("tposeIOReadInputHeader(): fieldCount = %u\n", fieldCount);
	TposeHeader* header = tposeIOHeaderAlloc(fieldCount); // Allocate the needed memory

	// Find index of first row delimiter
	inputFile->dataAddr = strchr(inputFile->fileAddr, *rowDelimiter);
	*inputFile->dataAddr = '\0';
	inputFile->dataAddr+=1; // Make sure we're not pointing at the NULL 
	//printf("*** TEST: %s\n", tposeFile->fileAddr);
	//printf("*** TEST: \n%s\n", tposeFile->dataAddr);

	// Create a copy of the NULL terminated string (as strsep/strtok_r modifies this)
	rowtok = strdup(inputFile->fileAddr);
	
	// Read header fields
	fieldtok = strtok_r(rowtok, &(inputFile->fieldDelimiter), &fieldSavePtr);
	if(fieldtok == NULL) return NULL;
	tempString = malloc(strlen(fieldtok) * sizeof(char));
	strcpy(tempString, fieldtok);
	*(header->fields) = tempString;
	for(fieldCount = 1; (fieldtok = strtok_r(NULL, &(inputFile->fieldDelimiter), &fieldSavePtr)) != NULL; ) {
		tempString = malloc(strlen(fieldtok) * sizeof(char));
		strcpy(tempString, fieldtok);
		*(header->fields+(fieldCount++)) = tempString;
	}
	
	// Clean-up
	free(rowtok);

	return header;

}



/** 
 ** Returns a unique list of GROUP variable values 
 **/
/* TposeHeader**/ void tposeIOgetUniqueGroups(
	TposeQuery* tposeQuery
	,BTree* btree
) {

	char* fieldSavePtr;
	char tempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* allocString;

	TposeHeader* header = tposeIOHeaderAlloc(TPOSE_IO_MAX_FIELDS); // Allocate the needed memory
	BTreeKey* key = btreeKeyAlloc();
	BTreeKey* resultKey;
	
	unsigned int rowCount = 2; // For debugging only
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned int uniqueGroupCount = 0; // Used to index array of header ptrs
	unsigned int groupCharCount = 0; 
	unsigned int hashCharCount = 0; 
	unsigned int totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	unsigned long hashValue = 0; 
//	const unsigned long hashMult = 37; 

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)

	/*printf("sizeof(unsigned long) = %u bytes\n", sizeof(unsigned long));
	printf("sizeof(unsigned long long) = %u bytes\n", sizeof(unsigned long long)); */

	//printf("\n");
	while(totalCharCount <= (tposeQuery->inputFile)->fileSize) {
	//while((*fieldSavePtr != EOF) && (fieldSavePtr != NULL)) {

		if(*fieldSavePtr == '\t') {
			++fieldCount;
			++fieldSavePtr;
			++totalCharCount;
			continue;
		}

		if(*fieldSavePtr == '\n') {
			fieldCount = 0;
			++rowCount;
			++fieldSavePtr;
			++totalCharCount;
			continue;
		}
		
		// Check if = group field
		if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

			// Copy field value
			while(*fieldSavePtr != '\t' && *fieldSavePtr != '\n') {
				tempString[groupCharCount++] = *fieldSavePtr++;
				++totalCharCount;
			}
			tempString[groupCharCount] = '\0'; // Null-terminate string

			// Convert char array to hash
			for(hashCharCount = 0; hashCharCount <= groupCharCount-1; ++hashCharCount)
				hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) tempString[hashCharCount];

			// Insert into btree
			if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) == NULL) {
				/*printf("\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
				printf("New group value found = '"); */
				
				// Check for collisions - print only when we add a unique group
//				printf("ROW:%u = %s\t%ld\t%u\n", rowCount, tempString, hashValue, uniqueGroupCount);

				btreeSetKeyValue(key, hashValue, uniqueGroupCount, 0);
				if(btreeInsert(btree, key) == -1) {
					printf("tposeIOgetUniqueGroups: btreeInsert error\n");
				}

				// Insert into TposeHeader object
				//printf("length of tempString = %d\n", strlen(tempString));
				allocString = malloc(strlen(tempString) * sizeof(char));
				strcpy(allocString, tempString);
				//printf("allocString = %s\n", allocString);
				//printf("uniqueGroupCount = %d\n", uniqueGroupCount);
				*(header->fields+(uniqueGroupCount++)) = allocString;
				header->numFields = uniqueGroupCount; // Update number of fields in header
				/*printf("BTREE\n");
				btreeForEach(btree, btreeCBPrintNode);
				printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");*/
			}

			// Reset variables (
			groupCharCount = 0;
			hashValue = 0;
			
			if(*fieldSavePtr == '\t') {
				--fieldSavePtr;
				--totalCharCount;
			}

			if(*fieldSavePtr == '\n') {
				--fieldSavePtr;
				--totalCharCount;
			}
		}

		// If not a delimiter or the group field, then ++
		++fieldSavePtr;
		++totalCharCount;

	}

	btreeKeyFree(&key);
	
	/*printf("\n");
	printf("uniqueGroupCount = %d\n", uniqueGroupCount);
	printf("header->numFields = %u\n", header->numFields);
	btreeForEach(btree, btreeCBPrintNode); 
	
	int i;
	for(i = 0; i < uniqueGroupCount; ++i)
		printf("group %d = %s\n", i, header->fields[i]); */
	(tposeQuery->outputFile)->fileGroupHeader = header;
	//return header;
	
}



/** 
 ** Returns a unique list of GROUP variable values 
 **/
void tposeIOTransposeGroup(
	TposeQuery* tposeQuery
	,BTree* btree
) {

	/* printf("tposeQuery->id = %d\n", tposeQuery->id);
	printf("tposeQuery->group = %d\n", tposeQuery->group);
	printf("tposeQuery->numeric = %d\n", tposeQuery->numeric);
	printf("outputHeader->numFields = %d\n", outputHeader->numFields);
	printf("outputHeader->fields[0] = %s\n", outputHeader->fields[0]);
	printf("aggregator->numFields = %d\n", aggregator->numFields);
	printf("aggregator->aggregates[0] = %f\n", aggregator->aggregates[0]); */

	//printf("fileAddr = \n%s\n", (tposeQuery->inputFile)->fileAddr);
	//printf("dataAddr = \n%s\n", (tposeQuery->inputFile)->dataAddr);
	
	char* fieldSavePtr;
	char groupTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char numericTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)
	unsigned int numericFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)
	//char* allocString;

	//BTreeKey* key = btreeKeyAlloc();
	tposeQuery->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields);
	BTreeKey* resultKey;

	unsigned int fieldCharCount = 0; 
	unsigned int hashCharCount = 0; 
	unsigned int totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned long hashValue = 0; 
//	const unsigned long hashMult = 37; 

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)


	printf("\n");
	while(totalCharCount <= (tposeQuery->inputFile)->fileSize) {
	//while(*fieldSavePtr != EOF) {

		if(*fieldSavePtr == '\t') {
			++fieldCount;
			++fieldSavePtr;
			++totalCharCount;
			continue;
		}

		if(*fieldSavePtr == '\n') {
			// Aggregate value for each group
			if((groupFoundFlag == 1) && (numericFoundFlag == 1))
				(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);
				
			// Reset flags for next row
			groupFoundFlag = 0;
			numericFoundFlag = 0;

			fieldCount = 0;
			++fieldSavePtr;
			++totalCharCount;
			continue;

		}
		
		// Track current group field
		if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

			// Get group field value
			while(*fieldSavePtr != '\t' && *fieldSavePtr != '\n') {
				groupTempString[fieldCharCount++] = *fieldSavePtr++;
				++totalCharCount;
			}
			groupTempString[fieldCharCount] = '\0'; // Null-terminate string

			// Convert char array to hash
			for(hashCharCount = 0; hashCharCount <= fieldCharCount-1; ++hashCharCount)
				hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) groupTempString[hashCharCount];

			// Insert into btree
			if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) != NULL) {
				
				// Flag group field as found
				groupFoundFlag = 1;
				groupFieldIndex = resultKey->dataOffset;

			}

			// Reset variables
			fieldCharCount = 0;
			hashValue = 0;
			
			if(*fieldSavePtr == '\t') {
				--fieldSavePtr;
				--totalCharCount;
			}

			if(*fieldSavePtr == '\n') {
				--fieldSavePtr;
				--totalCharCount;
			}
		}
		

		// Aggregate numeric field for given group
		if(fieldCount == (tposeQuery->numeric))  { // if group value is empty string we ignore
			
			// Copy field value
			while(*fieldSavePtr != '\t' && *fieldSavePtr != '\n') {
				numericTempString[fieldCharCount++] = *fieldSavePtr++;
				++totalCharCount;
			}
			numericTempString[fieldCharCount] = '\0'; // Null-terminate string

			// Flag numeric field as found
			numericFoundFlag = 1;

			// Reset variables
			fieldCharCount = 0;

			if(*fieldSavePtr == '\t') {
				--totalCharCount;
				--fieldSavePtr;
			}

			if(*fieldSavePtr == '\n') {
				--fieldSavePtr;
				--totalCharCount;
			}
		}

		// If not a delimiter or the group field, then ++
		++fieldSavePtr;
		++totalCharCount;

	}

}



/** 
 ** 
 **/
void tposeIOPrintOutput(
	TposeQuery* tposeQuery
) {

	int i;

	// Group Header
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == (((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1))
			fprintf((tposeQuery->outputFile)->fd, "%s\n", ((tposeQuery->outputFile)->fileGroupHeader)->fields[i]);
		else
			fprintf((tposeQuery->outputFile)->fd, "%s\t", ((tposeQuery->outputFile)->fileGroupHeader)->fields[i]);
	}

	// Aggregates
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
			fprintf((tposeQuery->outputFile)->fd, "%.2f\n", (tposeQuery->aggregator)->aggregates[i]);
		else
			fprintf((tposeQuery->outputFile)->fd, "%.2f\t", (tposeQuery->aggregator)->aggregates[i]);
	}

	fflush((tposeQuery->outputFile)->fd);

}


