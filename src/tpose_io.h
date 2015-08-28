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
	 ** TposeLine
	 **/
	typedef struct {
	} TposeLine;

	/**
	 ** TposeFile
	 **/
	typedef struct {
		int fd;
		char* fileAddr;
		char* dataAddr;
		size_t fileSize;
		unsigned char fieldDelimiter;
		//size_t pageSize;
		
		TposeHeader* fileHeader;
	} TposeFile;

	/**
	 ** TposeQuery
	 **/
	typedef struct {
		TposeFile* tposeInputFile;
		//TposeFile* tposeOutputFile;
		int id;
		int group;
		int numeric;
	} TposeQuery;


	/* tpose io */
	TposeFile* tposeIOOpenFile(char* filePath, unsigned char fieldDelimiter);
	int tposeIOCloseFile(TposeFile* tposeFile);

	TposeHeader* tposeIOReadHeader(TposeFile* tposeFile);

	TposeFile* tposeIOFileAlloc(int fd, char* fileAddr, size_t fileSize, unsigned char fieldDelimiter);
	void tposeIOFileFree(TposeFile** tposeFilePtr);

	TposeHeader* tposeIOHeaderAlloc(unsigned int numFields);
	void tposeIOHeaderFree(TposeHeader** tposeHeaderPtr);

	TposeQuery* tposeIOQueryAlloc(TposeFile* tposeFile, char* idVar, char* groupVar, char* numericVar);
	void tposeIOQueryFree(TposeQuery** tposeQueryPtr);

	//int tposeIOgetFieldIndex(TposeHeader* tposeHeader, char* field);
	TposeHeader* tposeIOgetUniqueGroups(TposeQuery* tposeQuery);


#endif /* TPOSE_IO_H */

