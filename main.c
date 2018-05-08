/*
 * Autor: Lubomir Gallovic (xgallo03)
 * Datum: 16.2.2018
 * Soubor: main.c
 * Komentar:
 */ 
#include <stdlib.h>
#include "ahed.h"


void printHelp() {
	printf("Usage: ahed [-i inputFile] [-o outputFile] [-l logFile] [h|c|x]\n");
	printf("   -i inputFile - input file, otherwise strin\n");
	printf("   -o outputFile - output file, otherwise stdout\n");
	printf("   -l logFile - file for log info\n");
	printf("   -h - prints help\n");
	printf("   -c - program codes plaintext input\n");
	printf("   -x - program decodes coded niput into plaintext\n");
}

int main(int argc, char *argv[])
{

	FILE* inputFile = stdin;
	FILE* outputFile = stdout;
	FILE* logFile = NULL;
	int retVal;
	int opt;
	bool code = false;
	bool decode = false;
	bool log = false;

	while ((opt = getopt(argc, argv, "i:o:l:cxh")) != -1) {
		switch (opt) {
		case 'i':
			if ((inputFile = fopen(optarg, "r")) == NULL) {
				printf("Failed to open input file\n");
				inputFile = stdin;
			}
			break;
		case 'o':
			if ((outputFile = fopen(optarg, "w+")) == NULL) {
				printf("Failed to open output file\n");
				outputFile = stdout;
			} 
			break;
		case 'l':
			if ((logFile = fopen(optarg, "w+")) == NULL) {
				printf("Failed to open log file\n");
				return AHEDFail;
			}
			log = true;
			break;
		case 'c':
			code = true;
			break;
		case 'x':
			decode = true;
			break;
		case 'h':
			printHelp();
			return AHEDOK;
			break;
		default:
			printHelp();
			return AHEDFail;
			break;
		}
	}

	if (code && decode) {
		printf("Both coding and decoding operation specified\n");
		return AHEDFail;
	}
	if (!code && !decode) {
		printf("No operation specified\n");
		return AHEDFail;
	}

	tAHED* ahed = malloc(sizeof(tAHED));
	ahed->uncodedSize = 0;
	ahed->codedSize = 0;

	if (code) {
		retVal = AHEDEncoding(ahed, inputFile, outputFile);
		if (retVal == AHEDFail) {
			printf("Encoding failed\n");
			free(ahed);
			return AHEDFail;
		}
	}

	if (decode) {
		retVal = AHEDDecoding(ahed, inputFile, outputFile);
		if (retVal == AHEDFail) {
			printf("Decoding failed\n");
			free(ahed);
			return AHEDFail;
		}
	}

	if (log) {
		fprintf(logFile, "login = xgallo03\n");
		fprintf(logFile, "uncodedSize = %ld\n",ahed->uncodedSize);
		fprintf(logFile, "codedSize = %ld\n",ahed->codedSize);
	}

	free(ahed);

	if (inputFile != NULL) 
		fclose(inputFile);
	if (outputFile != NULL) 
		fclose(outputFile);
	if (logFile != NULL) 
		fclose(logFile);

	return AHEDOK;	
}


	
