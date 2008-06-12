/** @file patest_multi_sine.c
	@ingroup test_src
	@brief Play a different sine wave on each channel.
	@author Phil Burk  http://www.softsynth.com
*/
/*
 * $Id: patest_multi_sine.c 1097 2006-08-26 08:27:53Z rossb $
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
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
 
#include <stdio.h>
#include <math.h>

#include "portaudio.h"

#define SAMPLE_RATE       (44100)
#define FRAMES_PER_BUFFER (256)
#define FREQ_INCR         (300.0 / SAMPLE_RATE)
#define MAX_CHANNELS      (64)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

typedef struct
{
    short   interleaved;          /* Nonzero for interleaved / zero for non-interleaved. */
    int     numChannels;          /* Actually used. */
    double  phases[MAX_CHANNELS]; /* Each channel gets its' own frequency. */
}
paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void*                     inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags           statusFlags,
                          void*                           userData)
{
    int         frameIndex, channelIndex;
    float**     outputs = (float**)outputBuffer;
    paTestData* data    = (paTestData*)userData;

    (void) inputBuffer;     /* Prevent unused arg warning. */
    if (data->interleaved)
        {
        float *out = (float*)outputBuffer;      /* interleaved version */
        for( frameIndex=0; frameIndex<(int)framesPerBuffer; frameIndex++ )
            {
            for( channelIndex=0; channelIndex<data->numChannels; channelIndex++ )
                {
                /* Output sine wave on every channel. */
                *out++ = (float) sin(data->phases[channelIndex]);

                /* Play each channel at a higher frequency. */
                data->phases[channelIndex] += FREQ_INCR * (4 + channelIndex);
                if( data->phases[channelIndex] >= (2.0 * M_PI) ) data->phases[channelIndex] -= (2.0 * M_PI);
                }
            }
        }
    else
        {
        for( frameIndex=0; frameIndex<(int)framesPerBuffer; frameIndex++ )
            {
            for( channelIndex=0; channelIndex<data->numChannels; channelIndex++ )
                {
                /* Output sine wave on every channel. */
                outputs[channelIndex][frameIndex] = (float) sin(data->phases[channelIndex]);

                /* Play each channel at a higher frequency. */
                data->phases[channelIndex] += FREQ_INCR * (4 + channelIndex);
                if( data->phases[channelIndex] >= (2.0 * M_PI) ) data->phases[channelIndex] -= (2.0 * M_PI);
                }
            }
        }
    return 0;
}

/*******************************************************************/
int test(short interleaved)
{
    PaStream*           stream;
    PaStreamParameters  outputParameters;
    PaError             err;
    const PaDeviceInfo* pdi;
    paTestData          data;
    short               n;

    outputParameters.device = Pa_GetDefaultOutputDevice();  /* Default output device, max channels. */
    pdi = Pa_GetDeviceInfo(outputParameters.device);
    outputParameters.channelCount = pdi->maxOutputChannels;
    if (outputParameters.channelCount > MAX_CHANNELS)
        outputParameters.channelCount = MAX_CHANNELS;
    outputParameters.sampleFormat = paFloat32;              /* 32 bit floating point output */
    outputParameters.suggestedLatency = pdi->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
    data.interleaved = interleaved;
    data.numChannels = outputParameters.channelCount;
    for (n = 0; n < data.numChannels; n++)
        data.phases[n] = 0.0; /* Phases wrap and maybe don't need initialisation. */
    printf("%d ", data.numChannels);
    if (interleaved)
        printf("interleaved ");
    else
        {
        printf(" non-interleaved ");
        outputParameters.sampleFormat |= paNonInterleaved;
        }
    printf("channels.\n");

    err = Pa_OpenStream(&stream,
                        NULL,               /* No input. */
                        &outputParameters,
                        SAMPLE_RATE,        /* Sample rate. */
                        FRAMES_PER_BUFFER,  /* Frames per buffer. */
                        paClipOff,          /* Samples never out of range, no clipping. */
                        patestCallback,
                        &data);
    if (err == paNoError)
        {
        err = Pa_StartStream(stream);
        if (err == paNoError)
            {
            printf("Hit ENTER to stop this test.\n");
            getchar();
            err = Pa_StopStream(stream);
            }
        Pa_CloseStream( stream );
        }
    return err;    
}


/*******************************************************************/
int main(void)
{
    PaError err;

    printf("PortAudio Test: output sine wave on each channel.\n" );

    err = Pa_Initialize();
    if (err != paNoError)
        goto done;

    err = test(1);          /* 1 means interleaved. */
    if (err != paNoError)
        goto done;

    err = test(0);          /* 0 means not interleaved. */
    if (err != paNoError)
        goto done;

    printf("Test finished.\n");
done:
    if (err)
        {
        fprintf(stderr, "An error occured while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err );
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        }
    Pa_Terminate();
    return 0;
}
