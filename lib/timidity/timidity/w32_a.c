/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    w32_a.c

    Functions to play sound on the Windows audio driver (Windows 95/98/NT).

    Modified by Masanao Izumo <mo@goice.co.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifdef __W32__
#include "interface.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <windows.h>

extern CRITICAL_SECTION critSect;

/*****************************************************************************************************************************/

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#ifdef HAVE_NEW_MMSYSTEM
#include <mmsystem.h>
#else
/* On cygnus, there is not mmsystem.h for Multimedia API's.
 * mmsystem.h can not distribute becase of Microsoft Lisence
 * Then declare some of them here. **/
#define WOM_OPEN                0x3BB
#define WOM_CLOSE               0x3BC
#define WOM_DONE                0x3BD
#define WAVE_FORMAT_QUERY       0x0001
#define WAVE_ALLOWSYNC          0x0002
#define WAVE_FORMAT_PCM         1
#define CALLBACK_FUNCTION       0x00030000l
#define WAVERR_BASE             32
#define WAVE_MAPPER             (UINT)-1

DECLARE_HANDLE(HWAVEOUT);
DECLARE_HANDLE(HWAVE);
typedef HWAVEOUT *LPHWAVEOUT;

/* Define WAVEHDR, WAVEFORMAT structure */

typedef struct wavehdr_tag
{
    LPSTR       lpData;
    DWORD       dwBufferLength;
    DWORD       dwBytesRecorded;
    DWORD       dwUser;
    DWORD       dwFlags;
    DWORD       dwLoops;
    struct wavehdr_tag *lpNext;
    DWORD       reserved;
} WAVEHDR;

typedef struct
{
    WORD    wFormatTag;
    WORD    nChannels;
    DWORD   nSamplesPerSec;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;
    WORD    wBitsPerSample;
    WORD    cbSize;
} WAVEFORMAT, WAVEFORMATEX, *LPWAVEFORMATEX;


typedef struct waveoutcaps_tag
{
    WORD    wMid;
    WORD    wPid;
    UINT    vDriverVersion;
#define MAXPNAMELEN      32
    char    szPname[MAXPNAMELEN];
    DWORD   dwFormats;
    WORD    wChannels;
    DWORD   dwSupport;
} WAVEOUTCAPS;

typedef WAVEHDR *       LPWAVEHDR;
typedef WAVEFORMAT *    LPWAVEFORMAT;
typedef WAVEOUTCAPS *   LPWAVEOUTCAPS;
typedef UINT            MMRESULT;

MMRESULT WINAPI waveOutOpen(LPHWAVEOUT, UINT, LPWAVEFORMAT, DWORD, DWORD, DWORD);
MMRESULT WINAPI waveOutClose(HWAVEOUT);
MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT, LPWAVEHDR, UINT);
MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT, LPWAVEHDR, UINT);
MMRESULT WINAPI waveOutWrite(HWAVEOUT, LPWAVEHDR, UINT);
UINT     WINAPI waveOutGetNumDevs(void);
MMRESULT WINAPI waveOutReset(HWAVEOUT);
MMRESULT WINAPI waveOutGetDevCaps(UINT, LPWAVEOUTCAPS, UINT);
MMRESULT WINAPI waveOutGetDevCapsA(UINT, LPWAVEOUTCAPS, UINT);
#define waveOutGetDevCaps waveOutGetDevCapsA
MMRESULT WINAPI waveOutGetID(HWAVEOUT, UINT*);

#endif
#endif /* __CYGWIN32__ */

/*****************************************************************************************************************************/

#include "timidity.h"
#include "output.h"
#include "controls.h"
#include "timer.h"
#include "instrum.h"
#include "playmidi.h"
#include "mblock.h"
#include "miditrace.h"
#include "interface.h"

#define NOT !

static int  open_output     (void); /* 0=success, 1=warning, -1=fatal error */
static void close_output    (void);
static int  output_data     (char * Data, int32 Size);
static int  acntl           (int request, void * arg);

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
//#if defined ( IA_W32GUI )
volatile int data_block_bits = DEFAULT_AUDIO_BUFFER_BITS;
volatile int data_block_num = 64;
#endif

#define DATA_BLOCK_SIZE     (4 * AUDIO_BUFFER_SIZE)
#define DATA_BLOCK_NUM      (dpm.extra_param[0])

struct MMBuffer
{
    int                 Number;
    int                 Prepared;   // Non-zero if this buffer has been prepared.

    HGLOBAL             hData;
    void *              Data;

