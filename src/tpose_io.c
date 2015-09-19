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

	/* Allocate memory for the field names */
	if((inputFile = (TposeInputFile*) malloc(sizeof(TposeInputFile))) == NULL ) {
		printf("tposeIOFileAlloc(): malloc error\n");
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
	
	debug_print("tposeIOInputFileAlloc(): fileAddr = \n%s\n", inputFile->fileAddr);

	return inputFile;
	
}



/** 
 ** Free memory for a TposeInputFile 
 **/
void tposeIOInputFileFree(
    TposeInputFile** inputFilePtr
) {

	debug_print("tposeIOFileFree(): Freeing TposeInputFile memory...\n");

	tposeIOHeaderFree(&((*inputFilePtr)->fileHeader));
    
   if(*inputFilePtr != NULL) {
       free(*inputFilePtr);
       *inputFilePtr = NULL;
   }

   assert(*inputFilePtr == NULL);

   debug_print("tposeIOFileFree(): TposeInputFile has been freed!\n");
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
		printf("tposeIOOutputFileAlloc(): malloc error\n");
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

	debug_print("tposeIOFileFree(): Freeing TposeInputFile memory...\n");

	if( (*outputFilePtr)->fileIdHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileIdHeader));

	if( (*outputFilePtr)->fileGroupHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileGroupHeader));
    
   if(*outputFilePtr != NULL) {
       free(*outputFilePtr);
       *outputFilePtr = NULL;
   }

   assert(*outputFilePtr == NULL);
   
   debug_print("tposeIOFileFree(): TposeInputFile has been freed!\n");
}



/** 
 ** Allocates memory for the input file headers arrays 
 **/
TposeHeader* tposeIOHeaderAlloc(
	unsigned int maxFields
	,unsigned int mutateHeader
) {
	
	debug_print("tposeIOHeaderAlloc(): Allocating memory for TposeHeader with maxFields = %u\n", maxFields);

	TposeHeader* tposeHeader;

	/* Allocate memory for the field names */
	if((tposeHeader = (TposeHeader*) malloc(sizeof(TposeHeader))) == NULL ) {
		printf("tposeIOHeaderAlloc(): malloc error\n");
		return NULL;
	}

	if(mutateHeader) {
		if((tposeHeader->fields = (char**) malloc(maxFields * sizeof(char*))) == NULL ) {
			printf("tposeIOHeaderAlloc(): malloc error\n");
			return NULL;
		}
	}
	else {
		tposeHeader->fields = NULL;
	}

	tposeHeader->maxFields = maxFields;
	tposeHeader->numFields = 0;
	
	assert(tposeHeader->maxFields != 0);
	assert(tposeHeader->numFields == 0);

	debug_print("tposeIOHeaderAlloc(): TposeHeader memory allocated\n");

	return tposeHeader;
	
}



/** 
 ** Free memory for a TposeHeader 
 **/
void tposeIOHeaderFree(
    TposeHeader** tposeHeaderPtr
) {

	debug_print("tposeIOHeaderFree(): Freeing TposeHeader memory...\n");

	int field;
	if((*tposeHeaderPtr)->fields != NULL) {
		for(field = 0; field < (*tposeHeaderPtr)->maxFields; field++) {
			free((*tposeHeaderPtr)->fields[field]);
		} 

		free((*tposeHeaderPtr)->fields);
		(*tposeHeaderPtr)->fields = NULL;
	}
    
	assert((*tposeHeaderPtr)->fields == NULL);
	debug_print("tposeIOHeaderFree(): TposeHeader variables have been freed!\n");

	if(*tposeHeaderPtr != NULL) {
		free(*tposeHeaderPtr);
		*tposeHeaderPtr = NULL;
	}

	assert(*tposeHeaderPtr == NULL);

	debug_print("tposeIOHeaderFree(): TposeHeader has been freed!\n");
    
}



/** 
 ** Aggregator for each group/id 
 **/
