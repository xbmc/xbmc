/***************************************************************************************
MAC Console Frontend (MAC.exe)

Pretty simple and straightforward console front end.  If somebody ever wants to add 
more functionality like tagging, auto-verify, etc., that'd be excellent.

Copyrighted (c) 2000 - 2003 Matthew T. Ashland.  All Rights Reserved.
***************************************************************************************/
#include "All.h"
#include <stdio.h>
#include "GlobalFunctions.h"
#include "MACLib.h"
#include "CharacterHelper.h"

#include <wchar.h>

// defines
#define COMPRESS_MODE		0
#define DECOMPRESS_MODE		1
#define VERIFY_MODE			2
#define CONVERT_MODE		3
#define UNDEFINED_MODE		-1

// global variables
TICK_COUNT_TYPE g_nInitialTickCount = 0;

/***************************************************************************************
Displays the proper usage for MAC.exe
***************************************************************************************/
void DisplayProperUsage(FILE * pFile) 
{
	fprintf(pFile, "Proper Usage: [EXE] [Input File] [Output File] [Mode]\n\n");

	fprintf(pFile, "Modes: \n");
	fprintf(pFile, "    Compress (fast): '-c1000'\n");
	fprintf(pFile, "    Compress (normal): '-c2000'\n");
	fprintf(pFile, "    Compress (high): '-c3000'\n");
	fprintf(pFile, "    Compress (extra high): '-c4000'\n");
	fprintf(pFile, "    Compress (insane): '-c5000'\n");
	fprintf(pFile, "    Decompress: '-d'\n");
	fprintf(pFile, "    Verify: '-v'\n");
	fprintf(pFile, "    Convert: '-nXXXX'\n\n");

	fprintf(pFile, "Examples:\n");
	fprintf(pFile, "    Compress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000\n");
	fprintf(pFile, "    Decompress: mac.exe \"Metallica - One.ape\" \"Metallica - One.wav\" -d\n");
	fprintf(pFile, "    Verify: mac.exe \"Metallica - One.ape\" -v\n");
	fprintf(pFile, "    (note: int filenames must be put inside of quotations)\n");
}

/***************************************************************************************
Progress callback
***************************************************************************************/
void CALLBACK ProgressCallback(int nPercentageDone)
{
    // get the current tick count
	TICK_COUNT_TYPE  nTickCount;
	TICK_COUNT_READ(nTickCount);

	// calculate the progress
	double dProgress = nPercentageDone / 1.e5;											// [0...1]
	double dElapsed = (double) (nTickCount - g_nInitialTickCount) / TICK_COUNT_FREQ;	// seconds
	double dRemaining = dElapsed * ((1.0 / dProgress) - 1.0);							// seconds

	// output the progress
	fprintf(stderr, "Progress: %.1f%% (%.1f seconds remaining, %.1f seconds total)          \r", 
		dProgress * 100, dRemaining, dElapsed);
}

/***************************************************************************************
Main (the main function)
***************************************************************************************/
int main(int argc, char * argv[])
{
	// variable declares
	CSmartPtr<wchar_t> spInputFilename; CSmartPtr<wchar_t> spOutputFilename;
	int nRetVal = ERROR_UNDEFINED;
	int nMode = UNDEFINED_MODE;
	int nCompressionLevel;
	int nPercentageDone;
		
	// output the header
	fprintf(stderr, CONSOLE_NAME);
	
	// make sure there are at least four arguments (could be more for EAC compatibility)
	if (argc < 3) 
	{
		DisplayProperUsage(stderr);
		exit(-1);
	}

	// store the input file
	wchar_t temp[1024];
	mbtowc(temp,argv[1],1024);
	spInputFilename.Assign(temp, TRUE);
	
	// store the output file
	mbtowc(temp,argv[2],1024);
	spOutputFilename.Assign(temp, TRUE);

	// verify that the input file exists
	if (!FileExists(spInputFilename))
	{
		fprintf(stderr, "Input File Not Found...\n\n");
		exit(-1);
	}

	// if the output file equals '-v', then use this as the next argument
	char cMode[256];
	strcpy(cMode, argv[2]);

	if (_strnicmp(cMode, "-v", 2) != 0)
	{
		// verify is the only mode that doesn't use at least the third argument
		if (argc < 4) 
		{
			DisplayProperUsage(stderr);
			exit(-1);
		}

		// check for and skip if necessary the -b XXXXXX arguments (3,4)
		strcpy(cMode, argv[3]);
	}

	// get the mode
	nMode = UNDEFINED_MODE;
	if (_strnicmp(cMode, "-c", 2) == 0)
		nMode = COMPRESS_MODE;
	else if (_strnicmp(cMode, "-d", 2) == 0)
		nMode = DECOMPRESS_MODE;
	else if (_strnicmp(cMode, "-v", 2) == 0)
		nMode = VERIFY_MODE;
	else if (_strnicmp(cMode, "-n", 2) == 0)
		nMode = CONVERT_MODE;

	// error check the mode
	if (nMode == UNDEFINED_MODE) 
	{
		DisplayProperUsage(stderr);
		exit(-1);
	}

	// get and error check the compression level
	if (nMode == COMPRESS_MODE || nMode == CONVERT_MODE) 
	{
		nCompressionLevel = atoi(&cMode[2]);
		if (nCompressionLevel != 1000 && nCompressionLevel != 2000 && 
			nCompressionLevel != 3000 && nCompressionLevel != 4000 &&
			nCompressionLevel != 5000) 
		{
			DisplayProperUsage(stderr);
			return -1;
		}
	}

	// set the initial tick count
	TICK_COUNT_READ(g_nInitialTickCount);
	
	// process
	int nKillFlag = 0;
	if (nMode == COMPRESS_MODE) 
	{
		char cCompressionLevel[16];
		if (nCompressionLevel == 1000) { strcpy(cCompressionLevel, "fast"); }
		if (nCompressionLevel == 2000) { strcpy(cCompressionLevel, "normal"); }
		if (nCompressionLevel == 3000) { strcpy(cCompressionLevel, "high"); }
		if (nCompressionLevel == 4000) { strcpy(cCompressionLevel, "extra high"); }
		if (nCompressionLevel == 5000) { strcpy(cCompressionLevel, "insane"); }

		fprintf(stderr, "Compressing (%s)...\n", cCompressionLevel);
		nRetVal = CompressFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);
	}
	else if (nMode == DECOMPRESS_MODE) 
	{
		fprintf(stderr, "Decompressing...\n");
		nRetVal = DecompressFileW(spInputFilename, spOutputFilename, &nPercentageDone, ProgressCallback, &nKillFlag);
	}	
	else if (nMode == VERIFY_MODE) 
	{
		fprintf(stderr, "Verifying...\n");
		nRetVal = VerifyFileW(spInputFilename, &nPercentageDone, ProgressCallback, &nKillFlag);
	}	
	else if (nMode == CONVERT_MODE) 
	{
		fprintf(stderr, "Converting...\n");
		nRetVal = ConvertFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);
	}

	if (nRetVal == ERROR_SUCCESS) 
		fprintf(stderr, "\nSuccess...\n");
	else 
		fprintf(stderr, "\nError: %i\n", nRetVal);

	return nRetVal;
}
