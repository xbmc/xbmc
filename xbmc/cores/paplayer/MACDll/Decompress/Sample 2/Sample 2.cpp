/***************************************************************************************
Decompress - Sample 2
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example illustrates using MACLib.lib to do decoding and seeking of an APE file.  
The library manages all seeking and buffering, so you simply seek
to a location and ask for the amount of data required.  (Seek() and GetData())

GetData will return the amount requested unless you're at the end of the file. (in
which case it returns everything up to the end)  It's typically a good idea to check
the return value of each decode.  A value other than ERROR_SUCCESS (0) means errors
were encountered.  In these cases, the decoder will do the best job it can to keep
returning valid data. (corrupt data will be converted to silence)

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
	everything gets finalized.  Use them at your own risk
***************************************************************************************/

// includes
#include <windows.h>
#include "stdio.h"
#include "maclib.h"

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
		printf("~~~Improper Usage~~~\r\n\r\n");
		printf("Usage Example: Sample 2.exe \"c:\\1.ape\"\r\n\r\n");
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// variable declares
	///////////////////////////////////////////////////////////////////////////////
	int nPercentageDone = 0;	// the percentage done... continually updated by the decoder
	int nKillFlag = 0;			// the kill flag for controlling the decoder
	int nRetVal = 0;			// generic holder for return values
	char * pFilename = argv[1];	// the file to open
	
	///////////////////////////////////////////////////////////////////////////////
	// open the APE file
	///////////////////////////////////////////////////////////////////////////////
	IAPEDecompress * pAPEDecompress = CreateIAPEDecompress(pFilename, &nRetVal);
	if (pAPEDecompress == NULL)
	{
		printf("Error opening APE file (error code: %d)\r\n", nRetVal);
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	// allocate space for the raw decompressed data
	///////////////////////////////////////////////////////////////////////////////
	char * pRawData = new char [1024 * pAPEDecompress->GetInfo(APE_INFO_BLOCK_ALIGN)];
	if (pRawData == NULL) { return -1; }

	///////////////////////////////////////////////////////////////////////////////
	// ask for data at a a few random locations and display the sum of 1024 blocks
	///////////////////////////////////////////////////////////////////////////////
	for (int z = 0; z < 16; z++) 
	{
		// figure a random location in the file and seek to it
		int nRandomBlock = rand() % (pAPEDecompress->GetInfo(APE_DECOMPRESS_TOTAL_BLOCKS) - 1024);
		if (pAPEDecompress->Seek(nRandomBlock) != 0) { return -1; }

		// decompress 1024 blocks from that location
		int nBlocksRetrieved;
		nRetVal = pAPEDecompress->GetData(pRawData, 1024, &nBlocksRetrieved);
		if (nRetVal != ERROR_SUCCESS)
		{
			printf("Decoding error (%d)\r\n", nRetVal);
		}
		
		// figure the sum of the decoded data
		int nBytesRetrieved = nBlocksRetrieved * pAPEDecompress->GetInfo(APE_INFO_BLOCK_ALIGN);		
		int nSum = 0;
		for (int x = 0; x < nBytesRetrieved; x++) 
		{
			nSum += pRawData[x];
		}

		// display the sum
		printf("Block / Sum = %.4d / %d\r\n", nRandomBlock, nSum);

	}

	///////////////////////////////////////////////////////////////////////////////
	// clean-up and quit
	///////////////////////////////////////////////////////////////////////////////
	delete pAPEDecompress;
	return 0;
}