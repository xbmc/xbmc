/** @file pa_minlat.c
	@ingroup test_src
    @brief Experiment with different numbers of buffers to determine the
	minimum latency for a computer.
	@author Phil Burk  http://www.softsynth.com
*/
/*
 * $Id: pa_minlat.c 1097 2006-08-26 08:27:53Z rossb $
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
#include <string.h>
#include "portaudio.h"

#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TWOPI (M_PI * 2.0)

#define DEFAULT_BUFFER_SIZE   (32)

typedef struct
{
    double left_phase;
    double right_phase;
}
paTestData;

/* Very simple synthesis routine to generate two sine waves. */
static int paminlatCallback( const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned int i;
    double left_phaseInc = 0.02;
    double right_phaseInc = 0.06;

    double left_phase = data->left_phase;
    double right_phase = data->right_phase;

    for( i=0; i<framesPerBuffer; i++ )
    {
        left_phase += left_phaseInc;
        if( left_phase > TWOPI ) left_phase -= TWOPI;
        *out++ = (float) sin( left_phase );

        right_phase += right_phaseInc;
        if( right_phase > TWOPI ) right_phase -= TWOPI;
        *out++ = (float) sin( right_phase );
    }

    data->left_phase = left_phase;
    data->right_phase = right_phase;
    return 0;
}

int main( int argc, char **argv );
int main( int argc, char **argv )
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int    go;
    int    outLatency = 0;
    int    minLatency = DEFAULT_BUFFER_SIZE * 2;
    int    framesPerBuffer;
    double sampleRate = 44100.0;
    char   str[256];

    printf("pa_minlat - Determine minimum latency for your computer.\n");
    printf("  usage:         pa_minlat {userBufferSize}\n");
    printf("  for example:   pa_minlat 64\n");
    printf("Adjust your stereo until you hear a smooth tone in each speaker.\n");
    printf("Then try to find the smallest number of frames that still sounds smooth.\n");
    printf("Note that the sound will stop momentarily when you change the number of buffers.\n");

    /* Get bufferSize from command line. */
    framesPerBuffer = ( argc > 1 ) ? atol( argv[1] ) : DEFAULT_BUFFER_SIZE;
    printf("Frames per buffer = %d\n", framesPerBuffer );

    data.left_phase = data.right_phase = 0.0;

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outLatency = sampleRate * 200.0 / 1000.0; /* 200 msec. */

    /* Try different numBuffers in a loop. */
    go = 1;
    while( go )
    {
        outputParameters.device                    = Pa_GetDefaultOutputDevice(); /* Default output device. */
        outputParameters.channelCount              = 2;                           /* Stereo output */
        outputParameters.sampleFormat              = paFloat32;                   /* 32 bit floating point output. */
        outputParameters.suggestedLatency          = (double)outLatency / sampleRate; /* In seconds. */
        outputParameters.hostApiSpecificStreamInfo = NULL;
        
        printf("Latency = %d frames = %6.1f msec.\n", outLatency, outputParameters.suggestedLatency * 1000.0 );

        err = Pa_OpenStream(
                  &stream,
                  NULL, /* no input */
                  &outputParameters,
                  sampleRate,
                  framesPerBuffer,
                  paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                  paminlatCallback,
                  &data );
        if( err != paNoError ) goto error;
        if( stream == NULL ) goto error;

        /* Start audio. */
        err = Pa_StartStream( stream );
        if( err != paNoError ) goto error;

        /* Ask user for a new nlatency. */
        printf("\nMove windows around to see if the sound glitches.\n");
        printf("Latency now %d, enter new number of frames, or 'q' to quit: ", outLatency );
        fgets( str, 256, stdin );
        {
            /* Get rid of newline */
            size_t l = strlen( str ) - 1;
            if( str[ l ] == '\n')
                str[ l ] = '\0';
        }
        if( str[0] == 'q' ) go = 0;
        else
        {
            outLatency = atol( str );
            if( outLatency < minLatency )
            {
                printf( "Latency below minimum of %d! Set to minimum!!!\n", minLatency );
                outLatency = minLatency;
            }
        }
        /* Stop sound until ENTER hit. */
        err = Pa_StopStream( stream );
        if( err != paNoError ) goto error;
        err = Pa_CloseStream( stream );
        if( err != paNoError ) goto error;
    }
    printf("A good setting for latency would be somewhat higher than\n");
    printf("the minimum latency that worked.\n");
    printf("PortAudio: Test finished.\n");
    Pa_Terminate();
    return 0;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return 1;
}
