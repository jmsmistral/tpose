/* tpose_io.h: tpose input/output interface; 

   Copyright 2015 Jonathan Sacramento.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TPOSE_IO_H_
#define _TPOSE_IO_H_

	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <ctype.h>

	#include "system.h"
	#include "btree.h"


	/**
	 ** Implementation defs & limits
	 **/
	#define _FILE_OFFSET_BITS 64 // Enable long file access

	#define TPOSE_IO_MAX_LINE 1048576
	#define TPOSE_IO_MAX_FIELDS 5000
	#define TPOSE_IO_MAX_FIELD_WIDTH 5000
	
	#define TPOSE_IO_HASH_MULT 37

	#define TPOSE_IO_CHUNK_SIZE 1073741824

	#define TPOSE_IO_MODIFY_HEADER 1
	#define TPOSE_IO_NO_MODIFY_HEADER 0



	extern unsigned char rowDelimiter; // Defines row delimiter
	extern char** prefixString;
	extern char** suffixString;



	/**
	 ** TposeHeader
	 **/
	typedef struct {
		char** fields;
		unsigned int numFields; // Number of actual fields
		unsigned int maxFields; // Number of total fields allocated (used to free memory)
	} TposeHeader;


	/**
	 ** TposeAggregator
	 **/
	typedef struct {
		double* aggregates;
		unsigned int numFields;
	} TposeAggregator;


	/**
	 ** TposeInputFile
	 **/
	typedef struct {
		int fd;
		char* fileAddr;
		char* dataAddr;
		off_t fileSize;
		unsigned char fieldDelimiter;
		TposeHeader* fileHeader;
	} TposeInputFile;

	/**
	 ** TposeOutputFile
	 **/
	typedef struct {
		FILE* fd;
		unsigned char fieldDelimiter;
		TposeHeader* fileIdHeader;
		TposeHeader* fileGroupHeader;
	} TposeOutputFile;

	/**
	 ** TposeQuery
	 **/
	typedef struct {
		TposeInputFile* inputFile;
		TposeOutputFile* outputFile;
		TposeAggregator* aggregator;
		int id;
		int group;
		int numeric;
	} TposeQuery;


	/* Input */
	TposeInputFile* tposeIOOpenInputFile(char* filePath, unsigned char fieldDelimiter, unsigned int mutateHeader);
	int tposeIOCloseInputFile(TposeInputFile* inputFile);

	TposeInputFile* tposeIOInputFileAlloc(int fd, char* fileAddr, off_t fileSize, unsigned char fieldDelimiter);
	void tposeIOInputFileFree(TposeInputFile** intputFilePtr);

	TposeHeader* tposeIOReadInputHeader(TposeInputFile* inputFile, unsigned int mutateHeader);


	/* Output */
	TposeOutputFile* tposeIOOpenOutputFile(char* filePath, const char* mode, unsigned char fieldDelimiter);
	int tposeIOCloseOutputFile(TposeOutputFile* outputFile);

	TposeOutputFile* tposeIOOutputFileAlloc(FILE* fd, unsigned char fieldDelimiter);
	void tposeIOOutputFileFree(TposeOutputFile** outputFilePtr);


	/* General */
	TposeHeader* tposeIOHeaderAlloc(unsigned int maxFields, unsigned int mutateHeader);
	void tposeIOHeaderFree(TposeHeader** tposeHeaderPtr);

	TposeAggregator* tposeIOAggregatorAlloc(unsigned int numFields);
	void tposeIOAggregatorFree(TposeAggregator** tposeAggregatorPtr);

	TposeQuery* tposeIOQueryAlloc(TposeInputFile* inputFile, TposeOutputFile* outputFile, char* idVar, char* groupVar, char* numericVar);
	TposeQuery* tposeIOQueryIndexedAlloc(TposeInputFile* inputFile, TposeOutputFile* outputFile, int idVar, int groupVar, int numericVar);
	void tposeIOQueryFree(TposeQuery** tposeQueryPtr);


	/* Util */
	void tposeIOTransposeSimple(TposeQuery* tposeQuery);

	void tposeIOUniqueGroups(TposeQuery* tposeQuery, BTree* btree);
	void tposeIOTransposeGroup(TposeQuery* tposeQuery, BTree* btree);
	void tposeIOPrintOutput(TposeQuery* tposeQuery);

	void tposeIOTransposeGroupId(TposeQuery* tposeQuery, BTree* btree);
	void tposeIOPrintGroupIdHeader(TposeQuery* tposeQuery);
	void tposeIOPrintGroupIdData(char* id, TposeQuery* tposeQuery);
	int tposeIOGetFieldIndex(TposeHeader* tposeHeader, char* field); 
	char* tposeIOLowerCase(char* string);


/* parallel test start */
	
	#define TPOSE_IO_PARTITION_GROUP 0
	#define TPOSE_IO_PARTITION_ID 1

	BTree* btreeGlobal; // Needs to persist between computing unique groups, and aggregating values

	typedef struct {
		unsigned int threadId;
		TposeQuery* query;
		TposeHeader* header;
	} TposeThreadData;

	TposeThreadData** threadDataArray;
	TposeThreadData* threadData;

	typedef struct {
		unsigned int threadId;
		TposeQuery* query;
		TposeAggregator* aggregator;
	} TposeThreadAggregator;

	TposeThreadAggregator** threadAggregatorArray;
	TposeThreadAggregator* threadAggregator;

	unsigned int fileChunks; // Number of file chunks
	off_t partitions[1000];
	
	TposeOutputFile* tempFileArray[1000];


	// functions
	int tposeIOBuildPartitions(TposeQuery* tposeQuery, unsigned int mode);

	void tposeIOUniqueGroupsParallel(TposeQuery* tposeQuery);
	void* tposeIOUniqueGroupsMap(void* threadArg);
	void tposeIOUniqueGroupsReduce(TposeQuery* tposeQuery);

	void tposeIOTransposeGroupParallel(TposeQuery* tposeQuery);
	void* tposeIOTransposeGroupMap(void* threadArg);
	void tposeIOTransposeGroupReduce(TposeQuery* tposeQuery);

	void tposeIOTransposeGroupIdParallel(TposeQuery* tposeQuery);
	void* tposeIOTransposeGroupIdMap(void* threadArg);
	void tposeIOTransposeGroupIdReduce(TposeQuery* tposeQuery);

	void tposeIOPrintGroupIdHeaderParallel(TposeQuery* tposeQuery, unsigned int threadId);
	void tposeIOPrintGroupIdDataParallel(char* id, TposeQuery* tposeQuery, TposeAggregator* aggregator, unsigned int threadId);
/* parallel test end */



#endif /* TPOSE_IO_H */

