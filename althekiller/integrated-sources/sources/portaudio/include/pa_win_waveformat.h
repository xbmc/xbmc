#ifndef PA_WIN_WAVEFORMAT_H
#define PA_WIN_WAVEFORMAT_H

/*
 * PortAudio Portable Real-Time Audio Library
 * Windows WAVEFORMAT* data structure utilities
 * portaudio.h should be included before this file.
 *
 * Copyright (c) 2007 Ross Bencina
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/** @file
 @brief Windows specific PortAudio API extension and utilities header file.
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
	The following #defines for speaker channel masks are the same
	as those in ksmedia.h, except with PAWIN_ prepended, KSAUDIO_ removed
	in some cases, and casts to PaWinWaveFormatChannelMask added.
*/

typedef unsigned long PaWinWaveFormatChannelMask;

/* Speaker Positions: */
#define PAWIN_SPEAKER_FRONT_LEFT				((PaWinWaveFormatChannelMask)0x1)
#define PAWIN_SPEAKER_FRONT_RIGHT				((PaWinWaveFormatChannelMask)0x2)
#define PAWIN_SPEAKER_FRONT_CENTER				((PaWinWaveFormatChannelMask)0x4)
#define PAWIN_SPEAKER_LOW_FREQUENCY				((PaWinWaveFormatChannelMask)0x8)
#define PAWIN_SPEAKER_BACK_LEFT					((PaWinWaveFormatChannelMask)0x10)
#define PAWIN_SPEAKER_BACK_RIGHT				((PaWinWaveFormatChannelMask)0x20)
#define PAWIN_SPEAKER_FRONT_LEFT_OF_CENTER		((PaWinWaveFormatChannelMask)0x40)
#define PAWIN_SPEAKER_FRONT_RIGHT_OF_CENTER		((PaWinWaveFormatChannelMask)0x80)
#define PAWIN_SPEAKER_BACK_CENTER				((PaWinWaveFormatChannelMask)0x100)
#define PAWIN_SPEAKER_SIDE_LEFT					((PaWinWaveFormatChannelMask)0x200)
#define PAWIN_SPEAKER_SIDE_RIGHT				((PaWinWaveFormatChannelMask)0x400)
#define PAWIN_SPEAKER_TOP_CENTER				((PaWinWaveFormatChannelMask)0x800)
#define PAWIN_SPEAKER_TOP_FRONT_LEFT			((PaWinWaveFormatChannelMask)0x1000)
#define PAWIN_SPEAKER_TOP_FRONT_CENTER			((PaWinWaveFormatChannelMask)0x2000)
#define PAWIN_SPEAKER_TOP_FRONT_RIGHT			((PaWinWaveFormatChannelMask)0x4000)
#define PAWIN_SPEAKER_TOP_BACK_LEFT				((PaWinWaveFormatChannelMask)0x8000)
#define PAWIN_SPEAKER_TOP_BACK_CENTER			((PaWinWaveFormatChannelMask)0x10000)
#define PAWIN_SPEAKER_TOP_BACK_RIGHT			((PaWinWaveFormatChannelMask)0x20000)

/* Bit mask locations reserved for future use */
#define PAWIN_SPEAKER_RESERVED					((PaWinWaveFormatChannelMask)0x7FFC0000)

/* Used to specify that any possible permutation of speaker configurations */
#define PAWIN_SPEAKER_ALL						((PaWinWaveFormatChannelMask)0x80000000)

/* DirectSound Speaker Config */
#define PAWIN_SPEAKER_DIRECTOUT					0
#define PAWIN_SPEAKER_MONO						(PAWIN_SPEAKER_FRONT_CENTER)
#define PAWIN_SPEAKER_STEREO					(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT)
#define PAWIN_SPEAKER_QUAD						(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | \
												PAWIN_SPEAKER_BACK_LEFT  | PAWIN_SPEAKER_BACK_RIGHT)
#define PAWIN_SPEAKER_SURROUND					(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | \
												PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_BACK_CENTER)
#define PAWIN_SPEAKER_5POINT1					(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | \
												PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
												PAWIN_SPEAKER_BACK_LEFT  | PAWIN_SPEAKER_BACK_RIGHT)
#define PAWIN_SPEAKER_7POINT1					(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | \
												PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
												PAWIN_SPEAKER_BACK_LEFT | PAWIN_SPEAKER_BACK_RIGHT | \
												PAWIN_SPEAKER_FRONT_LEFT_OF_CENTER | PAWIN_SPEAKER_FRONT_RIGHT_OF_CENTER)