    HGLOBAL             hHead;
    WAVEHDR *           Head;

    struct MMBuffer *   Next;
};

static struct MMBuffer *            Buffers;

static volatile struct MMBuffer *   FreeBuffers;
static volatile int                 NumBuffersInUse;

static HWAVEOUT                     hDevice;
static int                          BufferDelay;                    // in milliseconds

static const int                    AllowSynchronousWaveforms = 1;

/*****************************************************************************************************************************/

static void CALLBACK                OnPlaybackEvent (HWAVE hWave, UINT Msg, DWORD UserData, DWORD Param1, DWORD Param2);
static void                         BufferPoolReset (void);
static struct MMBuffer *            GetBuffer       ();
static void                         PutBuffer       (struct MMBuffer *);
static const char *                 MMErrorMessage  (MMRESULT Result);
static void                         WaitForBuffer   (int WaitForAllBuffers);

/*****************************************************************************************************************************/

static int detect(void);

#define dpm w32_play_mode

PlayMode dpm =
{
    DEFAULT_RATE,
    PE_16BIT | PE_SIGNED,
    PF_PCM_STREAM|PF_CAN_TRACE|PF_BUFF_FRAGM_OPT,
    -1,
    {32},
    "Windows audio driver", 'd',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl,
	detect
};

/*****************************************************************************************************************************/

static int open_output(void)
{
    int             i;
    int             j;
    int             IsMono;
    WAVEFORMATEX    wf;
    WAVEOUTCAPS     woc;
    MMRESULT        Result;
    UINT            DeviceID;

    if (dpm.extra_param[0] < 8)
    {
        ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Too small -B option: %d", dpm.extra_param[0]);
        dpm.extra_param[0] = 8;
    }

/** Check if there is at least one audio device. **/

    if (waveOutGetNumDevs() == 0)
    {
        ctl->cmsg (CMSG_ERROR, VERB_NORMAL, "No audio devices present!");
        return -1;
    }

/** They can't mean these. **/

    dpm.encoding &= ~(PE_ULAW | PE_ALAW | PE_BYTESWAP);

    if (dpm.encoding & PE_16BIT || dpm.encoding & PE_24BIT)
        dpm.encoding |= PE_SIGNED;
    else
        dpm.encoding &= ~PE_SIGNED;

    IsMono  = (dpm.encoding & PE_MONO);

    memset(&wf, 0, sizeof(wf));

    wf.wFormatTag     = WAVE_FORMAT_PCM;
    wf.nChannels      = IsMono ? 1 : 2;
    wf.nSamplesPerSec = dpm.rate;

    i = dpm.rate;
    j = 1;

    if (!IsMono)
    {
        i *= 2;
        j *= 2;
    }

	if (dpm.encoding & PE_24BIT) {
		i *= 3;
		j *= 3;
	} else if (dpm.encoding & PE_16BIT) {
        i *= 2;
        j *= 2;
    }

    wf.nAvgBytesPerSec = i;
    wf.nBlockAlign     = j;
	if (dpm.encoding & PE_24BIT) {
		wf.wBitsPerSample  = 24;
	} else if (dpm.encoding & PE_16BIT) {
		wf.wBitsPerSample  = 16;
	} else {
		wf.wBitsPerSample  = 8;
	}
    wf.cbSize          = sizeof(WAVEFORMAT);

/** Open the device. **/

    { CHAR  b[256]; wsprintf(b, "Opening device...\n"); OutputDebugString(b); }

    hDevice = 0;

    if (AllowSynchronousWaveforms)
        Result = waveOutOpen(&hDevice, WAVE_MAPPER, (LPWAVEFORMATEX) &wf, (DWORD) OnPlaybackEvent, 0, CALLBACK_FUNCTION | WAVE_ALLOWSYNC);
    else
        Result = waveOutOpen(&hDevice, WAVE_MAPPER, (LPWAVEFORMATEX) &wf, (DWORD) OnPlaybackEvent, 0, CALLBACK_FUNCTION);

    if (Result)
    {
        ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Can't open audio device: " "encoding=<%s>, rate=<%d>, ch=<%d>: %s", output_encoding_string(dpm.encoding), dpm.rate, wf.nChannels, MMErrorMessage(Result));
        return -1;
    }
    else
        { CHAR  b[256]; wsprintf(b, "Device opened.\n"); OutputDebugString(b); }

/** Get the device ID. **/

    DeviceID = 0;
    waveOutGetID(hDevice, &DeviceID);

/** Get the device capabilities. **/

    memset(&woc, 0, sizeof(WAVEOUTCAPS));
    Result = waveOutGetDevCaps(DeviceID, &woc, sizeof(WAVEOUTCAPS));

    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Device ID: %d",              DeviceID);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Manufacture ID: %d",         woc.wMid);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Product ID: %d",             woc.wPid);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Driver version: %d",         woc.vDriverVersion);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Product name: %s",           woc.szPname);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Formats supported: 0x%08X",  woc.dwFormats);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Max. channels: %d",          woc.wChannels);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Supported features: 0x%08X", woc.dwSupport);

