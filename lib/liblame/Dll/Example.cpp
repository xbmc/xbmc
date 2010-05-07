/*
 *	LAME DLL Sample Code.
 *
 *	Copyright (c) 2000 A.L. Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */


#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "BladeMP3EncDLL.h"

BEINITSTREAM		beInitStream=NULL;
BEENCODECHUNK		beEncodeChunk=NULL;
BEDEINITSTREAM		beDeinitStream=NULL;
BECLOSESTREAM		beCloseStream=NULL;
BEVERSION			beVersion=NULL;
BEWRITEVBRHEADER	beWriteVBRHeader=NULL;
BEWRITEINFOTAG		beWriteInfoTag=NULL;


// Main program
int main(int argc, char *argv[])
{	
	HINSTANCE	hDLL			=NULL;
	FILE*		pFileIn			=NULL;
	FILE*		pFileOut		=NULL;
	BE_VERSION	Version			={0,};
	BE_CONFIG	beConfig		={0,};

	CHAR		strFileIn[255]	={'0',};
	CHAR		strFileOut[255]	={'0',};

	DWORD		dwSamples		=0;
	DWORD		dwMP3Buffer		=0;
	HBE_STREAM	hbeStream		=0;
	BE_ERR		err				=0;

	PBYTE		pMP3Buffer		=NULL;
	PSHORT		pWAVBuffer		=NULL;

	// check number of arguments
	if(argc != 2)
	{
		fprintf(stderr,"Usage: %s <filename.wav>\n", argv[0]);
		fprintf(stderr,"Descr: Short demo to show how to use the lame_enc.dll library file\n");
		fprintf(stderr,"Note : WAV file is assumed to to have the following parameters\n");
		fprintf(stderr,"     : 44100 Hz, stereo, 16 Bits per sample\n");
		return -1;
	}

	// Setup the file names
	strcpy(strFileIn ,argv[1]);
	strcpy(strFileOut,argv[1]);

	// Add mp3 extention
	strcat(strFileOut,".mp3");

	// Load lame_enc.dll library (Make sure though that you set the 
	// project/settings/debug Working Directory correctly, otherwhise the DLL can't be loaded

	hDLL = LoadLibrary("lame_enc.dll");

  	if ( NULL == hDLL )
  	{
  		hDLL = LoadLibrary("..\\..\\output\\lame_enc.dll");
  	}

	if( NULL == hDLL )
	{
		fprintf(stderr,"Error loading lame_enc.DLL");
		return -1;
	}

	// Get Interface functions from the DLL
	beInitStream	= (BEINITSTREAM) GetProcAddress(hDLL, TEXT_BEINITSTREAM);
	beEncodeChunk	= (BEENCODECHUNK) GetProcAddress(hDLL, TEXT_BEENCODECHUNK);
	beDeinitStream	= (BEDEINITSTREAM) GetProcAddress(hDLL, TEXT_BEDEINITSTREAM);
	beCloseStream	= (BECLOSESTREAM) GetProcAddress(hDLL, TEXT_BECLOSESTREAM);
	beVersion		= (BEVERSION) GetProcAddress(hDLL, TEXT_BEVERSION);
	beWriteVBRHeader= (BEWRITEVBRHEADER) GetProcAddress(hDLL,TEXT_BEWRITEVBRHEADER);
	beWriteInfoTag  = (BEWRITEINFOTAG) GetProcAddress(hDLL,TEXT_BEWRITEINFOTAG);

	// Check if all interfaces are present
	if(!beInitStream || !beEncodeChunk || !beDeinitStream || !beCloseStream || !beVersion || !beWriteVBRHeader)
	{
		printf("Unable to get LAME interfaces");
		return -1;
	}

	// Get the version number
	beVersion( &Version );

	printf(
			"lame_enc.dll version %u.%02u (%u/%u/%u)\n"
			"lame_enc Engine %u.%02u\n"
			"lame_enc homepage at %s\n\n",	
			Version.byDLLMajorVersion, Version.byDLLMinorVersion,
			Version.byDay, Version.byMonth, Version.wYear,
			Version.byMajorVersion, Version.byMinorVersion,
			Version.zHomepage);

	// Try to open the WAV file, be sure to open it as a binary file!	
	pFileIn = fopen( strFileIn, "rb" );

	// Check file open result
	if(pFileIn == NULL)
	{
		fprintf(stderr,"Error opening %s", argv[1]);
		return -1;
	}

	// Open MP3 file
	pFileOut= fopen(strFileOut,"wb+");

	// Check file open result
	if(pFileOut == NULL)
	{
		fprintf(stderr,"Error creating file %s", strFileOut);
		return -1;
	}

	memset(&beConfig,0,sizeof(beConfig));					// clear all fields

	// use the LAME config structure
	beConfig.dwConfig = BE_CONFIG_LAME;

	// this are the default settings for testcase.wav
	beConfig.format.LHV1.dwStructVersion	= 1;
	beConfig.format.LHV1.dwStructSize		= sizeof(beConfig);		
	beConfig.format.LHV1.dwSampleRate		= 44100;				// INPUT FREQUENCY
	beConfig.format.LHV1.dwReSampleRate		= 0;					// DON"T RESAMPLE
	beConfig.format.LHV1.nMode				= BE_MP3_MODE_JSTEREO;	// OUTPUT IN STREO
	beConfig.format.LHV1.dwBitrate			= 128;					// MINIMUM BIT RATE
	beConfig.format.LHV1.nPreset			= LQP_R3MIX;		// QUALITY PRESET SETTING
	beConfig.format.LHV1.dwMpegVersion		= MPEG1;				// MPEG VERSION (I or II)
	beConfig.format.LHV1.dwPsyModel			= 0;					// USE DEFAULT PSYCHOACOUSTIC MODEL 
	beConfig.format.LHV1.dwEmphasis			= 0;					// NO EMPHASIS TURNED ON
	beConfig.format.LHV1.bOriginal			= TRUE;					// SET ORIGINAL FLAG
	beConfig.format.LHV1.bWriteVBRHeader	= TRUE;					// Write INFO tag

//	beConfig.format.LHV1.dwMaxBitrate		= 320;					// MAXIMUM BIT RATE
//	beConfig.format.LHV1.bCRC				= TRUE;					// INSERT CRC
//	beConfig.format.LHV1.bCopyright			= TRUE;					// SET COPYRIGHT FLAG	
//	beConfig.format.LHV1.bPrivate			= TRUE;					// SET PRIVATE FLAG
//	beConfig.format.LHV1.bWriteVBRHeader	= TRUE;					// YES, WRITE THE XING VBR HEADER
//	beConfig.format.LHV1.bEnableVBR			= TRUE;					// USE VBR
//	beConfig.format.LHV1.nVBRQuality		= 5;					// SET VBR QUALITY
	beConfig.format.LHV1.bNoRes				= TRUE;					// No Bit resorvoir

// Preset Test
//	beConfig.format.LHV1.nPreset			= LQP_PHONE;

	// Init the MP3 Stream
	err = beInitStream(&beConfig, &dwSamples, &dwMP3Buffer, &hbeStream);

	// Check result
	if(err != BE_ERR_SUCCESSFUL)
	{
		fprintf(stderr,"Error opening encoding stream (%lu)", err);
		return -1;
	}


	// Allocate MP3 buffer
	pMP3Buffer = new BYTE[dwMP3Buffer];

	// Allocate WAV buffer
	pWAVBuffer = new SHORT[dwSamples];

	// Check if Buffer are allocated properly
	if(!pMP3Buffer || !pWAVBuffer)
	{
		printf("Out of memory");
		return -1;
	}

	DWORD dwRead=0;
	DWORD dwWrite=0;
	DWORD dwDone=0;
	DWORD dwFileSize=0;

	// Seek to end of file
	fseek(pFileIn,0,SEEK_END);

	// Get the file size
	dwFileSize=ftell(pFileIn);

	// Seek back to start of WAV file,
	// but skip the first 44 bytes, since that's the WAV header
	fseek(pFileIn,44,SEEK_SET);


	// Convert All PCM samples
	while ( (dwRead=fread(pWAVBuffer,sizeof(SHORT),dwSamples,pFileIn)) >0 )
	{
		// Encode samples
		err = beEncodeChunk(hbeStream, dwRead, pWAVBuffer, pMP3Buffer, &dwWrite);

		// Check result
		if(err != BE_ERR_SUCCESSFUL)
		{
			beCloseStream(hbeStream);
			fprintf(stderr,"beEncodeChunk() failed (%lu)", err);
			return -1;
		}
		
		// write dwWrite bytes that are returned in tehe pMP3Buffer to disk
		if(fwrite(pMP3Buffer,1,dwWrite,pFileOut) != dwWrite)
		{
			fprintf(stderr,"Output file write error");
			return -1;
		}

		dwDone += dwRead*sizeof(SHORT);

		printf("Done: %0.2f%%     \r", 100 * (float)dwDone/(float)(dwFileSize));
	}

	// Deinit the stream
	err = beDeinitStream(hbeStream, pMP3Buffer, &dwWrite);

	// Check result
	if(err != BE_ERR_SUCCESSFUL)
	{

		beCloseStream(hbeStream);
		fprintf(stderr,"beExitStream failed (%lu)", err);
		return -1;
	}

	// Are there any bytes returned from the DeInit call?
	// If so, write them to disk
	if( dwWrite )
	{
		if( fwrite( pMP3Buffer, 1, dwWrite, pFileOut ) != dwWrite )
		{
			fprintf(stderr,"Output file write error");
			return -1;
		}
	}

	// close the MP3 Stream
	beCloseStream( hbeStream );

	// Delete WAV buffer
	delete [] pWAVBuffer;

	// Delete MP3 Buffer
	delete [] pMP3Buffer;

	// Close input file
	fclose( pFileIn );

	// Close output file
	fclose( pFileOut );

	if ( beWriteInfoTag )
	{
		// Write the INFO Tag
		beWriteInfoTag( hbeStream, strFileOut );
	}
	else
	{
		beWriteVBRHeader( strFileOut );
	}

	// Were done, return OK result
	return 0;
}

