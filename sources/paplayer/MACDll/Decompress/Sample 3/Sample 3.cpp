/***************************************************************************************
Decompress - Sample 3
Copyright (C) 2000-2001 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example illustrates dynamic linkage to MACDll.dll to decompress a whole file
and display a simple checksum. (see Sample 1 and Sample 2 for more info)

General Notes:
	-the terminology "Sample" refers to a single sample value, and "Block" refers 
	to a collection	of "Channel" samples.  For simplicity, MAC typically uses blocks
	everywhere so that channel mis-alignment cannot happen.

Notes for use in a new project:
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
#include "MACDll.h"

/***************************************************************************************
MAC_DLL structure (holds function pointers)
***************************************************************************************/
struct MAC_DLL
{
	// APEDecompress functions
	proc_APEDecompress_Create			Create;
	proc_APEDecompress_Destroy			Destroy;
	proc_APEDecompress_GetData			GetData;
	proc_APEDecompress_Seek				Seek;
	proc_APEDecompress_GetInfo			GetInfo;
};

/***************************************************************************************
GetFunctions - helper that gets the function addresses for the functions we need
***************************************************************************************/
int GetFunctions(HMODULE hMACDll, MAC_DLL * pMACDll)
{
	// clear
	memset(pMACDll, 0, sizeof(MAC_DLL));

	// load the functions
	if (hMACDll != NULL)
	{	
		pMACDll->Create = (proc_APEDecompress_Create) GetProcAddress(hMACDll, "c_APEDecompress_Create");
		pMACDll->Destroy = (proc_APEDecompress_Destroy) GetProcAddress(hMACDll, "c_APEDecompress_Destroy");
		pMACDll->GetData = (proc_APEDecompress_GetData) GetProcAddress(hMACDll, "c_APEDecompress_GetData");
		pMACDll->Seek = (proc_APEDecompress_Seek) GetProcAddress(hMACDll, "c_APEDecompress_Seek");
		pMACDll->GetInfo = (proc_APEDecompress_GetInfo) GetProcAddress(hMACDll, "c_APEDecompress_GetInfo");
	}

	// error check
	if ((pMACDll->Create == NULL) ||
		(pMACDll->Destroy == NULL) ||
		(pMACDll->GetData == NULL) ||
		(pMACDll->Seek == NULL) ||
		(pMACDll->GetInfo == NULL))
	{
		return -1;
	}

	return 0;
}

/***************************************************************************************
Version checks the dll / interface
***************************************************************************************/
int VersionCheckInterface(HMODULE hMACDll)
{
	int nRetVal = -1;
	proc_GetInterfaceCompatibility GetInterfaceCompatibility = (proc_GetInterfaceCompatibility) GetProcAddress(hMACDll, "GetInterfaceCompatibility");
	if (GetInterfaceCompatibility)
	{
		nRetVal = GetInterfaceCompatibility(MAC_VERSION_NUMBER, TRUE, NULL);
	}

	return nRetVal;
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
		printf("~~~Improper Usage~~~\r\n\r\n");
		printf("Usage Example: Sample 3.exe \"c:\\1.ape\"\r\n\r\n");
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// variable declares
	///////////////////////////////////////////////////////////////////////////////
	int nRetVal = 0;			// generic holder for return values
	char * pFilename = argv[1];	// the file to open

	///////////////////////////////////////////////////////////////////////////////
	// load MACDll.dll and get the functions
	///////////////////////////////////////////////////////////////////////////////

	// load the DLL
	HMODULE hMACDll = LoadLibrary("MACDll.dll");
	if (hMACDll == NULL) 
		return -1;
	
	// always check the interface version (so we don't crash if something changed)
	if (VersionCheckInterface(hMACDll) != 0)
	{
		FreeLibrary(hMACDll);
		return -1;
	}

	// get the functions
	MAC_DLL MACDll; 
	if (GetFunctions(hMACDll, &MACDll) != 0)
	{
		FreeLibrary(hMACDll);
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	// create the APEDecompress object for the file
	///////////////////////////////////////////////////////////////////////////////
	
	APE_DECOMPRESS_HANDLE hAPEDecompress = MACDll.Create(pFilename, &nRetVal);
	if (hAPEDecompress == NULL)
	{
		printf("Error opening APE file (error code: %d)\r\n", nRetVal);
		FreeLibrary(hMACDll);
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	// calculate a byte-level checksum of the whole file
	///////////////////////////////////////////////////////////////////////////////
	
	// make a buffer to hold 1024 blocks of audio data
	int nBlockAlign = MACDll.GetInfo(hAPEDecompress, APE_INFO_BLOCK_ALIGN, 0, 0);
	unsigned char * pBuffer = new unsigned char [1024 * nBlockAlign];
	
	// loop through the whole file
	int nTotalBlocks = MACDll.GetInfo(hAPEDecompress, APE_DECOMPRESS_TOTAL_BLOCKS, 0, 0);
	int nBlocksRetrieved = 1;
	int nTotalBlocksRetrieved = 0;
	unsigned int nChecksum = 0;
	while (nBlocksRetrieved > 0)
	{
		// try to decompress 1024 blocks
		nRetVal = MACDll.GetData(hAPEDecompress, (char *) pBuffer, 1024, &nBlocksRetrieved);
		if (nRetVal != 0)
			printf("Decompression error (continuing with checksum, but file is probably corrupt)\r\n");

		// calculate the sum (byte-by-byte)
		for (int z = 0; z < (nBlockAlign * nBlocksRetrieved); z++)
		{
			nChecksum += abs(int(pBuffer[z]));
		}

		nTotalBlocksRetrieved += nBlocksRetrieved;

		// output the progress
		printf("Progress: %.1f%%          \r", (float(nTotalBlocksRetrieved) * float(100)) / float(max(nTotalBlocks, 1.0)));	
	}

	delete [] pBuffer;

	// output the result
	printf("Progress: done.                          \r\n");
	printf("Stupid-style Checksum: 0x%X\r\n", nChecksum);

	///////////////////////////////////////////////////////////////////////////////
	// clean-up and quit
	///////////////////////////////////////////////////////////////////////////////
	MACDll.Destroy(hAPEDecompress);
	FreeLibrary(hMACDll);
	return 0;
}