TposeAggregator* tposeIOAggregatorAlloc(unsigned int numFields)
{

	debug_print("tposeIOAggregatorAlloc(): Allocating memory for TposeAggregator\n");

	TposeAggregator* tposeAggregator;

	/* Allocate memory for the floating-point numeric variables being transposed */
	if((tposeAggregator = (TposeAggregator*) calloc(1, sizeof(TposeAggregator))) == NULL ) {
		printf("tposeIOAggregatorAlloc(): calloc error\n");
		return NULL;
	}

	if((tposeAggregator->aggregates = (double*) calloc(numFields, sizeof(double))) == NULL ) {
		printf("tposeIOAggregatorAlloc(): calloc error\n");
		return NULL;
	}

	tposeAggregator->numFields = numFields;
	
	assert(tposeAggregator->aggregates != NULL);
	assert(tposeAggregator->numFields != 0);

	debug_print("tposeIOAggregatorAlloc(): TposeAggregator memory allocated\n");

	return tposeAggregator;
	
}



/** 
 ** Free memory for a TposeAggregator
 **/
void tposeIOAggregatorFree(
    TposeAggregator** tposeAggregatorPtr
) {

	debug_print("tposeIOAggregatorFree(): Freeing TposeAggregator memory...\n");

	int value;
	if((*tposeAggregatorPtr)->aggregates != NULL) {
		free((*tposeAggregatorPtr)->aggregates);
		(*tposeAggregatorPtr)->aggregates = NULL;
	}
    
    assert((*tposeAggregatorPtr)->aggregates == NULL);

    if(*tposeAggregatorPtr != NULL) {
        free(*tposeAggregatorPtr);
        *tposeAggregatorPtr = NULL;
    }

	assert(*tposeAggregatorPtr == NULL);

	debug_print("tposeIOAggregatorFree(): TposeAggregator has been freed!\n");
    
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
) {

	debug_print("tposeIOQueryAlloc(): Allocating memory for TposeQuery\n");

	// Check parameters passed
	if((idVar != NULL) && (tposeIOGetFieldIndex(inputFile->fileHeader, idVar) == -1)) return NULL;
	if((groupVar != NULL) && (tposeIOGetFieldIndex(inputFile->fileHeader, groupVar) == -1)) return NULL;
	if((numericVar != NULL) && (tposeIOGetFieldIndex(inputFile->fileHeader, numericVar) == -1)) return NULL;

	TposeQuery* tposeQuery;

	/* Allocate memory for the query parameters */
	if((tposeQuery = (TposeQuery*) malloc(sizeof(TposeQuery))) == NULL ) {
		printf("tposeIOQueryAlloc: malloc error\n");
		exit(EXIT_FAILURE);
	}
	
	tposeQuery->inputFile = inputFile;
	tposeQuery->outputFile = outputFile;
	tposeQuery->aggregator = NULL;
	tposeQuery->id = -1;
	tposeQuery->group = -1;
	tposeQuery->numeric = -1;
	if(idVar != NULL) tposeQuery->id = tposeIOGetFieldIndex(inputFile->fileHeader, idVar);
	if(groupVar != NULL) tposeQuery->group = tposeIOGetFieldIndex(inputFile->fileHeader, groupVar);
	if(numericVar != NULL) tposeQuery->numeric = tposeIOGetFieldIndex(inputFile->fileHeader, numericVar);

	debug_print("tposeIOQueryAlloc(): id = %d\n", tposeQuery->id);
	debug_print("tposeIOQueryAlloc(): group = %d\n", tposeQuery->group);
	debug_print("tposeIOQueryAlloc(): numeric = %d\n", tposeQuery->numeric);

	debug_print("tposeIOQueryAlloc(): TposeQuery memory allocated\n");

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
) {

	debug_print("tposeIOQueryIndexedAlloc(): Allocating memory for TposeQuery\n");

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

	TposeQuery* tposeQuery;
	/* Allocate memory for the query parameters */
	if((tposeQuery = (TposeQuery*) malloc(sizeof(TposeQuery))) == NULL ) {
		printf("tposeIOQueryAlloc: malloc error\n");
		exit(EXIT_FAILURE);
	}
	
	tposeQuery->inputFile = inputFile;
	tposeQuery->outputFile = outputFile;
	tposeQuery->aggregator = NULL;
	if(idVar != -1) tposeQuery->id = idVar;
	if(groupVar != -1) tposeQuery->group = groupVar;
	if(numericVar != -1) tposeQuery->numeric = numericVar;

	debug_print("tposeIOQueryIndexedAlloc(): TposeQuery memory allocated\n");

	return tposeQuery;
	
}



