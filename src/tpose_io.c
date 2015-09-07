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
			debug_print("i = %d\n", i);
			foundFlag = 1;
			break;
		}
	}

	if(!foundFlag && (i==numFields))
		return -1; // Not found


	debug_print("tposeIOGetFieldIndex found flag =  %d\n", foundFlag);
	debug_print("tposeIOGetFieldIndex search field =  %s\n", field);
	debug_print("tposeIOGetFieldIndex numFields = %d\n", numFields);
	debug_print("broken out @ i = %d\n", i);

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

	debug_print("tposeIOFileFree: Freeing TposeInputFile memory...\n");

	tposeIOHeaderFree(&((*inputFilePtr)->fileHeader));
    
   if(*inputFilePtr != NULL) {
       free(*inputFilePtr);
       *inputFilePtr = NULL;
   }

   assert(*inputFilePtr == NULL);

   debug_print("tposeIOFileFree: TposeInputFile has been freed!\n");
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

	debug_print("tposeIOFileFree: Freeing TposeInputFile memory...\n");

	if( (*outputFilePtr)->fileIdHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileIdHeader));

	if( (*outputFilePtr)->fileGroupHeader != NULL)
		tposeIOHeaderFree(&((*outputFilePtr)->fileGroupHeader));
    
   if(*outputFilePtr != NULL) {
       free(*outputFilePtr);
       *outputFilePtr = NULL;
   }

   assert(*outputFilePtr == NULL);
   
   debug_print("tposeIOFileFree: TposeInputFile has been freed!\n");
}



/** 
 ** Allocates memory for the input file headers arrays 
 **/
TposeHeader* tposeIOHeaderAlloc(
	unsigned int maxFields
) {
	
	debug_print("tposeIOHeaderAlloc: maxFields = %u\n", numFields);

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
	
	//assert(tposeHeader->fields != NULL); // Can be NULL if !mutateHeader
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

	debug_print("tposeIOHeaderFree: Freeing TposeHeader memory...\n");

	int field;
	if((*tposeHeaderPtr)->fields != NULL) {
		for(field = 0; field < (*tposeHeaderPtr)->maxFields; field++) {
			free((*tposeHeaderPtr)->fields[field]);
		} 

		free((*tposeHeaderPtr)->fields);
		(*tposeHeaderPtr)->fields = NULL;
	}
    
    assert((*tposeHeaderPtr)->fields == NULL);
    debug_print("tposeIOHeaderFree: TposeHeader variables have been freed!\n");

    if(*tposeHeaderPtr != NULL) {
        free(*tposeHeaderPtr);
        *tposeHeaderPtr = NULL;
    }

    assert(*tposeHeaderPtr == NULL);

    debug_print("tposeIOHeaderFree: TposeHeader has been freed!\n");
    
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
 ** Note: Matches field names
 **/
TposeQuery* tposeIOQueryAlloc(
	TposeInputFile* inputFile
	,TposeOutputFile* outputFile
	,char* idVar
	,char* groupVar
	,char* numericVar
) {

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
	if(idVar != NULL) tposeQuery->id = tposeIOGetFieldIndex(inputFile->fileHeader, idVar);
	if(groupVar != NULL) tposeQuery->group = tposeIOGetFieldIndex(inputFile->fileHeader, groupVar);
	if(numericVar != NULL) tposeQuery->numeric = tposeIOGetFieldIndex(inputFile->fileHeader, numericVar);

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

	if((fileAddr = mmap(0, statBuffer.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED ) {
		printf("Error: tposeIOOpenInputFile - can not map inputfile %s\n", filePath);
		return NULL;
	}

	// might speed-up via aggressive read-ahead caching
	if(madvise(fileAddr, fileSize, MADV_SEQUENTIAL) == -1) {
		printf("Warning: tposeIOOpenInputFile - cannot advise kernel on file %s\n", filePath);
	}

	debug_print("tposeIOOpenInputFile(): opened input file %s\n", filePath);

	TposeInputFile* inputFile = tposeIOInputFileAlloc(fd, fileAddr, fileSize, fieldDelimiter); // Creates the file handle 
	inputFile->fileHeader = tposeIOReadInputHeader(inputFile, mutateHeader); // Opening a file also creates the TposeHeader struct

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

	debug_print("tposeIOCloseInputFile(): closed input file\n");
    
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
		if((fd = fopen(filePath, "wa" )) == NULL) {
			printf("Error: tposeIOOpenOutputFile - can not open output file %s\n", filePath);
			return NULL;
		}
	}
	else
		fd = stdout; // If no output file is passed, print to std output

	debug_print("tposeIOOpenOutputFile(): opened output file %s\n", filePath);

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

	debug_print("tposeIOCloseOutputFile(): closed output file\n");
    
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

	TposeHeader* header = tposeIOHeaderAlloc(fieldCount); // Allocate the needed memory

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
void tposeIOgetUniqueGroups(
	TposeQuery* tposeQuery
	,BTree* btree
) {

	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;

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

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)


	while(totalCharCount <= (tposeQuery->inputFile)->fileSize) {

		if(*fieldSavePtr == fieldDelimiter) {
			++fieldCount;
			++fieldSavePtr;
			++totalCharCount;
			continue;
		}

		if(*fieldSavePtr == rowDelimiter) {
			fieldCount = 0;
			++rowCount;
			++fieldSavePtr;
			++totalCharCount;
			continue;
		}
		
		// Check if = group field
		if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

			// Copy field value
			while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
				tempString[groupCharCount++] = *fieldSavePtr++;
				++totalCharCount;
			}
			tempString[groupCharCount] = '\0'; // Null-terminate string

			// Convert char array to hash
			for(hashCharCount = 0; hashCharCount <= groupCharCount-1; ++hashCharCount)
				hashValue = TPOSE_IO_HASH_MULT * hashValue + (unsigned char) tempString[hashCharCount];

			// Insert into btree
			if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) == NULL) {
				debug_print("tposeIOgetUniqueGroups(): New group value found = '");
				
				// Check for collisions - print only when we add a unique group
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

		// If not a delimiter or the group field, then ++
		++fieldSavePtr;
		++totalCharCount;

	}

	btreeKeyFree(&key);
	
	// Assign groups to output file header 
	(tposeQuery->outputFile)->fileGroupHeader = header; 
	
}