/** Calculate the buffer delay. **/

    BufferDelay = AUDIO_BUFFER_SIZE;

    if (NOT (dpm.encoding & PE_MONO))
        BufferDelay *= 2;

	if (dpm.encoding & PE_24BIT)
		BufferDelay *= 3;
    else if (dpm.encoding & PE_16BIT)
        BufferDelay *= 2;

    BufferDelay = (BufferDelay * 1000) / dpm.rate;

/** Create the buffer pool. **/

    Buffers = (struct MMBuffer *) safe_malloc(DATA_BLOCK_NUM * sizeof(struct MMBuffer));

    for (i = 0; i < DATA_BLOCK_NUM; i++)
    {
        struct MMBuffer *   b;

        b = &Buffers[i];

        b->hData = GlobalAlloc(GMEM_ZEROINIT, DATA_BLOCK_SIZE);
        b->Data  = GlobalLock (b->hData);
        b->hHead = GlobalAlloc(GMEM_ZEROINIT, sizeof(WAVEHDR));
        b->Head  = GlobalLock (b->hHead);
    }

    BufferPoolReset();

/** Set the file descriptor. **/

    dpm.fd = 0;

    return 0;
}

/*****************************************************************************************************************************/

static void close_output(void)
{
    int i;

    if (dpm.fd != -1)
    {
        WaitForBuffer(1);

        { CHAR  b[256]; wsprintf(b, "Closing device...\n"); OutputDebugString(b); }

        waveOutReset(hDevice);
        waveOutClose(hDevice);

        { CHAR  b[256]; wsprintf(b, "Device closed.\n"); OutputDebugString(b); }

    /** Free all buffers. **/

        for (i = 0; i < DATA_BLOCK_NUM; i++)
        {
            struct MMBuffer *   block;

            block = &Buffers[i];

            GlobalUnlock(block->hHead);
            GlobalFree  (block->hHead);

            GlobalUnlock(block->hData);
            GlobalFree  (block->hData);
        }

        free(Buffers);

    /** Reset the file descriptor. **/

        dpm.fd = -1;
    }
}

static int detect(void)
{
	if (waveOutGetNumDevs() == 0) {return 0;}	/* not found */
	return 1;	/* found */
}


/*****************************************************************************************************************************/

static int output_data(char * Data, int32 Size)
{
    char *  d;
    int32   s;

    d = Data;
    s = Size;

    while (s > 0)
    {
        int32               n;
        struct MMBuffer *   b;

        MMRESULT            Result;
        LPWAVEHDR           wh;

        if ((b = GetBuffer()) == NULL)
        {
            WaitForBuffer(0);
            continue;
        }

        if (s <= DATA_BLOCK_SIZE)
            n = s;
        else
            n = DATA_BLOCK_SIZE;

        CopyMemory(b->Data, d, n);

        wh = b->Head;

        wh->dwBufferLength = n;
        wh->lpData         = b->Data;
        wh->dwUser         = b->Number;

    /** Prepare the buffer. **/

        { CHAR  b[256]; wsprintf(b, "%2d: Preparing buffer %d...\n", NumBuffersInUse, wh->dwUser); OutputDebugString(b); }

        Result = waveOutPrepareHeader(hDevice, wh, sizeof(WAVEHDR));

        if (Result)
        {
            { CHAR  b[256]; wsprintf(b, "%2d: Buffer preparation failed.\n", NumBuffersInUse); OutputDebugString(b); }

            ctl->cmsg (CMSG_ERROR, VERB_NORMAL, "waveOutPrepareHeader(): %s", MMErrorMessage(Result));
            return -1;
        }
        else
            { CHAR  b[256]; wsprintf(b, "%2d: Buffer %d prepared.\n", NumBuffersInUse, wh->dwUser); OutputDebugString(b); }

        b->Prepared = 1;

    /** Queue the buffer. **/

        { CHAR  b[256]; wsprintf(b, "%2d: Queueing buffer %d...\n", NumBuffersInUse, wh->dwUser); OutputDebugString(b); }

        Result = waveOutWrite(hDevice, wh, sizeof(WAVEHDR));

        if (Result)
        {
            { CHAR  b[256]; wsprintf(b, "%2d: Buffer queueing failed.\n", NumBuffersInUse); OutputDebugString(b); }

            ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "waveOutWrite(): %s", MMErrorMessage(Result));
            return -1;
        }
        else
            { CHAR  b[256]; wsprintf(b, "%2d: Buffer %d queued.\n", NumBuffersInUse, wh->dwUser); OutputDebugString(b); }

        d += n;
        s -= n;
    }

    return 0;
}