/** 
 ** Free memory for a TposeQuery 
 **/
void tposeIOQueryFree(
    TposeQuery** tposeQueryPtr
) {

	debug_print("tposeIOQueryFree: Freeing TposeQuery memory...\n");

	if((*tposeQueryPtr)->aggregator != NULL) tposeIOAggregatorFree( &((*tposeQueryPtr)->aggregator) );
    
	// No need to free the inputFile, as this is done in the tposeIOCloseFile() call
	// No need to free the outputFile, as this is done in the tposeIOCloseFile() call
   if(*tposeQueryPtr != NULL) { 
       free(*tposeQueryPtr);
       *tposeQueryPtr = NULL;
   }

   assert(*tposeQueryPtr == NULL);
   
   debug_print("tposeIOQueryFree: TposeQuery has been freed!\n");

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
		printf("Error: tposeIOOpenInputFile - can not open file %s\n", filePath);
		return NULL;
	}

	if(fstat(fd, &statBuffer) < 0) {
		printf("Error: tposeIOOpenInputFile- can not stat input file %s\n", filePath);
		return NULL;
	}

	fileSize = statBuffer.st_size;

	if((fileAddr = mmap(0, statBuffer.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED ) {
		printf("Error: tposeIOOpenInputFile - can not map inputfile %s\n", filePath);
		return NULL;
	}

	// might speed-up via aggressive read-ahead caching
	if(madvise(fileAddr, fileSize, MADV_SEQUENTIAL) == -1) {
		printf("Warning: tposeIOOpenInputFile - cannot advise kernel on file %s\n", filePath);
	}


	debug_print("tposeIOOpenInputFile(): mutateHeader for input file? =  %u\n", mutateHeader);
	TposeInputFile* inputFile = tposeIOInputFileAlloc(fd, fileAddr, fileSize, fieldDelimiter); // Creates the file handle 
	inputFile->fileHeader = tposeIOReadInputHeader(inputFile, mutateHeader); // Opening a file also creates the TposeHeader struct
	debug_print("tposeIOOpenInputFile(): Input file opened!  %s\n", filePath);
	return inputFile;

}



/** 
 ** Close an input file and unmap any memory
 **/
int tposeIOCloseInputFile(
	TposeInputFile* inputFile
) {

   if((munmap(inputFile->fileAddr, inputFile->fileSize)) < 0) {
       printf("Error: tposeIOCloseInputFile - can not unmap file\n");
       return -1;
   }

   if(close(inputFile->fd) < 0) {
       printf("Error: tposeIOCloseInputFile - can not close file\n");
       return -1;
   }

	// Free TposeInputFile memory
	tposeIOInputFileFree(&inputFile);

	debug_print("tposeIOCloseInputFile(): Input file closed!\n");
    
	return 0;

}



/** 
 ** Open output file
 **/
TposeOutputFile* tposeIOOpenOutputFile(
	char* filePath
	,unsigned char fieldDelimiter
) {

	FILE* fd;
	if(strcmp(filePath, "stdout")) {
		if((fd = fopen(filePath, "wa" )) == NULL) {
			printf("Error: tposeIOOpenOutputFile - can not open output file %s\n", filePath);
			return NULL;
		}
	}
	else
		fd = stdout; // If no output file is passed, print to std output

	debug_print("tposeIOOpenOutputFile(): Output file opened! %s\n", filePath);
	
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
			fprintf(stderr, "tposeIOCloseOutputFile(): Error closing output file. Error number = %d - %s\n", errno, strerror(errno));
		fclose(outputFile->fd);
	}

	// Free TposeOutputFile memory
	tposeIOOutputFileFree(&outputFile);

	debug_print("tposeIOCloseOutputFile(): Output file closed!\n");
    
	return 0;

}



