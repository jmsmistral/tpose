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
#include "btree.h"

char* rowDelimiter = "\n";


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
	
	int i;
	for(i=0; i<tposeHeader->numFields; i++) {
		if(!strcmp(field, *(tposeHeader->fields+i))) {
			printf("i = %d\n", i);
			break;
		}
	}

	printf("broken out @ i = %d\n", i);

	return i; 
	
}



/** 
 ** Allocates memory for the input file headers arrays 
 **/
TposeFile* tposeIOFileAlloc(
	int fd
	,char* fileAddr
	,size_t fileSize
	,unsigned char fieldDelimiter
) {

	TposeFile* tposeFile;

	/* Allocate memory for the field names */
	if((tposeFile = (TposeFile*) malloc(sizeof(TposeFile))) == NULL ) {
		printf("tposeIOFileAlloc: malloc error\n");
		return NULL;
	}

	tposeFile->fd = fd;
	tposeFile->fileAddr = fileAddr;
	tposeFile->fileSize = fileSize;
	tposeFile->fieldDelimiter = fieldDelimiter;
	
	//assert(tposeFile->fd != NULL);
	assert(tposeFile->fileAddr != NULL);
	assert(tposeFile->fileSize != 0);
	//assert(tposeFile->fieldDelimiter != '');

	return tposeFile;
	
}



/** 
 ** Free memory for a TposeFile 
 **/
void tposeIOFileFree(
    TposeFile** tposeFilePtr
) {

	printf("tposeIOFileFree: Freeing TposeFile memory...\n");

	tposeIOHeaderFree(&((*tposeFilePtr)->fileHeader));
    
   if(*tposeFilePtr != NULL) {
       free(*tposeFilePtr);
       *tposeFilePtr = NULL;
   }

   assert(*tposeFilePtr == NULL);
   printf("tposeIOFileFree: TposeFile has been freed!\n");
}



/** 
 ** Allocates memory for the input file headers arrays 
 **/
TposeHeader* tposeIOHeaderAlloc(unsigned int numFields)
{
	printf("tposeIOHeaderAlloc: numFields = %u\n", numFields);

	TposeHeader* tposeHeader;

	/* Allocate memory for the field names */
	if((tposeHeader = (TposeHeader*) malloc(sizeof(TposeHeader))) == NULL ) {
		printf("tposeIOHeaderAlloc: malloc error\n");
		return NULL;
	}

	if((tposeHeader->fields = (char**) malloc(numFields * sizeof(char*))) == NULL ) {
		printf("tposeIOHeaderAlloc: malloc error\n");
		return NULL;
	}

	tposeHeader->numFields = numFields;
	
	assert(tposeHeader->fields != NULL);
	assert(tposeHeader->numFields != 0);

	return tposeHeader;
	
}



/** 
 ** Free memory for a TposeHeader 
 **/
void tposeIOHeaderFree(
    TposeHeader** tposeHeaderPtr
) {

	printf("tposeIOHeaderFree: Freeing TposeHeader memory...\n");

/*	printf("field: %s\n", *(*tposeHeaderPtr)->fields);
	printf("field: %s\n", *((*tposeHeaderPtr)->fields+1));
	printf("field: %s\n", (*tposeHeaderPtr)->fields[2]);
	printf("field: %s\n", (*tposeHeaderPtr)->fields[3]);
*/

	int field;
	if((*tposeHeaderPtr)->fields != NULL) {
		for(field = 0; field < (*tposeHeaderPtr)->numFields; field++) {
			free((*tposeHeaderPtr)->fields[field]);
		} 

		free((*tposeHeaderPtr)->fields);
		(*tposeHeaderPtr)->fields = NULL;
	}
    
    assert((*tposeHeaderPtr)->fields == NULL);
    printf("tposeIOHeaderFree: TposeHeader variables have been freed!\n");

    if(*tposeHeaderPtr != NULL) {
        free(*tposeHeaderPtr);
        *tposeHeaderPtr = NULL;
    }

    assert(*tposeHeaderPtr == NULL);
    printf("tposeIOHeaderFree: TposeHeader has been freed!\n");
	
    
}



/** 
 ** Allocates memory for the transpose parameters 
 **/
TposeQuery* tposeIOQueryAlloc(
	TposeFile* tposeInputFile
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
	
	tposeQuery->tposeInputFile = tposeInputFile;
	tposeQuery->id = getFieldIndex(tposeInputFile->fileHeader, idVar);
	tposeQuery->group = getFieldIndex(tposeInputFile->fileHeader, groupVar);
	tposeQuery->numeric = getFieldIndex(tposeInputFile->fileHeader, numericVar);

	printf("*** Initialised query id = %d\n", tposeQuery->id);
	printf("*** Initialised query group = %d\n", tposeQuery->group);
	printf("*** Initialised query numeric = %d\n", tposeQuery->numeric);

	return tposeQuery;
	
}



/** 
 ** Free memory for a TposeQuery 
 **/