/** 
 ** "Simple" tranpose (no group/id) of rows-to-columns (naive algorithm)
 **/
void tposeIOTransposeSimple(
	TposeQuery* tposeQuery
) {

	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	char* fieldSavePtr;
	char fieldTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)

	unsigned int fieldCharCount = 0;
	unsigned int totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned long currentField = 0;
	unsigned long numFields = ((tposeQuery->inputFile)->fileHeader)->maxFields;

	fieldSavePtr = (tposeQuery->inputFile)->fileAddr; // Init with ptr to first row (no group/id in this transpose method)

	// Main loop
	for(currentField = 0; currentField < numFields; ++currentField) {
		fieldSavePtr = (tposeQuery->inputFile)->fileAddr; // Re-initialise to start again
		totalCharCount = 0;

		while(totalCharCount <= (tposeQuery->inputFile)->fileSize) {

			if(*fieldSavePtr == fieldDelimiter) {
				++fieldCount;
				++fieldSavePtr;
				++totalCharCount;
				continue;
			}

			if(*fieldSavePtr == rowDelimiter) {
				// Aggregate value for each group
				fprintf((tposeQuery->outputFile)->fd, "%s%c", fieldTempString, fieldDelimiter);
					
				// Reset flags for next row
				fieldCount = 0;

				++fieldSavePtr;
				++totalCharCount;
				continue;
			}
			
			// Track current group field
			if(fieldCount == currentField)  { // if group value is empty string we ignore

				// Get group field value
				while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
					fieldTempString[fieldCharCount++] = *fieldSavePtr++;
					++totalCharCount;
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
			++totalCharCount;

		} // End while-loop

		fprintf((tposeQuery->outputFile)->fd, "%c", rowDelimiter); // Output a new line after each iteration
	
	} // End for-loop

}



/** 
 ** Transposes numeric values for each unique group value
 **/