/** 
 ** Reads the first line (header) of an input file
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

	debug_print("tposeIOReadInputHeader(): Input file header read!\n");

	return header;

}



void tposeIOBuildPartitions(
	TposeQuery* tposeQuery
) {

	extern unsigned int fileChunks; // Number of file chunks

	char* fieldSavePtr; // Points at start of each field after every loop
	off_t fileSize = (tposeQuery->inputFile)->fileSize;
	off_t rFileSize = fileSize; // Remaining file size

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)

	// Calculate file chunks
	off_t prepartitions[1000];
	int t=0, i=0, j=0;
	prepartitions[0] = fileSize;
	while(rFileSize >= TPOSE_IO_CHUNK_SIZE){
		rFileSize -= TPOSE_IO_CHUNK_SIZE;
		prepartitions[fileChunks++] = rFileSize;
	}
	prepartitions[fileChunks] = 0;
	
	// Reverse partitions to ease processing
	for(t=fileChunks; t>=0; t--)
		partitions[i++] = prepartitions[t];

	// Correct partitions to start after new lines
	char* partSavePtr;
	off_t offset;
	for(t=1; t<fileChunks; t++) { // Don't need to modify first partition (0)
		offset = 0;
		partSavePtr = fieldSavePtr + partitions[t];
		while(*(partSavePtr++) != '\n') {
			++offset;
		}
		++offset;
		partitions[t] += offset;
	}

	// Test new partitions TODO: Remove
	for(t=1; t<fileChunks; t++) { // Don't need to modify first partition (0)
		printf("Thread: %d\t", t);
		partSavePtr = fieldSavePtr + partitions[t];
		while(*(partSavePtr) != '\t') {
			printf("%c", *partSavePtr);
			++partSavePtr;
		}
		++partSavePtr;
		printf("\t");
		while(*(partSavePtr) != '\t') {
			printf("%c", *partSavePtr);
			++partSavePtr;
		}
		printf("\n");
	}
	
	// Print updated partitions
	/*for(i=0; i<=fileChunks; i++) {
		printf("partitions[%u] = %u\n", i, partitions[i]);
	}*/

	printf("Parallelize(): Chunks = %d\n", fileChunks);

}