void tposeIOQueryFree(
    TposeQuery** tposeQueryPtr
) {

	printf("tposeIOQueryFree: Freeing TposeQuery memory...\n");
    
	// No need to free the tposeInputFile, as this is done in the tposeIOCloseFile() call
   if(*tposeQueryPtr != NULL) { 
       free(*tposeQueryPtr);
       *tposeQueryPtr = NULL;
   }

   assert(*tposeQueryPtr == NULL);
   printf("tposeIOQueryFree: TposeQuery has been freed!\n");
}



/** 
 ** Open a file
 **/
TposeFile* tposeIOOpenFile(
	char* filePath
	,unsigned char fieldDelimiter
) {

	int fd;
	char* fileAddr;
	size_t fileSize;
	struct stat statBuffer;

	if((fd = open(filePath, O_RDONLY)) < 0) {
		printf("Error: tposeIOOpenFile - can not open file %s\n", filePath);
		return NULL;
	}

	if(fstat(fd, &statBuffer) < 0) {
		printf("Error: tposeIOOpenFile - can not stat file %s\n", filePath);
		return NULL;
	}

	fileSize = statBuffer.st_size;
	//pageSize = (size_t) sysconf (_SC_PAGESIZE); // Get page size

	if((fileAddr = mmap(0, statBuffer.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED ) {
		printf("Error: tposeIOOpenFile - can not map file %s\n", filePath);
		return NULL;
	}

	// might speed-up via aggressive read-ahead caching
	if(madvise(fileAddr, fileSize, MADV_SEQUENTIAL) == -1) {
		printf("Warning: tposeIOOpenFile - cannot advise kernel on file %s\n", filePath);
	}

	printf("tposeIOOpenFile opened file %s\n", filePath);

	TposeFile* tposeFile = tposeIOFileAlloc(fd, fileAddr, fileSize, fieldDelimiter); // Creates the file handle 
	tposeFile->fileHeader = tposeIOReadHeader(tposeFile); // Opening a file also creates the TposeHeader struct

	return tposeFile;

}



/** 
 ** Close a file and unmap any memory
 **/
int tposeIOCloseFile(
	TposeFile* tposeFile
) {

   if((munmap(tposeFile->fileAddr, tposeFile->fileSize)) < 0) {
       printf("Error: tposeIOCloseFile - can not unmap file\n");
       return -1;
   }

   if(close(tposeFile->fd) < 0) {
       printf("Error: tposeIOCloseFile - can not close file\n");
       return -1;
   }

	// Free TposeFile memory
	tposeIOFileFree(&tposeFile);
    
	return 0;

}




TposeHeader* tposeIOReadHeader(
	TposeFile* tposeFile
) {
    
	char* rowtok;
	char* fieldtok;
	char* fieldSavePtr;
	
	char* tempString;

	unsigned int fieldCount = 0;
	unsigned int length = 0;
	unsigned int curField = 0;

	
	// Test if we have a good TposeFile*
	if(!tposeFile) return NULL;

	// Count number of fields
	while( *(tposeFile->fileAddr+length) != '\n') {
		if(  *(tposeFile->fileAddr+length) == (tposeFile->fieldDelimiter))
			fieldCount++; length++;
	}
	
	if(fieldCount > 0)
		fieldCount++; // Quick hack to get real number of fields

	TposeHeader* header = tposeIOHeaderAlloc(fieldCount); // Allocate the needed memory

	// Find index of first row delimiter
	tposeFile->dataAddr = strchr(tposeFile->fileAddr, *rowDelimiter);
	*tposeFile->dataAddr = '\0';
	tposeFile->dataAddr+=1; // Make sure we're not pointing at the NULL 
	printf("*** TEST: %s\n", tposeFile->fileAddr);
	//printf("*** TEST: \n%s\n", tposeFile->dataAddr);

	// Create a copy of the NULL terminated string (as strsep/strtok_r modifies this)
	rowtok = strdup(tposeFile->fileAddr);
	
	// Read header fields
	fieldtok = strtok_r(rowtok, &(tposeFile->fieldDelimiter), &fieldSavePtr);
	if(fieldtok == NULL) return NULL;
	tempString = malloc(strlen(fieldtok) * sizeof(char));
	strcpy(tempString, fieldtok);
	*(header->fields) = tempString;
	for(fieldCount = 1; (fieldtok = strtok_r(NULL, &(tposeFile->fieldDelimiter), &fieldSavePtr)) != NULL; ) {
		tempString = malloc(strlen(fieldtok) * sizeof(char));
		strcpy(tempString, fieldtok);
		*(header->fields+(fieldCount++)) = tempString;
	}
	

	free(rowtok);
	//free(fieldtok);
	//free(fieldSavePtr);
	//free(tempString);
	/*rowtok = strtok_r(tposeFile->fileAddr, rowDelimiter, &rowSavePtr);
	if(!isHeader++) {
		if(rowtok != NULL) {
			//printf("HEADER FIELDS = %s\n", rowtok);
			fieldtok = strtok_r(rowtok, &(tposeFile->fieldDelimiter), &fieldSavePtr);
			tempString = malloc(strlen(fieldtok) * sizeof(char));
			strcpy(tempString, fieldtok);
			*(header->fields) = tempString;
			for(fieldCount = 1; (fieldtok = strtok_r(NULL, &(tposeFile->fieldDelimiter), &fieldSavePtr)) != NULL; ) {
				tempString = malloc(strlen(fieldtok) * sizeof(char));
				strcpy(tempString, fieldtok);
				*(header->fields+(fieldCount++)) = tempString;
			}
				
		}
		else
			return NULL;
        
	}*/

	return header;

    /*while(rowtok != NULL) {
        //printf("row = %s\n", rowtok);
        fieldtok = strtok_r(rowtok, fieldDel, &fieldSavePtr);
        while(fieldtok != NULL) {
            ++curField;
            //printf("field = %s\n", fieldtok);
            if(curField == primaryKeyField) {
                //printf("primary key field = %s\n", fieldtok);
                tposeSetKeyValue(key, atol(fieldtok), 0, strlen(fieldtok));
                if(tposeInsert(tpose, key) == -1) {
                    printf("tposeIOIndexFile Error: tposeInsertKey error!\n");
                    exit(-1);
                }
            }
            fieldtok = strtok_r(NULL, fieldDel, &fieldSavePtr);
        }
        curField = 0; //reset field count on new row
        rowtok = strtok_r(NULL, rowDelimiter, &rowSavePtr);
    } */

}



/** 
 ** Returns a unique list of GROUP variable values 
 **/
TposeHeader* tposeIOgetUniqueGroups(
	TposeQuery* tposeQuery
) {


	char* fieldSavePtr;
	char tempChar;
	char tempString[TPOSE_IO_MAX_FIELD_WIDTH];
	char* allocString;

	TposeHeader* header = tposeIOHeaderAlloc(TPOSE_IO_MAX_FIELDS); // Allocate the needed memory
	BTree* btree = btreeAlloc();
	BTreeKey* key = btreeKeyAlloc();
	BTreeKey* resultKey;

	unsigned int fieldCount = 0; // Already pointing at the first field when loop starts 
	unsigned int uniqueGroupCount = 0; // Used to index array of header ptrs
	unsigned int charCount = 0; 
	unsigned int hashCharCount = 0; 
	unsigned long hashValue = 0; 
	const unsigned long hashKey = 5381; 
	const unsigned long hashMult = 31; 

	fieldSavePtr = (tposeQuery->tposeInputFile)->dataAddr; // Init with ptr to second row (where data starts)

	printf("\n");
	while(*fieldSavePtr != EOF) {

		if(*fieldSavePtr == '\t') {
			++fieldCount;
			++fieldSavePtr;
				continue;
		}

		if(*fieldSavePtr == '\n') {
			fieldCount = 0;
			++fieldSavePtr;
				continue;
		}
		
		// Check if = group field
		if(fieldCount == (tposeQuery->group))  { // if group value is empty string we ignore

			// Copy field value
			while(*fieldSavePtr != '\t' && *fieldSavePtr != '\n') {
				tempString[charCount++] = *fieldSavePtr++;
			}
			tempString[charCount] = '\0'; // Null-terminate string

			// Convert char array to hash
			for(hashCharCount = 0; hashCharCount <= charCount-1; ++hashCharCount)
				hashValue += hashMult * hashKey + hashCharCount + (unsigned char) tempString[hashCharCount];
				//hashValue += hashMult * hashKey + (unsigned char) tempString[hashCharCount];

				// Check for collisions
				printf("%s\t", tempString);
				printf("%ld\n", hashValue); 

			// Insert into btree
			if( (resultKey = (BTreeKey*) btreeSearch(btree, btree->root, hashValue)) == NULL) {
				/*printf("\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
				printf("New group value found = '"); */

				btreeSetKeyValue(key, hashValue, 0, 0);
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
				header->numFields = uniqueGroupCount;
				/*printf("BTREE\n");
				btreeForEach(btree, btreeCBPrintNode);
				printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");*/
			}

			// Reset variables (
			charCount = 0;
			hashValue = 0;
			
			if(*fieldSavePtr == '\t')
				--fieldSavePtr;

			if(*fieldSavePtr == '\n')
				--fieldSavePtr;
		}

		// If not a delimiter or the group field, then ++
		++fieldSavePtr;

	}

	
	/*printf("\n");
	printf("uniqueGroupCount = %d\n", uniqueGroupCount);
	printf("header->numFields = %u\n", header->numFields);
	btreeForEach(btree, btreeCBPrintNode); 
	
	int i;
	for(i = 0; i < uniqueGroupCount; ++i)
		printf("group %d = %s\n", i, header->fields[i]); */
	

	return header;
	
}


