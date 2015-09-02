/* tpose.c -- transpose structured data text files. 

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

#include "system.h"
#include "util.h"
#include "tpose.h"
#include "tpose_io.h"



/* Commandline Options */
static const char* shortopts = "d:xp:s:a:k:i:g:n:hv";
static const struct option longopts[] = {
	{"delimiter", required_argument, NULL, 'd'}
	,{"indexed", no_argument, NULL, 'x'}
	,{"unbuffered", no_argument, NULL, 'u'}
	,{"prefix", required_argument, NULL, 'p'}
	,{"suffix", required_argument, NULL, 's'}
	,{"aggregate", required_argument, NULL, 'a'}
	,{"keep", required_argument, NULL, 'k'}
	,{"id", required_argument, NULL, 'i'}
	,{"group", required_argument, NULL, 'g'}
	,{"numeric", required_argument, NULL, 'n'}
	,{"help", no_argument, NULL, 'h'}
	,{"version", no_argument, NULL, 'v'}
	,{NULL, 0, NULL, 0}
};

/* Forward declarations */
static void printHelp (void);
static void printVersion (void);

static unsigned char delimiter;

int main(
	int argc
	,char* argv[]
) {

	set_program_name (argv[0]);
	atexit (close_stdout);

	int delimiterFlag = 0;
	int indexedFlag = 0;
	int unbufferedFlag = 0;
	int prefixFlag = 0;
	int suffixFlag = 0;
	int aggregateFlag = 0;
	int keepFlag = 0;
	int idFlag = 0;
	int groupFlag = 0;
	int numericFlag = 0;
	int helpFlag = 0;
	int versionFlag = 0;
	char* delimiterArg = NULL;
	char* prefixArg = NULL;
	char* suffixArg = NULL;
	char* aggregateArg = NULL;
	char* keepArg = NULL;
	char* idArg = NULL;
	char* groupArg = NULL;
	char* numericArg = NULL;

	bool delimiterSpecified = false;

	char* inputFilePath = NULL;
	char* outputFilePath = NULL;

	int curArg;	
	int iArg;
	int fileArg = 0;

	opterr = 0; // Sets getopt error flag to zero

	while ((curArg = getopt_long(argc, argv,  shortopts, longopts, NULL)) != -1) {
		switch(curArg) {
			case 'd':
				delimiterFlag = 1;
				delimiterArg = optarg;
				break;
			case 'x':
				indexedFlag = 1;
				break;
			case 'u':
				unbufferedFlag = 1;
				break;
			case 'p':
				prefixFlag = 1;
				prefixArg = optarg;
				break;
			case 's':
				suffixFlag = 1;
				suffixArg = optarg;
				break;
			case 'a':
				aggregateFlag = 1;
				aggregateArg = optarg;
				break;
			case 'k':
				keepFlag = 1;
				keepArg = optarg;
				break;
			case 'i':
				idFlag = 1;
				idArg = optarg;
				break;
			case 'g':
				groupFlag = 1;
				groupArg = optarg;
				break;
			case 'n':
				numericFlag = 1;
				numericArg = optarg;
				break;
			case 'h':
				helpFlag = 1;
				exit (EXIT_SUCCESS);
				break;
			case 'v':
				versionFlag = 1;
				exit (EXIT_SUCCESS);
				break;
			case '?':
				/*if (optopt == 'c')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr, "Unknown option '-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
				return 1; */
				fprintf(stderr, "Missing argument. optopt = %c\n", optopt);
				//TODO print usage text
				abort;
			default:
				abort();
		}
	}

	/* Get input file */
	//printf("optind = %d && argc = %d\n", optind, argc);
	if(optind < argc) {
		for (iArg = optind; iArg < argc; ++iArg) {

			/*printf("iArg = %d\n", iArg);
			printf("argv[iArg] = %s\n", argv[iArg]);
			printf("fileArg = %d\n", fileArg);*/

			if(fileArg == 0)
				inputFilePath = argv[iArg];
			else if(fileArg == 1)
				outputFilePath = argv[iArg];
			else {
				fprintf(stderr, "Too many files specified!\n");
				//printUsage();
				abort();
			}
			
			++fileArg;
		}
	}
	else {
		fprintf(stderr, "Missing input file.\n");
		//printUsage();
		abort();
	}
		

	/* Run checks on option arguments */
	// Check delimiter (default = \t)
	delimiter = '\0';
	if(delimiterArg != NULL) {

		if (delimiterArg[0] != '\0' && delimiterArg[1] != '\0') {
			fprintf(stderr, "The delimiter must be a single character\n");
			abort();
		}

		//fprintf(stdout, "A delimiter character has been specified\n");
		delimiter = (char) delimiterArg[0];
		delimiterSpecified = true;
	}

	if (!delimiterSpecified) {
		//fprintf(stdout, "A delimiter character has not been specified\n");
		delimiter = '\t';
	}

	/*printf("DELIMITER = '%c'\n", delimiter); */

	//Check group variable
	//printf("ID = %s\n", idArg);

	//Check group variable
	//printf("GROUP = %s\n", groupArg);
	
	//Check numeric variable
	//printf("NUMERIC = %s\n", numericArg); 
	
	// Check output file (if empty, use stdout)
	if(!outputFilePath) {
		outputFilePath = "stdout";
	}

	unsigned int mutateHeader = 1;
	if(!groupFlag && !numericFlag && !idFlag)
		mutateHeader = 0;
	
	printf("input: %s\n", inputFilePath); 
	printf("output: %s\n", outputFilePath); 

	/* Event-loop */
	TposeInputFile* inputFile = tposeIOOpenInputFile(inputFilePath, delimiter, mutateHeader);
	TposeOutputFile* outputFile = tposeIOOpenOutputFile(outputFilePath, delimiter);
	TposeQuery* tposeQuery = tposeIOQueryAlloc(inputFile, outputFile, idArg, groupArg, numericArg);
	
	// Transpose Simple
	if(!groupFlag && !numericFlag && !idFlag) {
		tposeIOTransposeSimple(tposeQuery);
	}

	// Transpose Group
	if(groupFlag && numericFlag && !idFlag) {
		BTree* btree = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
		tposeIOgetUniqueGroups(tposeQuery, btree);
		tposeIOTransposeGroup(tposeQuery, btree);
		tposeIOPrintOutput(tposeQuery);
		btreeFree(&btree);
	}
	
	// Transpose Group Id
	if(groupFlag && numericFlag && idFlag) {
		BTree* btree = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
		tposeIOgetUniqueGroups(tposeQuery, btree);
		tposeIOTransposeGroupId(tposeQuery, btree);
		btreeFree(&btree);
	}
	
	//Clean-up
	tposeIOCloseInputFile(inputFile);
	tposeIOCloseOutputFile(outputFile);
	tposeIOQueryFree(&tposeQuery);
	
	exit(EXIT_SUCCESS);
}