void tposeIOUniqueGroupsParallel(
	TposeQuery* tposeQuery
) {

	extern unsigned int fileChunks; // Number of file chunks

	unsigned int mutateHeader = 1; // Allow for header row to be modified
	pthread_t threads[fileChunks]; // Thread array
	int *taskids[fileChunks]; // Array of thread Id's
	int rc;
	int t=0, i=0, j=0;

	// Allocate memory for threadDataArray (one for each file chunk)
	if((threadDataArray = (TposeThreadData**) calloc(1, fileChunks * sizeof(TposeThreadData*))) == NULL ) {
		printf("tposeIOParallelize(): calloc error\n");
		return NULL;
	}

	// Create threads
	for(t=0; t<fileChunks; t++) {

		if((threadData = (TposeThreadData*) calloc(1, sizeof(TposeThreadData))) == NULL ) {
			printf("tposeIOParallelize(): calloc error\n");
			return NULL;
		}
		
		// Assign arguments to current thread
		threadData->threadId = t;
		threadData->query = tposeQuery; 
		threadData->header = (TposeHeader*) tposeIOHeaderAlloc(TPOSE_IO_MAX_FIELDS, mutateHeader); // Allocate the needed memory
		threadDataArray[t] = threadData; 

		// Map input to threads
		rc = pthread_create(&threads[t], NULL, tposeIOUniqueGroupsMap, (void *) threadDataArray[t]);
		if (rc) {
			printf("tposeIOParallelize(): pthread_create error! Return code = %d\n", rc);
			exit(-1);
		}
	}
	
	// Sync threads
	for(t=0; t<fileChunks; t++)
		(void) pthread_join(threads[t], NULL);

	// Reduce output header
	tposeIOUniqueGroupsReduce(tposeQuery);

	// Clean-up
	for(t=0; t<fileChunks; t++) {
		free(threadDataArray[t]);
	}
	free(threadDataArray);

}



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
	BTree* btree = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
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

				//printf("%d : field[%d] = %s\n", threadCtr, fieldCtr, (threadDataArray[threadCtr]->header)->fields[fieldCtr]);

				charSavePtr = (threadDataArray[threadCtr]->header)->fields[fieldCtr];
				
				// Copy field value
				while(*charSavePtr != '\0') {
					tempString[groupCharCount++] = *charSavePtr++;
				}
				tempString[groupCharCount] = '\0'; // Null-terminate string
				//printf("%d : tempString = %s\n", threadCtr, tempString);

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= groupCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) tempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) == NULL) {

					btreeSetKeyValue(key, hashValue, uniqueGroupCount, 0);
					if(btreeInsert(btree, key) == -1) {
						printf("tposeIOUniqueGroupsReduce(): btreeInsert error\n");
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

	
	header->numFields = uniqueGroupCount;
	//printf("number of unique groups = %u\n", header->numFields);

	/*printf("\n");
	for(fieldCtr = 0; fieldCtr < header->numFields; fieldCtr++) {
		printf("unique groups [%u]  = %s\n", fieldCtr, header->fields[fieldCtr]);
	}*/

	btreeFree(&btree);
	
	// Assign groups to output file header 
	(tposeQuery->outputFile)->fileGroupHeader = header; 

}



void* tposeIOUniqueGroupsMap(
	void* threadArg
) {
	
	TposeThreadData* threadData = (TposeThreadData*) threadArg;

	TposeQuery* tposeQuery = (TposeQuery*) threadData->query;
	TposeHeader* header = (TposeHeader*) threadData->header;
	unsigned int threadId = (unsigned int) threadData->threadId;
	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	//printf("%u : Reporting for duty!\n", threadId);

	//sleep(1);
	//printf("**Thread %u: fileDelimiter='%c'\n", threadId, fieldDelimiter);
	//printf("**Thread %u: partition start = %u, end = %u\n", threadId, partitions[threadId], partitions[threadId+1]);
	off_t partitionStart = partitions[threadId];
	off_t partitionEnd = partitions[threadId+1];
	off_t partitionCharLimit = partitionEnd - partitionStart;

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


	fieldSavePtr = ((tposeQuery->inputFile)->dataAddr) + partitionStart; // Init with ptr to second row (where data starts)

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
				tempString[groupCharCount] = '\0'; // Null-terminate string

				// Convert char array to hash
				for(hashCharCount = 0; hashCharCount <= groupCharCount-1; ++hashCharCount)
					hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) tempString[hashCharCount];

				// Insert into btree
				if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) == NULL) {

					// Check for collisions - print only when we add a unique group
					debug_print("tposeIOgetUniqueGroups(): New group value found = '");
					debug_print("tposeIOgetUniqueGroups(): row=%u / field=%u = %s\thash=%ld\tuniqueGroupCount=%u\n", rowCount, fieldCount, tempString, hashValue, uniqueGroupCount);

					btreeSetKeyValue(key, hashValue, uniqueGroupCount, 0);
					if(btreeInsert(btree, key) == -1) {
						printf("tposeIOgetUniqueGroups(): btreeInsert error\n");
					}

					// Insert into TposeHeader object
					allocString = malloc(strlen(tempString) * sizeof(char));
					strcpy(allocString, tempString);
					*(header->fields+(uniqueGroupCount++)) = allocString;
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
			if(++totalCharCount == partitionCharLimit) break;

		}

		// Write output header for partition
		header->numFields = uniqueGroupCount;

		// Clean-up
		btreeFree(&btree);
	
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
	printf("chunks = %d\n", chunks);


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
		printf("current chunk = %d\n", chunkCtr);
			
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
					if(btreeInsert(btree, key) == -1) {
						printf("tposeIOgetUniqueGroups(): btreeInsert error\n");
					}

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

	} /* End for-loop over chunks */

	btreeKeyFree(&key);

	debug_print("tposeIOgetUniqueGroups(): Computed unique groups from input file\n");
	
	// Assign groups to output file header 
	(tposeQuery->outputFile)->fileGroupHeader = header; 
	
}