#define PAWIN_SPEAKER_5POINT1_SURROUND			(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | \
												PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
												PAWIN_SPEAKER_SIDE_LEFT  | PAWIN_SPEAKER_SIDE_RIGHT)
#define PAWIN_SPEAKER_7POINT1_SURROUND			(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | \
												PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
												PAWIN_SPEAKER_BACK_LEFT | PAWIN_SPEAKER_BACK_RIGHT | \
												PAWIN_SPEAKER_SIDE_LEFT | PAWIN_SPEAKER_SIDE_RIGHT)
/*
 According to the Microsoft documentation:
 The following are obsolete 5.1 and 7.1 settings (they lack side speakers).  Note this means
 that the default 5.1 and 7.1 settings (KSAUDIO_SPEAKER_5POINT1 and KSAUDIO_SPEAKER_7POINT1 are
 similarly obsolete but are unchanged for compatibility reasons).
*/
#define PAWIN_SPEAKER_5POINT1_BACK				PAWIN_SPEAKER_5POINT1
#define PAWIN_SPEAKER_7POINT1_WIDE				PAWIN_SPEAKER_7POINT1

/* DVD Speaker Positions */
#define PAWIN_SPEAKER_GROUND_FRONT_LEFT			PAWIN_SPEAKER_FRONT_LEFT
#define PAWIN_SPEAKER_GROUND_FRONT_CENTER		PAWIN_SPEAKER_FRONT_CENTER
#define PAWIN_SPEAKER_GROUND_FRONT_RIGHT		PAWIN_SPEAKER_FRONT_RIGHT
#define PAWIN_SPEAKER_GROUND_REAR_LEFT			PAWIN_SPEAKER_BACK_LEFT
#define PAWIN_SPEAKER_GROUND_REAR_RIGHT			PAWIN_SPEAKER_BACK_RIGHT
#define PAWIN_SPEAKER_TOP_MIDDLE				PAWIN_SPEAKER_TOP_CENTER
#define PAWIN_SPEAKER_SUPER_WOOFER				PAWIN_SPEAKER_LOW_FREQUENCY


/*
	PaWinWaveFormat is defined here to provide compatibility with
	compilation environments which don't have headers defining 
	WAVEFORMATEXTENSIBLE (e.g. older versions of MSVC, Borland C++ etc.

	The fields for WAVEFORMATEX and WAVEFORMATEXTENSIBLE are declared as an
    unsigned char array here to avoid clients who include this file having 
    a dependency on windows.h and mmsystem.h, and also to to avoid having
    to write separate packing pragmas for each compiler.
*/
#define PAWIN_SIZEOF_WAVEFORMATEX   18
#define PAWIN_SIZEOF_WAVEFORMATEXTENSIBLE (PAWIN_SIZEOF_WAVEFORMATEX + 22)

typedef struct{
    unsigned char fields[ PAWIN_SIZEOF_WAVEFORMATEXTENSIBLE ];
    unsigned long extraLongForAlignment; /* ensure that compiler aligns struct to DWORD */ 
} PaWinWaveFormat;

/*
    WAVEFORMATEXTENSIBLE fields:
    
    union  {
	    WORD  wValidBitsPerSample;    
	    WORD  wSamplesPerBlock;    
	    WORD  wReserved;  
    } Samples;
    DWORD  dwChannelMask;  
    GUID  SubFormat;
*/

#define PAWIN_INDEXOF_WVALIDBITSPERSAMPLE	(PAWIN_SIZEOF_WAVEFORMATEX+0)
#define PAWIN_INDEXOF_DWCHANNELMASK			(PAWIN_SIZEOF_WAVEFORMATEX+2)
#define PAWIN_INDEXOF_SUBFORMAT				(PAWIN_SIZEOF_WAVEFORMATEX+6)

/*
	Use the following two functions to initialize the waveformat structure.
*/

void PaWin_InitializeWaveFormatEx( PaWinWaveFormat *waveFormat, 
		int numChannels, PaSampleFormat sampleFormat, double sampleRate );


void PaWin_InitializeWaveFormatExtensible( PaWinWaveFormat *waveFormat, 
		int numChannels, PaSampleFormat sampleFormat, double sampleRate,
	    PaWinWaveFormatChannelMask channelMask );


/* Map a channel count to a speaker channel mask */
PaWinWaveFormatChannelMask PaWin_DefaultChannelMask( int numChannels );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_WIN_WAVEFORMAT_H */