/***************************************************************************************
Compress - Sample 2
Copyright (C) 2000-2002 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example illustrates dynamic linkage to MACDll.dll to create an APE file using
on-the-fly encoding.

The IAPECompress interface fully supports on-the-fly encoding.  To use on-
the-fly encoding, be sure to tell the encoder to create the proper WAV header on
decompression.  Also, you need to specify the absolute maximum audio bytes that will
get encoded. (trying to encode more than the limit set on Start() will cause failure)
This maximum is used to allocates space in the seek table at the front of the file.  Currently,
it takes around 8k per hour of CD music, so it isn't a big deal to allocate more than
needed. You can also specify MAX_AUDIO_BYTES_UNKNOWN to allocate as much space as possible. (2 GB)

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
#include <mmreg.h>
#include "stdio.h"
#include "all.h"
#include "MACDll.h"

/***************************************************************************************
MAC_DLL structure (holds function pointers)
***************************************************************************************/
struct MAC_COMPRESS_DLL
{
	// APECompress functions
	proc_APECompress_Create			Create;
	proc_APECompress_Destroy		Destroy;
	proc_APECompress_Start			Start;
	proc_APECompress_AddData		AddData;
	proc_APECompress_Finish			Finish;
};

/***************************************************************************************
GetFunctions - helper that gets the function addresses for the functions we need
***************************************************************************************/
int GetFunctions(HMODULE hMACDll, MAC_COMPRESS_DLL * pMACDll)
{
	// clear
	memset(pMACDll, 0, sizeof(MAC_COMPRESS_DLL));

	// load the functions
	if (hMACDll != NULL)
	{	
		pMACDll->Create = (proc_APECompress_Create) GetProcAddress(hMACDll, "c_APECompress_Create");
		pMACDll->Destroy = (proc_APECompress_Destroy) GetProcAddress(hMACDll, "c_APECompress_Destroy");
		pMACDll->Start = (proc_APECompress_Start) GetProcAddress(hMACDll, "c_APECompress_Start");
		pMACDll->AddData = (proc_APECompress_AddData) GetProcAddress(hMACDll, "c_APECompress_AddData");
		pMACDll->Finish = (proc_APECompress_Finish) GetProcAddress(hMACDll, "c_APECompress_Finish");
	}

	// error check
	if ((pMACDll->Create == NULL) ||
		(pMACDll->Destroy == NULL) ||
		(pMACDll->Start == NULL) ||
		(pMACDll->AddData == NULL) ||
		(pMACDll->Finish == NULL))
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
Fill a WAVEFORMATEX structure
***************************************************************************************/
int FillWaveFormatExStructure(WAVEFORMATEX *pWaveFormatEx, int nSampleRate, int nBitsPerSample, int nChannels)
{
	pWaveFormatEx->cbSize = 0;
	pWaveFormatEx->nSamplesPerSec = nSampleRate;
	pWaveFormatEx->wBitsPerSample = nBitsPerSample;
	pWaveFormatEx->nChannels = nChannels;
	pWaveFormatEx->wFormatTag = 1;

	pWaveFormatEx->nBlockAlign = (pWaveFormatEx->wBitsPerSample / 8) * pWaveFormatEx->nChannels;
	pWaveFormatEx->nAvgBytesPerSec = pWaveFormatEx->nBlockAlign * pWaveFormatEx->nSamplesPerSec;

	return 0;
}


/***************************************************************************************
Main (the main function)
***************************************************************************************/
int main(int argc, char* argv[]) 
{
	///////////////////////////////////////////////////////////////////////////////
	// variable declares
	///////////////////////////////////////////////////////////////////////////////
	int nTotalAudioBytes = 1048576;
	const char cOutputFile[MAX_PATH] = "c:\\Noise.ape";
	int nRetVal;
	
	printf("Creating file: %s\n", cOutputFile);
	
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
	MAC_COMPRESS_DLL MACDll; 
	if (GetFunctions(hMACDll, &MACDll) != 0)
	{
		FreeLibrary(hMACDll);
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	// create and start the encoder
	///////////////////////////////////////////////////////////////////////////////
	
	// set the input WAV format
	WAVEFORMATEX wfeAudioFormat; FillWaveFormatExStructure(&wfeAudioFormat, 44100, 16, 2);
	
	// create the encoder interface
	APE_COMPRESS_HANDLE hAPECompress = MACDll.Create(&nRetVal);
	if (hAPECompress == NULL)
	{
		printf("Error creating encoder (error code: %d)\r\n", nRetVal);
		FreeLibrary(hMACDll);
		return -1;
	}
	
	// start the encoder
	nRetVal = MACDll.Start(hAPECompress, cOutputFile, &wfeAudioFormat, nTotalAudioBytes, 
		COMPRESSION_LEVEL_HIGH, NULL, CREATE_WAV_HEADER_ON_DECOMPRESSION);

	if (nRetVal != 0)
	{
		printf("Error starting encoder.\n");
		MACDll.Destroy(hAPECompress);
		FreeLibrary(hMACDll);
		return -1;
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// pump through and feed the encoder audio data (white noise for the sample)
	///////////////////////////////////////////////////////////////////////////////
	int nAudioBytesLeft = nTotalAudioBytes;
	unsigned char cBuffer[1024];

	while (nAudioBytesLeft > 0)
	{
		// fill the buffer with white noise
		for (int z = 0; z < 1024; z++)
			cBuffer[z] = rand() % 255;

		// give the data to MAC 
		// (we can safely add any amount, but of course larger chunks take longer to process)
		int nBytesToAdd = min(1024, nAudioBytesLeft);
		nRetVal = MACDll.AddData(hAPECompress, &cBuffer[0], nBytesToAdd);
	
		if (nRetVal != ERROR_SUCCESS)
			printf("Encoding error (error code: %d)\r\n", nRetVal);

		// update the audio bytes left
		nAudioBytesLeft -= nBytesToAdd;
	}

	///////////////////////////////////////////////////////////////////////////////
	// finalize the file (could append a tag, or WAV terminating data)
	///////////////////////////////////////////////////////////////////////////////
	if (MACDll.Finish(hAPECompress, NULL, 0, 0) != 0)
	{
		printf("Error finishing encoder.\n");
	}

	///////////////////////////////////////////////////////////////////////////////
	// clean up and quit
	///////////////////////////////////////////////////////////////////////////////
	MACDll.Destroy(hAPECompress);
	FreeLibrary(hMACDll);
	printf("Done.\n");
	return 0;
}