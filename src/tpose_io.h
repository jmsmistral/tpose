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
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <assert.h>
	#include <error.h>
	#include <errno.h>

	#include "btree.h"


	/**
	 ** Implementation limits
	 **/
	#define TPOSE_IO_MAX_LINE 1048576
	#define TPOSE_IO_MAX_FIELDS 5000
	#define TPOSE_IO_MAX_FIELD_WIDTH 5000

	extern char* rowDelimiter;
	

	/**
	 ** TposeHeader
	 **/
	typedef struct {
		char** fields;
		unsigned int numFields;
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
		int fd;
		char* fileAddr;
		unsigned char fieldDelimiter;
		TposeHeader* fileHeader;
	} TposeOutputFile;

	/**
	 ** TposeQuery
	 **/
	typedef struct {
		TposeInputFile* inputFile;
		//TposeInputFile* tposeOutputFile;
		int id;
		int group;
		int numeric;
	} TposeQuery;


	/* interface */
	/* input */
	TposeInputFile* tposeIOOpenInputFile(char* filePath, unsigned char fieldDelimiter);
	int tposeIOCloseInputFile(TposeInputFile* tposeFile);

	TposeHeader* tposeIOReadInputHeader(TposeInputFile* tposeFile);

	TposeInputFile* tposeIOInputFileAlloc(int fd, char* fileAddr, size_t fileSize, unsigned char fieldDelimiter);
	void tposeIOInputFileFree(TposeInputFile** tposeFilePtr);


	/* output */
	TposeOutputFile* tposeIOOpenOutFile(char* filePath, unsigned char fieldDelimiter);
	int tposeIOCloseOutputFile(TposeOutputFile* tposeFile);


	/* input/output */
	TposeHeader* tposeIOHeaderAlloc(unsigned int numFields);
	void tposeIOHeaderFree(TposeHeader** tposeHeaderPtr);

	TposeAggregator* tposeIOAggregatorAlloc(unsigned int numFields);
	void tposeIOAggregatorFree(TposeAggregator** tposeAggregatorPtr);

	TposeQuery* tposeIOQueryAlloc(TposeInputFile* inputFile, char* idVar, char* groupVar, char* numericVar);
	void tposeIOQueryFree(TposeQuery** tposeQueryPtr);


	/* util */
	TposeHeader* tposeIOgetUniqueGroups(TposeQuery* tposeQuery, BTree* btree);
	void tposeIOTransposeGroup(TposeQuery* tposeQuery, TposeHeader* outputHeader, TposeAggregator* aggregates, BTree* btree);
	//int tposeIOgetFieldIndex(TposeHeader* tposeHeader, char* field); 


#endif /* TPOSE_IO_H */