/*****************************************************************************************************************************/

static int acntl(int request, void *arg)
{
  static char dummy_sounds[4*AUDIO_BUFFER_SIZE];

    switch(request)
    {
        case PM_REQ_GETQSIZ:
            *(int *)arg = (DATA_BLOCK_NUM-1) * AUDIO_BUFFER_SIZE;

            if (NOT (dpm.encoding & PE_MONO))
                *(int *)arg *= 2;

			if (dpm.encoding & PE_24BIT)
                *(int *)arg *= 3;
            else if (dpm.encoding & PE_16BIT)
                *(int *)arg *= 2;
            return 0;

        case PM_REQ_DISCARD:
        {
            { CHAR  b[256]; wsprintf(b, "Resetting audio device.\n"); OutputDebugString(b); }

            waveOutReset(hDevice);
	    close_output();
	    open_output();

            { CHAR  b[256]; wsprintf(b, "Audio device reset.\n"); OutputDebugString(b); }

            return 0;
        }

        case PM_REQ_FLUSH:
        {
	    close_output();
	    open_output();
            return 0;
        }
    }

    return -1;
}

/*****************************************************************************************************************************/

static void CALLBACK OnPlaybackEvent(HWAVE hWave, UINT Msg, DWORD UserData, DWORD Param1, DWORD Param2)
{
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Msg: 0x%08X, Num. buffers in use: %d", Msg, NumBuffersInUse);

    switch (Msg)
    {
        case WOM_OPEN:
            { CHAR  b[256]; wsprintf(b, "%2d: Device opened.\n", NumBuffersInUse); OutputDebugString(b); }
            break;

        case WOM_CLOSE:
            { CHAR  b[256]; wsprintf(b, "%2d: Device closed.\n", NumBuffersInUse); OutputDebugString(b); }
            break;

        case WOM_DONE:
        {
            WAVEHDR *   wh;

            EnterCriticalSection(&critSect);

            wh = (WAVEHDR *) Param1;

/* It's not safe to do this here. Read the remarks of waveOutProc() in the SDK on which functions are safe to call.
            if (NOT Queueing)
            {
                { CHAR  b[256]; wsprintf(b, "%2d: Dequeueing buffer %d...\n", NumBuffersInUse, wh->dwUser); OutputDebugString(b); }

                waveOutUnprepareHeader(hDevice, wh, sizeof(WAVEHDR));

                { CHAR  b[256]; wsprintf(b, "%2d: Buffer %d dequeued.\n",     NumBuffersInUse, wh->dwUser); OutputDebugString(b); }
            }
            else
                { CHAR  b[256]; wsprintf(b, "%2d: *** Buffer %d not dequeued! ***\n", NumBuffersInUse, wh->dwUser); OutputDebugString(b); }
 */
            PutBuffer(&Buffers[wh->dwUser]);

            LeaveCriticalSection(&critSect);

            break;
        }

        default:
        {
            CHAR    b[256];

            wsprintf(b, "%2d: Unknown play back event 0x%08X.\n", NumBuffersInUse, Msg);
            OutputDebugString(b);
        }
    }
}

/*****************************************************************************************************************************/

#define DIM(a) sizeof(a) / sizeof(a[0])

static const char * mmsyserr_code_string[] =
{
    "no error",
    "unspecified error",
    "device ID out of range",
    "driver failed enable",
    "device already allocated",
    "device handle is invalid",
    "no device driver present",
    "memory allocation error",
    "function isn't supported",
    "error value out of range",
    "invalid flag passed",
    "invalid parameter passed",
    "handle being used",
};

static const char * waverr_code_sring[] =
{
    "unsupported wave format",
    "still something playing",
    "header not prepared",
    "device is synchronous",
};

