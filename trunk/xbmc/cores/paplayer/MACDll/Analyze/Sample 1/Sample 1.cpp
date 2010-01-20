/***************************************************************************************
Analyze - Sample 1
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example opens an APE file and displays some basic information about it. To use it,
just type Sample 1.exe followed by a file name and it'll display information about that
file.

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
#include "all.h"
#include "stdio.h"
#include "maclib.h"
#include "apetag.h"

int main(int argc, char* argv[]) 
{
	///////////////////////////////////////////////////////////////////////////////
	// error check the command line parameters
	///////////////////////////////////////////////////////////////////////////////
	if (argc != 2) 
	{
		printf("~~~Improper Usage~~~\r\n\r\n");
		printf("Usage Example: Sample 1.exe 'c:\\1.ape'\r\n\r\n");
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// variable declares
	///////////////////////////////////////////////////////////////////////////////
	int					nRetVal = 0;										// generic holder for return values
	char				cTempBuffer[256]; ZeroMemory(&cTempBuffer[0], 256);	// generic buffer for string stuff
	char *				pFilename = argv[1];								// the file to open
	IAPEDecompress *	pAPEDecompress = NULL;								// APE interface
		
	///////////////////////////////////////////////////////////////////////////////
	// open the file and error check
	///////////////////////////////////////////////////////////////////////////////
	pAPEDecompress = CreateIAPEDecompress(pFilename, &nRetVal);
	if (pAPEDecompress == NULL) 
	{
		printf("Error opening APE file. (error code %d)\r\n\r\n", nRetVal);
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// display some information about the file
	///////////////////////////////////////////////////////////////////////////////
	printf("Displaying information about '%s':\r\n\r\n", pFilename);

	// file format information
	printf("File Format:\r\n");
	printf("\tVersion: %.2f\r\n", float(pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION)) / float(1000));
	switch (pAPEDecompress->GetInfo(APE_INFO_COMPRESSION_LEVEL))
	{
		case COMPRESSION_LEVEL_FAST: printf("\tCompression level: Fast\r\n\r\n"); break;
		case COMPRESSION_LEVEL_NORMAL: printf("\tCompression level: Normal\r\n\r\n"); break;
		case COMPRESSION_LEVEL_HIGH: printf("\tCompression level: High\r\n\r\n"); break;
		case COMPRESSION_LEVEL_EXTRA_HIGH: printf("\tCompression level: Extra High\r\n\r\n"); break;
	}

	// audio format information
	printf("Audio Format:\r\n");
	printf("\tSamples per second: %d\r\n", pAPEDecompress->GetInfo(APE_INFO_SAMPLE_RATE));
	printf("\tBits per sample: %d\r\n", pAPEDecompress->GetInfo(APE_INFO_BITS_PER_SAMPLE));
	printf("\tNumber of channels: %d\r\n", pAPEDecompress->GetInfo(APE_INFO_CHANNELS));
	printf("\tPeak level: %d\r\n\r\n", pAPEDecompress->GetInfo(APE_INFO_PEAK_LEVEL));

	// size and duration information
	printf("Size and Duration:\r\n");
	printf("\tLength of file (s): %d\r\n", pAPEDecompress->GetInfo(APE_INFO_LENGTH_MS) / 1000);
	printf("\tFile Size (kb): %d\r\n\r\n", pAPEDecompress->GetInfo(APE_INFO_APE_TOTAL_BYTES) / 1024);
	
	// tag information
	printf("Tag Information:\r\n");
	
	CAPETag * pAPETag = (CAPETag *) pAPEDecompress->GetInfo(APE_INFO_TAG);
	BOOL bHasID3Tag = pAPETag->GetHasID3Tag();
	BOOL bHasAPETag = pAPETag->GetHasAPETag();
	
	if (bHasID3Tag || bHasAPETag)
	{
		// iterate through all the tag fields
		BOOL bFirst = TRUE;
		CAPETagField * pTagField;
		while (pAPETag->GetNextTagField(bFirst, &pTagField))
		{
			bFirst = FALSE;
			
			// output the tag field properties (don't output huge fields like images, etc.)
			if (pTagField->GetFieldValueSize() > 128)
			{
				printf("\t%s: --- too much data to display ---\r\n", pTagField->GetFieldName());
			}
			else
			{
				printf("\t%s: %s\r\n", pTagField->GetFieldName(), pTagField->GetFieldValue());
			}
		}
	}
	else 
	{
		printf("\tNot tagged\r\n\r\n");
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// cleanup (just delete the object
	///////////////////////////////////////////////////////////////////////////////
	delete pAPEDecompress;

	///////////////////////////////////////////////////////////////////////////////
	// quit
	///////////////////////////////////////////////////////////////////////////////
	return 0;
}
