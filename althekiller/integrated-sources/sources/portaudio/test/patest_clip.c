/** @file patest_clip.c
	@ingroup test_src
	@brief Play a sine wave for several seconds at an amplitude 
	that would require clipping.

	@author Phil Burk  http://www.softsynth.com
*/
/*
 * $Id: patest_clip.c 1097 2006-08-26 08:27:53Z rossb $
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

#define NUM_SECONDS   (4)
#define SAMPLE_RATE   (44100)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TABLE_SIZE   (200)

typedef struct paTestData
{
    float sine[TABLE_SIZE];
    float amplitude;
    int left_phase;
    int right_phase;
}
paTestData;

PaError PlaySine( paTestData *data, unsigned long flags, float amplitude );

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int sineCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    float amplitude = data->amplitude;
    unsigned int i;
    (void) inputBuffer; /* Prevent "unused variable" warnings. */
    (void) timeInfo;
    (void) statusFlags;

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = amplitude * data->sine[data->left_phase];  /* left */
        *out++ = amplitude * data->sine[data->right_phase];  /* right */
        data->left_phase += 1;
        if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
        data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
        if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
    }
    return 0;
}
/*******************************************************************/
int main(void);
int main(void)
{
    PaError err;
    paTestData data;
    int i;

    printf("PortAudio Test: output sine wave with and without clipping.\n");
    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }

    printf("\nHalf amplitude. Should sound like sine wave.\n"); fflush(stdout);
    err = PlaySine( &data, paClipOff | paDitherOff, 0.5f );
    if( err < 0 ) goto error;

    printf("\nFull amplitude. Should sound like sine wave.\n"); fflush(stdout);
    err = PlaySine( &data, paClipOff | paDitherOff, 0.999f );
    if( err < 0 ) goto error;

    printf("\nOver range with clipping and dithering turned OFF. Should sound very nasty.\n");
    fflush(stdout);
    err = PlaySine( &data, paClipOff | paDitherOff, 1.1f );
    if( err < 0 ) goto error;

    printf("\nOver range with clipping and dithering turned ON.  Should sound smoother than previous.\n");
    fflush(stdout);
    err = PlaySine( &data, paNoFlag, 1.1f );
    if( err < 0 ) goto error;

    printf("\nOver range with paClipOff but dithering ON.\n"
           "That forces clipping ON so it should sound the same as previous.\n");
    fflush(stdout);
    err = PlaySine( &data, paClipOff, 1.1f );
    if( err < 0 ) goto error;
    
    return 0;
error:
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return 1;
}
/*****************************************************************************/
PaError PlaySine( paTestData *data, unsigned long flags, float amplitude )
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;

    data->left_phase = data->right_phase = 0;
    data->amplitude = amplitude;
    
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              1024,
              flags,
              sineCallback,
              data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    Pa_Sleep( NUM_SECONDS * 1000 );
    printf("CPULoad = %8.6f\n", Pa_GetStreamCpuLoad( stream ) );

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    
    Pa_Terminate();
    return paNoError;
error:
    return err;
}