static const char * MMErrorMessage(MMRESULT ErrorCode)
{
    static char s[32];

    if (ErrorCode >= WAVERR_BASE)
    {
        ErrorCode -= WAVERR_BASE;

        if (ErrorCode > DIM(waverr_code_sring))
        {
            wsprintf(s, "Unknown wave error %d", ErrorCode);
            return s;
        }
        else
            return waverr_code_sring[ErrorCode];
    }
    else
    if (ErrorCode > DIM(mmsyserr_code_string))
    {
        wsprintf(s, "Unknown multimedia error %d", ErrorCode);
        return s;
    }
    else
        return mmsyserr_code_string[ErrorCode];
}

#undef DIM

/*****************************************************************************************************************************/

static void BufferPoolReset(void)
{
    int i;

    { CHAR  b[256]; wsprintf(b, "Resetting buffer pool...\n"); OutputDebugString(b); }

    Buffers[0].Number   = 0;
    Buffers[0].Prepared = 0;
    Buffers[0].Next     = &Buffers[1];

    for (i = 1; i < DATA_BLOCK_NUM - 1; i++)
    {
        Buffers[i].Number   = i;
        Buffers[i].Prepared = 0;
        Buffers[i].Next     = &Buffers[i + 1];
    }

    Buffers[i].Number   = i;
    Buffers[i].Prepared = 0;
    Buffers[i].Next     = NULL;

    FreeBuffers     = &Buffers[0];
    NumBuffersInUse = 0;

    { CHAR  b[256]; wsprintf(b, "Buffer pool reset.\n", NumBuffersInUse); OutputDebugString(b); }
}

/*****************************************************************************************************************************/

static struct MMBuffer * GetBuffer()
{
    struct MMBuffer *   b;

    { CHAR  b[256]; wsprintf(b, "%2d: Getting buffer...\n", NumBuffersInUse); OutputDebugString(b); }

    EnterCriticalSection(&critSect);

    if (FreeBuffers)
    {
        b           = FreeBuffers;
        FreeBuffers = FreeBuffers->Next;
        NumBuffersInUse++;

    /** If this buffer is still prepared we can safely unprepare it because we got it from the free buffer list. **/

        if (b->Prepared)
        {
            waveOutUnprepareHeader(hDevice, (LPWAVEHDR) b->Head, sizeof(WAVEHDR));

            b->Prepared = 0;
        }

        b->Next     = NULL;
    }
    else
        b = NULL;

    LeaveCriticalSection(&critSect);

    { CHAR  b[256]; wsprintf(b, "%2d: Got buffer.\n", NumBuffersInUse); OutputDebugString(b); }

    return b;
}

/*****************************************************************************************************************************/

static void PutBuffer(struct MMBuffer * b)
{
    { CHAR  b[256]; wsprintf(b, "%2d: Putting buffer...\n", NumBuffersInUse); OutputDebugString(b); }

    b->Next     = FreeBuffers;
    FreeBuffers = b;
    NumBuffersInUse--;

    { CHAR  b[256]; wsprintf(b, "%2d: Buffer put.\n", NumBuffersInUse); OutputDebugString(b); }
}

/*****************************************************************************************************************************/

static void WaitForBuffer(int WaitForAllBuffers)
{
  int numbuf;

    if (WaitForAllBuffers)
    {
        { CHAR  b[256]; wsprintf(b, "%2d: Waiting for all buffers to be dequeued...\n", NumBuffersInUse); OutputDebugString(b); }

	while (1) {
	  EnterCriticalSection(&critSect);
	  numbuf = NumBuffersInUse;
	  if (numbuf) {
            LeaveCriticalSection(&critSect);
	    Sleep(BufferDelay);
	    continue;
	  }
	  break;
	}
	LeaveCriticalSection(&critSect);


//        while (NumBuffersInUse)
//            Sleep(BufferDelay);

        { CHAR  b[256]; wsprintf(b, "%2d: All buffers dequeued.\n", NumBuffersInUse); OutputDebugString(b); }

        BufferPoolReset();
    }
    else
    {
        { CHAR  b[256]; wsprintf(b, "%2d: Waiting %dms...\n", NumBuffersInUse, BufferDelay); OutputDebugString(b); }

		#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
//		#if !defined ( IA_W32GUI )
        if(ctl->trace_playing)
            Sleep(0);
        else
    #endif
            Sleep(BufferDelay);

        { CHAR  b[256]; wsprintf(b, "%2d: Wait finished.\n", NumBuffersInUse); OutputDebugString(b); }
    }
}
