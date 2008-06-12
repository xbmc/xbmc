/** @file patest_many.c
	@ingroup test_src
	@brief Start and stop the PortAudio Driver multiple times.
	@author Phil Burk  http://www.softsynth.com
*/
/*
 * $Id: patest_many.c 1097 2006-08-26 08:27:53Z rossb $
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
#include <stdlib.h>
#include <math.h>
#include "portaudio.h"
#define NUM_SECONDS   (1)
#define SAMPLE_RATE   (44100)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TABLE_SIZE   (200)
typedef struct
{
    short sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    unsigned int sampsToGo;
}
paTestData;
PaError TestOnce( void );
static int patest1Callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData );

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patest1Callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    paTestData *data = (paTestData*)userData;
    short *out = (short*)outputBuffer;
    unsigned int i;
    int finished = 0;
    (void) inputBuffer; /* Prevent "unused variable" warnings. */

    if( data->sampsToGo < framesPerBuffer )
    {
        /* final buffer... */

        for( i=0; i<data->sampsToGo; i++ )
        {
            *out++ = data->sine[data->left_phase];  /* left */
            *out++ = data->sine[data->right_phase];  /* right */
            data->left_phase += 1;
            if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
            data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
            if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
        }
        /* zero remainder of final buffer */
        for( ; i<framesPerBuffer; i++ )
        {
            *out++ = 0; /* left */
            *out++ = 0; /* right */
        }

        finished = 1;
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *out++ = data->sine[data->left_phase];  /* left */
            *out++ = data->sine[data->right_phase];  /* right */
            data->left_phase += 1;
            if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
            data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
            if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
        }
        data->sampsToGo -= framesPerBuffer;
    }
    return finished;
}
/*******************************************************************/
#ifdef MACINTOSH
int main(void);
int main(void)
{
    int i;
    PaError err;
    int numLoops = 10;
    printf("Loop %d times.\n", numLoops );
    for( i=0; i<numLoops; i++ )
    {
        printf("Loop %d out of %d.\n", i+1, numLoops );
        err = TestOnce();
        if( err < 0 ) return 0;
    }
}
#else
int main(int argc, char **argv);
int main(int argc, char **argv)
{
    PaError err;
    int i, numLoops = 10;
    if( argc > 1 )
    {
        numLoops = atoi(argv[1]);
    }
    for( i=0; i<numLoops; i++ )
    {
        printf("Loop %d out of %d.\n", i+1, numLoops );
        err = TestOnce();
        if( err < 0 ) return 1;
    }
    printf("Test complete.\n");
    return 0;
}
#endif
PaError TestOnce( void )
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int i;
    int totalSamps;
    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (short) (32767.0 * sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. ));
    }
    data.left_phase = data.right_phase = 0;
    data.sampsToGo = totalSamps =  NUM_SECONDS * SAMPLE_RATE; /* Play for a few seconds. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice();  /* default output device */
    outputParameters.channelCount = 2;                      /* stereo output */
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(
              &stream,
              NULL,         /* no input */
              &outputParameters,
              SAMPLE_RATE,
              1024,         /* frames per buffer */
              paClipOff,    /* we won't output out of range samples so don't bother clipping them */
              patest1Callback,
              &data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    printf("Waiting for sound to finish.\n");
    Pa_Sleep(1000);
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    Pa_Terminate();
    return paNoError;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
