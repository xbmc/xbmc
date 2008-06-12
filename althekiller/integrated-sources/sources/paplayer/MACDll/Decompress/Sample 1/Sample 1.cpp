/***************************************************************************************
Decompress - Sample 1
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example illustrates using MACDLib.lib and the simple IAPESimple class to verify
or decompress a file. (verify and decompress work the same, except that verify doesn't
have an output file)  Also, shows how to use a callback to display the progress.

General Notes:
	-the terminology "Sample" refers to a single sample value, and "Block" refers 
	to a collection	of "Channel" samples.  For simplicity, MAC typically uses blocks
	everywhere so that channel mis-alignment cannot happen.

Notes for use in a new project:
	-you need to include "MACLib.lib" in the included libraries list
	-life will be easier if you set the [MAC SDK]\\Shared directory as an include 
	directory and an additional library input path in the project settings
	-set the runtime library to "Mutlithreaded"

WARNING:
	-This class driven system for using Monkey's Audio is still in development, so
	I can't make any guarantees that the classes and libraries won't change before
	everything gets finalized.  Use them at your own risk.
***************************************************************************************/

// includes
#include <windows.h>
#include "stdio.h"
#include "maclib.h"

// global variables (evil... but alright for a simple sample)
unsigned __int32 g_nInitialTickCount;

/***************************************************************************************
Progress callback
***************************************************************************************/
void CALLBACK ProgressCallback(int nPercentageDone)
{
	double dProgress = double(nPercentageDone) / 1000;
	double dElapsedMS = (GetTickCount() - g_nInitialTickCount);

	double dSecondsRemaining = (((dElapsedMS * 100) / dProgress) - dElapsedMS) / 1000;
	printf("Progress: %.1f%% (%.1f seconds remaining)          \r", dProgress, dSecondsRemaining);	
}

/***************************************************************************************
Main (the main function)
***************************************************************************************/
int main(int argc, char* argv[]) 
{
	///////////////////////////////////////////////////////////////////////////////
	// error check the command line parameters
	///////////////////////////////////////////////////////////////////////////////
	if (argc != 2) 
	{
		printf("~~~Improper Usage~~~\n\n");
		printf("Usage Example: Sample 1.exe \"c:\\1.ape\"\n\n");
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// variable declares
	///////////////////////////////////////////////////////////////////////////////
	int nPercentageDone = 0;		//the percentage done... continually updated by the decoder
	int nKillFlag = 0;				//the kill flag for controlling the decoder
	int nRetVal = 0;				//generic holder for return values
	char * pFilename = argv[1];		//the file to open

	///////////////////////////////////////////////////////////////////////////////
	// attempt to verify the file
	///////////////////////////////////////////////////////////////////////////////

	// set the start time and display the starting message
	g_nInitialTickCount = GetTickCount();
	printf("Verifying '%s'...\n", pFilename);

	// do the verify (call unmac.dll)
	nRetVal = VerifyFile(pFilename, &nPercentageDone, ProgressCallback, &nKillFlag);

	// process the return value
	if (nRetVal == 0) 
		printf("\nPassed...\n");
	else 
		printf("\nFailed (error: %d)\n", nRetVal);

	///////////////////////////////////////////////////////////////////////////////
	// quit
	///////////////////////////////////////////////////////////////////////////////
	return 0;
}