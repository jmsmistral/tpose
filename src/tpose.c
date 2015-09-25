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
static const char* shortopts = "d:iPp:s:a:I:G:N:hv";
static const struct option longopts[] = {
	{"delimiter", required_argument, NULL, 'd'}
	,{"indexed", no_argument, NULL, 'i'}
	,{"parallel", no_argument, NULL, 'P'}
	,{"prefix", required_argument, NULL, 'p'}
	,{"suffix", required_argument, NULL, 's'}
	,{"aggregate", required_argument, NULL, 'a'}
	//,{"keep", required_argument, NULL, 'k'}
	,{"id", required_argument, NULL, 'I'}
	,{"group", required_argument, NULL, 'G'}
	,{"numeric", required_argument, NULL, 'N'}
	,{"help", no_argument, NULL, 'h'}
	,{"version", no_argument, NULL, 'v'}
	,{NULL, 0, NULL, 0}
};


/* Global declarations */
char* prefixGlobal = "";
char* suffixGlobal = "";

/* Static declarations */
static unsigned char delimiter;

/* Forward declarations */
static void printHelp (int status);
static void printVersion (int status);
static void printContact (int status);



int main(
	int argc
	,char* argv[]
) {

	set_program_name(argv[0]);
	atexit(close_stdout);

	int delimiterFlag = 0;
	int indexedFlag = 0;
	int parallelFlag = 0;
	int prefixFlag = 0;
	int suffixFlag = 0;
	int aggregateFlag = 0;
	int idFlag = 0;
	int groupFlag = 0;
	int numericFlag = 0;
	int helpFlag = 0;
	int versionFlag = 0;
	char* delimiterArg = NULL;
	char* prefixArg = NULL;
	char* suffixArg = NULL;
	char* aggregateArg = NULL;
	char* idArg = NULL;
	char* groupArg = NULL;
	char* numericArg = NULL;
	int idIndexedArg = -1;
	int groupIndexedArg = -1;
	int numericIndexedArg = -1;

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
			case 'i':
				indexedFlag = 1;
				break;
			case 'P':
				parallelFlag = 1;
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
				aggregateArg = strdup(optarg);
				break;
			case 'I':
				idFlag = 1;
				idArg = strdup(optarg);
				break;
			case 'G':
				groupFlag = 1;
				groupArg = strdup(optarg);
				break;
			case 'N':
				numericFlag = 1;
				numericArg = strdup(optarg);
				break;
			case 'h':
				printHelp(0);
				helpFlag = 1;
				exit (EXIT_SUCCESS);
				break;
			case 'v':
				printVersion(0);
				versionFlag = 1;
				exit (EXIT_SUCCESS);
				break;
			case '?':
				fprintf(stderr, "Missing option or argument.\n");
				printHelp(1);
				exit(EXIT_FAILURE);
			default:
				printHelp(1);
				exit(EXIT_FAILURE);
		}
	}

	/* Get input/output file */
	if(optind < argc) {
		for (iArg = optind; iArg < argc; ++iArg) {

			if(fileArg == 0)
				inputFilePath = argv[iArg];
			else if(fileArg == 1)
				outputFilePath = argv[iArg];
			else {
				fprintf(stderr, "Too many files specified!\n");
				printHelp(1);
				exit(EXIT_FAILURE);
			}
			
			++fileArg;
		}
	}
	else {
		fprintf(stderr, "Missing input file.\n");
		printHelp(1);
		exit(EXIT_FAILURE);
	}



	/* Run checks on option arguments */
	// Check delimiter (default = \t)
	delimiter = '\0';
	if(delimiterArg != NULL) {

		if (delimiterArg[0] != '\0' && delimiterArg[1] != '\0') {
			fprintf(stderr, "The delimiter must be a single character\n");
			exit(EXIT_FAILURE);
		}

		delimiter = (char) delimiterArg[0];
		delimiterSpecified = true;
	}

	if (!delimiterSpecified) {
		//fprintf(stderr, "No field delimiter character specified, using TAB as default\n");
		delimiter = '\t';
	}


	// Process id, group, numeric options depending on non-indexed/indexed
	if(!indexedFlag) {
		tposeIOLowerCase(idArg);
		tposeIOLowerCase(groupArg);
		tposeIOLowerCase(numericArg);
 	}
	else {
		
		if( idFlag && ((idIndexedArg = stringToInteger(idArg)) == -1)) {
			fprintf(stderr, "--indexed option requires a positive integer value for id field\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}

		if( groupFlag && ((groupIndexedArg = stringToInteger(groupArg)) == -1)) {
			fprintf(stderr, "--indexed option requires a positive integer value for group field\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}

		if( numericFlag && ((numericIndexedArg = stringToInteger(numericArg)) == -1)) {
			fprintf(stderr, "--indexed option requires a postive integer value for numeric option\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}
		
	}
	
	// Set prefix/suffix
	if(prefixFlag) { 
		prefixGlobal = prefixArg;
	}

	if(suffixFlag) { 
		suffixGlobal = suffixArg;
	}
	
	// Check aggregation type
	if(aggregateFlag) {
		if(strcmp("sum", tposeIOLowerCase(aggregateArg)) && strcmp("count", tposeIOLowerCase(aggregateArg)) && strcmp("avg", tposeIOLowerCase(aggregateArg))) {
			fprintf(stderr, "-a or --aggregate option requires either 'sum', 'count', or 'avg' to be passed\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}
	}


	// Check that option dependencies have been specified
	if(groupFlag && !numericFlag) {
		fprintf(stderr, "NUMERIC field needs to be specified (see --numeric option)\n");
		printHelp(1);
		exit(EXIT_FAILURE);
	}

	if(idFlag && !groupFlag && !numericFlag) {
		fprintf(stderr, "GROUP and NUMERIC fields need to be specified (see --group, and --numeric options)\n");
		printHelp(1);
		exit(EXIT_FAILURE);
	}
	
	// Check output file (if empty, use stdout)
	if(!outputFilePath) {
		outputFilePath = "stdout";
	}

	unsigned int mutateHeader = 1;
	if(!groupFlag && !numericFlag && !idFlag)
		mutateHeader = 0; // Don't need to read header for simple transpose
	

	/* Core */
	TposeInputFile* inputFile;
	if((inputFile = tposeIOOpenInputFile(inputFilePath, delimiter, mutateHeader)) == NULL) {
			exit(EXIT_FAILURE);
	}
	TposeOutputFile* outputFile;
	if((outputFile = tposeIOOpenOutputFile(outputFilePath, "wa", delimiter)) == NULL) {
			exit(EXIT_FAILURE);
	}

	// Create query
	TposeQuery* tposeQuery;
	if(!indexedFlag) {
		if((tposeQuery = tposeIOQueryAlloc(inputFile, outputFile, idArg, groupArg, numericArg, aggregateArg)) == NULL) {
			fprintf(stderr, "--id, --group, or --numeric parameters do not match input fields\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}
	}
	else {
		if((tposeQuery = tposeIOQueryIndexedAlloc(inputFile, outputFile, idIndexedArg, groupIndexedArg, numericIndexedArg, aggregateArg)) == NULL) {
			fprintf(stderr, "--id, --group, or --numeric parameters do not match input fields\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}
	}

	// Transpose Simple
	if(!groupFlag && !numericFlag && !idFlag) {
		tposeIOTransposeSimple(tposeQuery);
	}
	// Transpose Group
	if(groupFlag && numericFlag && !idFlag) {
		if((inputFile->fileSize >= TPOSE_IO_CHUNK_SIZE) && parallelFlag) {	
			// Multi-threaded
			btreeGlobal = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
			if(tposeIOBuildPartitions(tposeQuery, TPOSE_IO_PARTITION_GROUP) == -1) {
				fprintf(stderr, "Error reading input file! Make sure it is correctly formed.\n");
				exit(EXIT_FAILURE);
			}
			tposeIOUniqueGroupsParallel(tposeQuery);
			tposeIOTransposeGroupParallel(tposeQuery);
			btreeFree(&btreeGlobal);
		}
		else {
			// Single-threaded
			BTree* btree = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
			tposeIOUniqueGroups(tposeQuery, btree);
			tposeIOTransposeGroup(tposeQuery, btree);
			btreeFree(&btree);
		}
	}
	
	// Transpose Group Id
	if(groupFlag && numericFlag && idFlag) {
		if((inputFile->fileSize >= TPOSE_IO_CHUNK_SIZE) && parallelFlag) {	
			// Multi-threaded
			btreeGlobal = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
			if(tposeIOBuildPartitions(tposeQuery, TPOSE_IO_PARTITION_ID) == -1) {
				fprintf(stderr, "Error reading input file! Make sure it is correctly formed.\n");
				exit(EXIT_FAILURE);
			}
			tposeIOUniqueGroupsParallel(tposeQuery);
			tposeIOTransposeGroupIdParallel(tposeQuery);
			btreeFree(&btreeGlobal);
		}
		else {
			// Single-threaded
			BTree* btree = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
			tposeIOUniqueGroups(tposeQuery, btree);
			tposeIOTransposeGroupId(tposeQuery, btree);
			btreeFree(&btree);
		}
	}
	
	//Clean-up
	tposeIOCloseInputFile(inputFile);
	tposeIOCloseOutputFile(outputFile);
	tposeIOQueryFree(&tposeQuery);
	
	exit(EXIT_SUCCESS);
}



/* Print tpose usage information */
static void printHelp
(
	int status
) { 

	FILE *out = status ? stderr : stdout;

  fprintf(out, "\n\
Usage: %s input-file [output-file] [--options] \n\n", program_name);

  fprintf(out, "  -d<char>, --delimiter=<char>\
\tspecify field delimiter used to read input file\n");
  fprintf(out, "  -P, --parallel\
\t\tmulti-threaded transpose (files > 1GB)\n");
  fprintf(out, "  -i, --indexed\
\t\t\tuse field indexes (e.g. 1,2,...) instead of names\n");
  fprintf(out, "  -I<field>, --id=<field>\
\tdefines ID field in input (requires --group and --numeric)\n");
  fprintf(out, "  -G<field>, --group=<field>\
\tdefines GROUP field in input (requires --numeric)\n");
  fprintf(out, "  -N<field>, --numeric=<field>\
\tdefines NUMERIC field in input. Aggregated with --aggregate\n");
  fprintf(out, "  -p<string>, --prefix=<string>\
\tprefix transposed fields with string\n");
  fprintf(out, "  -s<string>, --suffix=<string>\
\tsuffix transposed fields with string\n");
  fprintf(out, "  -a<type>, --aggregate=<type>\
\taggregate NUMERIC values. Can be 'sum', 'count', or 'avg'.\n\
\t\t\t\tRequires --numeric to be specified (Default = 'sum')\n");
  fprintf(out, "  -h, --help\
\t\t\tdisplay this help and exit\n");
  fprintf(out, "  -v, --version\
\t\t\toutput version information and exit\n\n");

  printContact(status);

  fclose(out);
  exit(status);

}



/* Print tpose usage information */
static void printContact
(
	int status
) {

	char* email = "jmsmistral@gmail.com";
	FILE *out = status ? stderr : stdout;

	fprintf(out, "Examples can be found at the tpose home page below.\n\n");

	fprintf(out, "tpose home page: <https://www.bitbucket.org/jmsmistral/tpose>.\n");

	fprintf(out, "E-mail bug reports to: Jonathan Sacramento <%s>.\n\
Be sure to include the word ''TPOSE BUG'' somewhere in the ''Subject:'' field.\n", email);

}

static void printVersion
(
	int status
) {

	FILE *out = status ? stderr : stdout;

	fprintf(out, "tpose version %s\n", TPOSE_VERSION);
	fprintf(out, "Copyright (C) Jonathan Sacramento\n"); //, (wchar_t)0xA9);
	fprintf(out, "This is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 3, or (at your option)\n\
any later version;\n\n\
See source code for copying conditions.\n\
There is NO WARRANTY; not even for MERCHANTABILITY or \n\
FITNESS FOR A PARTICULAR PURPOSE, to the extent permitted by law.\n\n");

	printContact(status);

}