/** 
 ** "Simple" tranpose (no group/id) of rows-to-columns (naive algorithm)
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

		fieldSavePtr = (tposeQuery->inputFile)->fileAddr; // Re-initialise to start again
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

	debug_print("tposeIOTransposeSimple(): run tranpose!\n");

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
	printf("chunks = %d\n", chunks);


	/* Split large files into chunks */
	int chunkCtr;
	off_t iFileSize;
	for(chunkCtr = 1; chunkCtr <= chunks; chunkCtr++) {
		
		// Set file size limit for current chunk
		if(chunkCtr == chunks)
			iFileSize = rFileSize;
		else
			iFileSize = TPOSE_IO_CHUNK_SIZE;

		totalCharCount = 0;
		printf("current chunk = %d\n", chunkCtr);

		/* Process each chunk */
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
				if((groupFoundFlag == 1) && (numericFoundFlag == 1))
					(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);
					
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
					++totalCharCount;
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

	} /* End for-loop */

	debug_print("tposeIOTransposeGroup(): run tranpose!\n");

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
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct
	off_t totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	off_t hashCharCount = 0; 
	off_t hashValue = 0; 
	off_t fileSize = (tposeQuery->inputFile)->fileSize;
	off_t rFileSize = fileSize;
	unsigned int chunks = 1; // Number of file chunks


	tposeIOPrintGroupIdHeader(tposeQuery); // Print output header


	// Calculate file chunks
	while(rFileSize >= TPOSE_IO_CHUNK_SIZE){
		++chunks;
		rFileSize -= TPOSE_IO_CHUNK_SIZE;
	}
	printf("chunks = %d\n", chunks);


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

		totalCharCount = 0;
		printf("current chunk = %d\n", chunkCtr);

		/* process each chunk */
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
					}
					else {

						// 1 Print out current aggregates for id
						tposeIOPrintGroupIdData(idCurrentString, tposeQuery);
						// 2 Set new string as current id
						strcpy(idCurrentString, idTempString); // Set current id to aggregate values for
						// 3 Reset aggregates
						memset((tposeQuery->aggregator)->aggregates, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
						// 4 Aggregate value for new id 
						(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);

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

	} /* end for-loop */

	// Print last line
	tposeIOPrintGroupIdData(idCurrentString, tposeQuery);

	debug_print("tposeIOTransposeGroupId(): run tranpose!\n");

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
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
			fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], rowDelimiter);
		else
			fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], fieldDelimiter);
	}

	fflush((tposeQuery->outputFile)->fd);

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
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == ((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1)
			fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], rowDelimiter);
		else
			fprintf((tposeQuery->outputFile)->fd, "%.2f%c", (tposeQuery->aggregator)->aggregates[i], fieldDelimiter);
	}

	fflush((tposeQuery->outputFile)->fd);

}



/** 
 ** Prints output header
 **/
void tposeIOPrintGroupIdHeader(
	TposeQuery* tposeQuery
) {

	unsigned char fieldDelimiter = (tposeQuery->outputFile)->fieldDelimiter;

	
	// Id Header
	fprintf((tposeQuery->outputFile)->fd, "%s%c", "id", fieldDelimiter);

	int i;
	// Group Header
	for(i = 0; i < ((tposeQuery->outputFile)->fileGroupHeader)->numFields ; ++i) {
		if(i == (((tposeQuery->outputFile)->fileGroupHeader)->numFields - 1))
			fprintf((tposeQuery->outputFile)->fd, "%s%c", ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], rowDelimiter);
		else
			fprintf((tposeQuery->outputFile)->fd, "%s%c", ((tposeQuery->outputFile)->fileGroupHeader)->fields[i], fieldDelimiter);
	}

}