void tposeIOTransposeGroup(
	TposeQuery* tposeQuery
	,BTree* btree
) {

	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	char* fieldSavePtr;
	char groupTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char numericTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)
	unsigned int numericFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)

	tposeQuery->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields);
	BTreeKey* resultKey;

	unsigned int fieldCharCount = 0;
	unsigned int hashCharCount = 0; 
	unsigned int totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned long hashValue = 0; 

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)

	while(totalCharCount <= (tposeQuery->inputFile)->fileSize) {

		if(*fieldSavePtr == fieldDelimiter) {
			++fieldCount;
			++fieldSavePtr;
			++totalCharCount;
			continue;
		}

		if(*fieldSavePtr == rowDelimiter) {
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
				// Flag group field as found
				groupFoundFlag = 1;
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
		
		// Aggregate numeric field for given group
		if(fieldCount == (tposeQuery->numeric))  { // if group value is empty string we ignore

			numericFoundFlag = 1; // Flag numeric field as found
			
			// Copy field value
			while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
				numericTempString[fieldCharCount++] = *fieldSavePtr++;
				++totalCharCount;
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

		// If not a delimiter or the group field, then ++
		++fieldSavePtr;
		++totalCharCount;

	}

}



/** 
 ** Transposes numeric values for each unique group and id value
 **/
void tposeIOTransposeGroupId(
	TposeQuery* tposeQuery
	,BTree* btree
) {

	unsigned char fieldDelimiter = (tposeQuery->inputFile)->fieldDelimiter;
	char* fieldSavePtr;
	char idCurrentString[TPOSE_IO_MAX_FIELD_WIDTH]; // Holds current id value being aggregated
	char idTempString[TPOSE_IO_MAX_FIELD_WIDTH]; // Holds id value for each row
	char groupTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char numericTempString[TPOSE_IO_MAX_FIELD_WIDTH];
	off_t groupFieldIndex = 0; // Holds index of group field in TposeHeader struct
	unsigned int firstId = 1;
	unsigned int idFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)
	unsigned int groupFoundFlag = 0; // 1 when we find the group field, 0 otherwise (0 if there is no group value)
	unsigned int numericFoundFlag = 0; // 1 when we find the numeric field, 0 otherwise (0 if there is no numeric value)

	unsigned int fieldCharCount = 0;
	unsigned int hashCharCount = 0; 
	unsigned int totalCharCount = 0; // Needed to stop reading at EOF (mmap files are page aligned, so we end-up reading garbage after file data ends)
	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned long hashValue = 0; 

	tposeQuery->aggregator = tposeIOAggregatorAlloc(((tposeQuery->outputFile)->fileGroupHeader)->numFields); // Aggregates values
	BTreeKey* resultKey; // Used for searching the btree

	tposeIOPrintGroupIdHeader(tposeQuery); // Print header to output

	fieldSavePtr = (tposeQuery->inputFile)->dataAddr; // Init with ptr to second row (where data starts)

	while(totalCharCount <= (tposeQuery->inputFile)->fileSize) {

		if(*fieldSavePtr == fieldDelimiter) {
			++fieldCount;
			++fieldSavePtr;
			++totalCharCount;
			continue;
		}

		if(*fieldSavePtr == rowDelimiter) {

			if((idFoundFlag == 1) && (groupFoundFlag == 1) && (numericFoundFlag == 1)) {

				if(!strcmp(idCurrentString, idTempString)) {
					// Aggregate value for each group
					(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);
				}
				else {

					// 1 print out current aggregates for id
					tposeIOPrintGroupIdData(idCurrentString, tposeQuery);
					// 2 set new string as current id
					strcpy(idCurrentString, idTempString); // Set current id to aggregate values for
					// 3 reset aggregates
					memset((tposeQuery->aggregator)->aggregates, 0, ((tposeQuery->outputFile)->fileGroupHeader)->numFields * sizeof(double));
					// 4 aggregate value for new id 
					(tposeQuery->aggregator)->aggregates[groupFieldIndex] += atof(numericTempString);

				}

			}
				
			// Reset flags for next row
			idFoundFlag = 0;
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
				// Flag group field as found
				groupFoundFlag = 1;
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
		
		// Aggregate numeric field for given group
		if(fieldCount == (tposeQuery->numeric))  { // if group value is empty string we ignore

			numericFoundFlag = 1; // Flag numeric field as found
			
			// Copy field value
			while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
				numericTempString[fieldCharCount++] = *fieldSavePtr++;
				++totalCharCount;
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
		
		// Track current id field
		if(fieldCount == (tposeQuery->id))  { // if group value is empty string we ignore

			idFoundFlag = 1; // Flag numeric field as found
			
			// Copy field value
			while(*fieldSavePtr != fieldDelimiter && *fieldSavePtr != rowDelimiter) {
				idTempString[fieldCharCount++] = *fieldSavePtr++;
				++totalCharCount;
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
		++totalCharCount;

	}

	// Print last line
	tposeIOPrintGroupIdData(idCurrentString, tposeQuery);

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
