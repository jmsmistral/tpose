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
static const char* shortopts = "d:ip:s:a:hv";
static const struct option longopts[] = {
	{"delimiter", required_argument, NULL, 'd'}
	,{"indexed", no_argument, NULL, 'i'}
	//,{"unbuffered", no_argument, NULL, 'u'}
	,{"prefix", required_argument, NULL, 'p'}
	,{"suffix", required_argument, NULL, 's'}
	,{"aggregate", required_argument, NULL, 'a'}
	//,{"keep", required_argument, NULL, 'k'}
	,{"id", required_argument, NULL, 'x'}
	,{"group", required_argument, NULL, 'y'}
	,{"numeric", required_argument, NULL, 'z'}
	,{"help", no_argument, NULL, 'h'}
	,{"version", no_argument, NULL, 'v'}
	,{NULL, 0, NULL, 0}
};

/* Forward declarations */
static void printHelp (int status);
static void printVersion (int status);
static void printContact (int status);

static unsigned char delimiter;

int main(
	int argc
	,char* argv[]
) {

	set_program_name(argv[0]);
	atexit(close_stdout);

	int delimiterFlag = 0;
	int indexedFlag = 0;
	//int unbufferedFlag = 0;
	int prefixFlag = 0;
	int suffixFlag = 0;
	int aggregateFlag = 0;
	//int keepFlag = 0;
	int idFlag = 0;
	int groupFlag = 0;
	int numericFlag = 0;
	int helpFlag = 0;
	int versionFlag = 0;
	char* delimiterArg = NULL;
	char* prefixArg = NULL;
	char* suffixArg = NULL;
	char* aggregateArg = NULL;
	//char* keepArg = NULL;
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
			/*case 'u':
				unbufferedFlag = 1;
				break;*/
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
			/*case 'k':
				keepFlag = 1;
				keepArg = optarg;
				break;*/
			case 'x':
				idFlag = 1;
				idArg = strdup(optarg);
				break;
			case 'y':
				groupFlag = 1;
				groupArg = strdup(optarg);
				break;
			case 'z':
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
				/*if (optopt == 'c')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr, "Unknown option '-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
				return 1; */
				fprintf(stderr, "Missing option or argument. optopt = %c\n", optopt);
				printHelp(1);
				abort;
			default:
				printHelp(1);
				abort();
		}
	}

	/* Get input/output file */
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
				printHelp(1);
				abort();
			}
			
			++fileArg;
		}
	}
	else {
		fprintf(stderr, "Missing input file.\n");
		printHelp(1);
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
		//fprintf(stdout, "No field delimiter character specified, using TAB as default\n");
		delimiter = '\t';
	}

	/*printf("DELIMITER = '%c'\n", delimiter); */

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
		
		/*printf("idIndexedArg = %d\n", idIndexedArg);
		printf("groupIndexedArg = %d\n", groupIndexedArg);
		printf("numericIndexedArg = %d\n", numericIndexedArg);*/

	}
	


	// Check that option dependencies have been specified
	if(groupFlag && !numericFlag) {
		fprintf(stderr, "NUMERIC field needs to be specified (see --numeric option)\n");
		printHelp(1);
		abort();
	}

	if(idFlag && !groupFlag && !numericFlag) {
		fprintf(stderr, "GROUP and NUMERIC fields need to be specified (see --group, and --numeric options)\n");
		printHelp(1);
		abort();
	}
	
	// Check output file (if empty, use stdout)
	if(!outputFilePath) {
		outputFilePath = "stdout";
	}

	unsigned int mutateHeader = 1;
	if(!groupFlag && !numericFlag && !idFlag)
		mutateHeader = 0; // Don't need to read header for simple transpose
	
	/*printf("input: %s\n", inputFilePath); 
	printf("output: %s\n", outputFilePath); */



	/* Event-loop */
	TposeInputFile* inputFile = tposeIOOpenInputFile(inputFilePath, delimiter, mutateHeader);
	TposeOutputFile* outputFile = tposeIOOpenOutputFile(outputFilePath, delimiter);

	// Create query
	TposeQuery* tposeQuery;
	if(!indexedFlag) {
		//printf("non-indexed\n");
		if((tposeQuery = tposeIOQueryAlloc(inputFile, outputFile, idArg, groupArg, numericArg)) == NULL) {
			fprintf(stderr, "--id, --group, or --numeric parameters do not match input fields\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}
	}
	else {
		//printf("indexed\n");
		if((tposeQuery = tposeIOQueryIndexedAlloc(inputFile, outputFile, idIndexedArg, groupIndexedArg, numericIndexedArg)) == NULL) {
			fprintf(stderr, "--id, --group, or --numeric parameters do not match input fields\n");
			printHelp(1);
			exit(EXIT_FAILURE);
		}
	}

	
	// Transpose Simple
	if(!groupFlag && !numericFlag && !idFlag) {
		//printf("Transpose Simple\n");
		tposeIOTransposeSimple(tposeQuery);
	}

	// Transpose Group
	if(groupFlag && numericFlag && !idFlag) {
		//printf("Transpose Group\n");
		BTree* btree = btreeAlloc(); // Needs to persist between computing unique groups, and aggregating values
		tposeIOgetUniqueGroups(tposeQuery, btree);
		tposeIOTransposeGroup(tposeQuery, btree);
		tposeIOPrintOutput(tposeQuery);
		btreeFree(&btree);
	}
	
	// Transpose Group Id
	if(groupFlag && numericFlag && idFlag) {
		//printf("Transpose Group Id\n");
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



/* Print tpose usage information */
static void printHelp
(
	int status
) { 

	FILE *out = status ? stderr : stdout;

  fprintf(out, "\n\
Usage: %s input-file [output-file] [--options] \n\n", program_name);

  fprintf(out, "  -d<string>, --delimiter=<string>\n\
\t\tspecify field delimiter used to read input file\n");
  fprintf(out, "  -i, --indexed\n\
\t\tuse field indexes (e.g. 1,2,...,<max-fields>) \
to match fields, instead of field names\n");
  fprintf(out, "  -p<string>, --prefix=<string>\n\
\t\tprefix new field names with given string\n");
  fprintf(out, "  -s<string>, --suffix=<string>\n\
\t\tsuffix new field names with given string\n");
  fprintf(out, "  -a<type>, --aggregate=<type>\n\
\t\taggregates numeric values according to <aggregate-type> passed.\n\
\t\tcan be either 'SUM', 'COUNT', or 'AVG' (Default = 'SUM')\n\
\t\t(note: requires --numeric to be specified\n");
  fprintf(out, "  --id=<field-name>\n\
\t\tdefines ID field in input file (note: requires both --group, and --numeric\n");
  fprintf(out, "  --group=<field-name>\n\
\t\tdefines GROUP field in input file.\n\
\t\tThis field stores he group values to transpose\n\
\t\tthe NUMERIC field values over (note: requires --numeric)\n");
  fprintf(out, "  --numeric=<field-name>\n\
\t\tdefines NUMERIC field in input file. This field holds the\n\
\t\tnumeric values to be transposed. These values will be\n\
\t\taggregated according to the --aggregate option\n");
  fprintf(out, "  -h, --help\n\
\t\tdisplay this help and exit\n");
  fprintf(out, "  -v, --version\n\
\t\toutput version information and exit\n\n");

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

	fprintf(out, "tpose home page: <https://www.bitbucket.org/jmsmistral/tpose>.\n");

	//if (!status)
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
