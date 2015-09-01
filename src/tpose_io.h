/* tpose_io.h: tpose input/output declarations; 

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


	#include <string.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <assert.h>
	#include <error.h>
	#include <errno.h>
	#include <unistd.h>
	#include <fcntl.h>

	#include "btree.h"


	/**
	 ** Implementation limits
	 **/
	#define TPOSE_IO_MAX_LINE 1048576
	#define TPOSE_IO_MAX_FIELDS 5000
	#define TPOSE_IO_MAX_FIELD_WIDTH 5000
	
	#define TPOSE_IO_HASH_MULT 37

	extern char* rowDelimiter;


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
		size_t fileSize;
		unsigned char fieldDelimiter;
		//size_t pageSize;
		
		TposeHeader* fileHeader;
	} TposeInputFile;

	/**
	 ** TposeOutputFile
	 **/
	typedef struct {
		//int fd;
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
	TposeInputFile* tposeIOOpenInputFile(char* filePath, unsigned char fieldDelimiter);
	int tposeIOCloseInputFile(TposeInputFile* inputFile);

	TposeInputFile* tposeIOInputFileAlloc(int fd, char* fileAddr, size_t fileSize, unsigned char fieldDelimiter);
	void tposeIOInputFileFree(TposeInputFile** intputFilePtr);

	TposeHeader* tposeIOReadInputHeader(TposeInputFile* inputFile);


	/* Output */
	TposeOutputFile* tposeIOOpenOutputFile(char* filePath, unsigned char fieldDelimiter);
	int tposeIOCloseOutputFile(TposeOutputFile* outputFile);

	TposeOutputFile* tposeIOOutputFileAlloc(FILE* fd, unsigned char fieldDelimiter);
	void tposeIOOutputFileFree(TposeOutputFile** outputFilePtr);


	/* General */
	TposeHeader* tposeIOHeaderAlloc(unsigned int maxFields);
	void tposeIOHeaderFree(TposeHeader** tposeHeaderPtr);

	TposeAggregator* tposeIOAggregatorAlloc(unsigned int numFields);
	void tposeIOAggregatorFree(TposeAggregator** tposeAggregatorPtr);

	TposeQuery* tposeIOQueryAlloc(TposeInputFile* inputFile, TposeOutputFile* outputFile, char* idVar, char* groupVar, char* numericVar);
	void tposeIOQueryFree(TposeQuery** tposeQueryPtr);


	/* Util */
	void tposeIOgetUniqueGroups(TposeQuery* tposeQuery, BTree* btree);
	void tposeIOTransposeGroup(TposeQuery* tposeQuery, BTree* btree);
	void tposeIOPrintOutput(TposeQuery* tposeQuery);
	//int tposeIOgetFieldIndex(TposeHeader* tposeHeader, char* field); 


#endif /* TPOSE_IO_H */

