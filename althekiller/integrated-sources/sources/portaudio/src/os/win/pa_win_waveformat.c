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

#include <windows.h>
#include <mmsystem.h>

#include "portaudio.h"
#include "pa_win_waveformat.h"


#if !defined(WAVE_FORMAT_EXTENSIBLE)
#define  WAVE_FORMAT_EXTENSIBLE         0xFFFE
#endif

#if !defined(WAVE_FORMAT_IEEE_FLOAT)
#define  WAVE_FORMAT_IEEE_FLOAT         0x0003
#endif

static GUID pawin_ksDataFormatSubtypePcm = 
	{ (USHORT)(WAVE_FORMAT_PCM), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };

static GUID pawin_ksDataFormatSubtypeIeeeFloat = 
	{ (USHORT)(WAVE_FORMAT_IEEE_FLOAT), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };



void PaWin_InitializeWaveFormatEx( PaWinWaveFormat *waveFormat, 
		int numChannels, PaSampleFormat sampleFormat, double sampleRate )
{
	WAVEFORMATEX *waveFormatEx = (WAVEFORMATEX*)waveFormat;
    int bytesPerSample = Pa_GetSampleSize(sampleFormat);
	unsigned long bytesPerFrame = numChannels * bytesPerSample;

    if( sampleFormat == paFloat32 )
        waveFormatEx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    else
        waveFormatEx->wFormatTag = WAVE_FORMAT_PCM;
	
	waveFormatEx->nChannels = (WORD)numChannels;
	waveFormatEx->nSamplesPerSec = (DWORD)sampleRate;
	waveFormatEx->nAvgBytesPerSec = waveFormatEx->nSamplesPerSec * bytesPerFrame;
	waveFormatEx->nBlockAlign = (WORD)bytesPerFrame;
	waveFormatEx->wBitsPerSample = bytesPerSample * 8;
	waveFormatEx->cbSize = 0;
}


void PaWin_InitializeWaveFormatExtensible( PaWinWaveFormat *waveFormat, 
		int numChannels, PaSampleFormat sampleFormat, double sampleRate,
		PaWinWaveFormatChannelMask channelMask )
{
	WAVEFORMATEX *waveFormatEx = (WAVEFORMATEX*)waveFormat;
    int bytesPerSample = Pa_GetSampleSize(sampleFormat);
	unsigned long bytesPerFrame = numChannels * bytesPerSample;

	waveFormatEx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	waveFormatEx->nChannels = (WORD)numChannels;
	waveFormatEx->nSamplesPerSec = (DWORD)sampleRate;
	waveFormatEx->nAvgBytesPerSec = waveFormatEx->nSamplesPerSec * bytesPerFrame;
	waveFormatEx->nBlockAlign = (WORD)bytesPerFrame;
	waveFormatEx->wBitsPerSample = bytesPerSample * 8;
	waveFormatEx->cbSize = 22;

	*((WORD*)&waveFormat->fields[PAWIN_INDEXOF_WVALIDBITSPERSAMPLE]) =
			waveFormatEx->wBitsPerSample;

	*((DWORD*)&waveFormat->fields[PAWIN_INDEXOF_DWCHANNELMASK]) = channelMask;
			
    if( sampleFormat == paFloat32 )
        *((GUID*)&waveFormat->fields[PAWIN_INDEXOF_SUBFORMAT]) =
            pawin_ksDataFormatSubtypeIeeeFloat;
    else
        *((GUID*)&waveFormat->fields[PAWIN_INDEXOF_SUBFORMAT]) =
            pawin_ksDataFormatSubtypePcm;
}


PaWinWaveFormatChannelMask PaWin_DefaultChannelMask( int numChannels )
{
	switch( numChannels ){
		case 1:
			return PAWIN_SPEAKER_MONO;
		case 2:
			return PAWIN_SPEAKER_STEREO; 
		case 3:
            return PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_FRONT_RIGHT;
		case 4:
			return PAWIN_SPEAKER_QUAD;
		case 5:
            return PAWIN_SPEAKER_QUAD | PAWIN_SPEAKER_FRONT_CENTER;
		case 6:
            /* The meaning of the PAWIN_SPEAKER_5POINT1 flag has changed over time:
                http://msdn2.microsoft.com/en-us/library/aa474707.aspx
               We use PAWIN_SPEAKER_5POINT1 (not PAWIN_SPEAKER_5POINT1_SURROUND)
               because on some cards (eg Audigy) PAWIN_SPEAKER_5POINT1_SURROUND 
               results in a virtual mixdown placing the rear output in the 
               front _and_ rear speakers.
            */
			return PAWIN_SPEAKER_5POINT1; 
        /* case 7: */
		case 8:
			return PAWIN_SPEAKER_7POINT1;
	}

    /* Apparently some Audigy drivers will output silence 
       if the direct-out constant (0) is used. So this is not ideal.    
    */
	return  PAWIN_SPEAKER_DIRECTOUT;

    /* Note that Alec Rogers proposed the following as an alternate method to 
        generate the default channel mask, however it doesn't seem to be an improvement
        over the above, since some drivers will matrix outputs mapping to non-present
        speakers accross multiple physical speakers.

        if(nChannels==1) {
            pwfFormat->dwChannelMask = SPEAKER_FRONT_CENTER;
        }
        else {
            pwfFormat->dwChannelMask = 0;
            for(i=0; i<nChannels; i++)
                pwfFormat->dwChannelMask = (pwfFormat->dwChannelMask << 1) | 0x1;
        }
    */
}
