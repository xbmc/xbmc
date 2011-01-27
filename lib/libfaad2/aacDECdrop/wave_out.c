/*
 * function: Support for playing wave files - Win32 - ONLY
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 */

#include <string.h>
#include <errno.h>
#include "wave_out.h"

#define MAX_WAVEBLOCKS    32


static CRITICAL_SECTION  cs;
static HWAVEOUT          dev                    = NULL;
static int               ScheduledBlocks        = 0;
static int               PlayedWaveHeadersCount = 0;          // free index
static WAVEHDR*          PlayedWaveHeaders [MAX_WAVEBLOCKS];


static int
Box ( const char* msg )
{
	MessageBox ( NULL, msg, "Error Message . . .", MB_OK | MB_ICONEXCLAMATION );
	return -1;
}


/*
 *  This function registers already played WAVE chunks. Freeing is done by free_memory(),
 */

static void CALLBACK
wave_callback ( HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
	if ( uMsg == WOM_DONE )
	{
		EnterCriticalSection ( &cs );
		PlayedWaveHeaders [PlayedWaveHeadersCount++] = (WAVEHDR*) dwParam1;
		LeaveCriticalSection ( &cs );
	}
}


static void
free_memory ( void )
{
	WAVEHDR*  wh;
	HGLOBAL   hg;

	EnterCriticalSection ( &cs );
	wh = PlayedWaveHeaders [--PlayedWaveHeadersCount];
	ScheduledBlocks--;                        // decrease the number of USED blocks
	LeaveCriticalSection ( &cs );

	waveOutUnprepareHeader ( dev, wh, sizeof (WAVEHDR) );

	hg = GlobalHandle ( wh -> lpData );       // Deallocate the buffer memory
	GlobalUnlock (hg);
	GlobalFree   (hg);

	hg = GlobalHandle ( wh );                 // Deallocate the header memory
	GlobalUnlock (hg);
	GlobalFree   (hg);
}


Int
Set_WIN_Params ( FILE_T   dummyFile ,
                 Ldouble  SampleFreq,
                 Uint     BitsPerSample,
                 Uint     Channels )
{
	WAVEFORMATEX  outFormat;
	UINT          deviceID = WAVE_MAPPER;

	(void) dummyFile;

	if ( waveOutGetNumDevs () == 0 )
		return Box ( "No audio device present." );

	outFormat.wFormatTag      = WAVE_FORMAT_PCM;
	outFormat.wBitsPerSample  = BitsPerSample;
	outFormat.nChannels       = Channels;
	outFormat.nSamplesPerSec  = (unsigned long)(SampleFreq);
	outFormat.nBlockAlign     = outFormat.nChannels * outFormat.wBitsPerSample/8;
	outFormat.nAvgBytesPerSec = outFormat.nSamplesPerSec * outFormat.nChannels * outFormat.wBitsPerSample/8;

	switch ( waveOutOpen ( &dev, deviceID, &outFormat, (DWORD)wave_callback, 0, CALLBACK_FUNCTION ) )
	{
		case MMSYSERR_ALLOCATED:   return Box ( "Device is already open." );
		case MMSYSERR_BADDEVICEID: return Box ( "The specified device is out of range." );
		case MMSYSERR_NODRIVER:    return Box ( "There is no audio driver in this system." );
		case MMSYSERR_NOMEM:       return Box ( "Unable to allocate sound memory." );
		case WAVERR_BADFORMAT:     return Box ( "This audio format is not supported." );
		case WAVERR_SYNC:          return Box ( "The device is synchronous." );
		default:                   return Box ( "Unknown media error." );
		case MMSYSERR_NOERROR:     break;
	}

	waveOutReset ( dev );
	InitializeCriticalSection ( &cs );
	SetPriorityClass ( GetCurrentProcess (), HIGH_PRIORITY_CLASS );
//	SetPriorityClass ( GetCurrentProcess (), REALTIME_PRIORITY_CLASS );
	return 0;
}


int
WIN_Play_Samples ( const void* data, size_t len )
{
	HGLOBAL    hg;
	HGLOBAL    hg2;
	LPWAVEHDR  wh;
	void*      allocptr;

	do
	{
		while ( PlayedWaveHeadersCount > 0 )                        // free used blocks ...
			free_memory ();

		if ( ScheduledBlocks < sizeof(PlayedWaveHeaders)/sizeof(*PlayedWaveHeaders) ) // wait for a free block ...
			break;
		Sleep (26);

	} while (1);

	if ( (hg2 = GlobalAlloc ( GMEM_MOVEABLE, len )) == NULL )   // allocate some memory for a copy of the buffer
		return Box ( "GlobalAlloc failed." );

	allocptr = GlobalLock (hg2);
	CopyMemory ( allocptr, data, len );                         // Here we can call any modification output functions we want....

	if ( (hg = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (WAVEHDR))) == NULL ) // now make a header and WRITE IT!
		return -1;

	wh                   = GlobalLock (hg);
	wh->dwBufferLength   = len;
	wh->lpData           = allocptr;

	if ( waveOutPrepareHeader ( dev, wh, sizeof (WAVEHDR)) != MMSYSERR_NOERROR )
	{
		GlobalUnlock (hg);
		GlobalFree   (hg);
		return -1;
	}

	if ( waveOutWrite ( dev, wh, sizeof (WAVEHDR)) != MMSYSERR_NOERROR )
	{
		GlobalUnlock (hg);
		GlobalFree   (hg);
		return -1;
	}

	EnterCriticalSection ( &cs );
	ScheduledBlocks++;
	LeaveCriticalSection ( &cs );

	return len;
}


int
WIN_Audio_close ( void )
{
	if ( dev != NULL )
	{
		while ( ScheduledBlocks > 0 )
		{
			Sleep (ScheduledBlocks);
			while ( PlayedWaveHeadersCount > 0 )                        // free used blocks ...
			free_memory ();
		}

		waveOutReset (dev);      // reset the device
		waveOutClose (dev);      // close the device
		dev = NULL;
	}

	DeleteCriticalSection ( &cs );
	ScheduledBlocks = 0;
	return 0;
}


/******************************** end of wave_out.c ********************************/

