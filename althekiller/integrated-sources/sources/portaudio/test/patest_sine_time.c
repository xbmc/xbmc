/** @file patest_sine_time.c
	@ingroup test_src
	@brief Play a sine wave for several seconds, pausing in the middle.
	Uses the Pa_GetStreamTime() call.
	@author Ross Bencina <rossb@audiomulch.com>
	@author Phil Burk <philburk@softsynth.com>
*/
/*
 * $Id: patest_sine_time.c 1097 2006-08-26 08:27:53Z rossb $
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
#include "pa_util.h"

#define NUM_SECONDS   (8)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TWOPI (M_PI * 2.0)

#define TABLE_SIZE   (200)

typedef struct
{
    double           left_phase;
    double           right_phase;
    volatile PaTime  outTime;
}
paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback( const void *inputBuffer, void *outputBuffer,
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

    (void) statusFlags; /* Prevent unused variable warnings. */
    (void) inputBuffer;

    data->outTime = timeInfo->outputBufferDacTime;

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

    return paContinue;
}

/*******************************************************************/
static void ReportStreamTime( PaStream *stream, paTestData *data );
static void ReportStreamTime( PaStream *stream, paTestData *data )
{
    PaTime  streamTime, latency, outTime;
    
    streamTime = Pa_GetStreamTime( stream );
    outTime = data->outTime;
    if( outTime < 0.0 )
    {
        printf("Stream time = %8.1f\n", streamTime );
    }
    else
    {
        latency = outTime - streamTime;
        printf("Stream time = %8.4f, outTime = %8.4f, latency = %8.4f\n",
            streamTime, outTime, latency );
    }
    fflush(stdout);
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    PaTime startTime;

    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    data.left_phase = data.right_phase = 0;

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
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) goto error;
          
    /* Watch until sound is halfway finished. */
    printf("Play for %d seconds.\n", NUM_SECONDS/2 ); fflush(stdout);

    data.outTime = -1.0; /* mark time for callback as undefined */
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    startTime = Pa_GetStreamTime( stream );

    do
    {
        ReportStreamTime( stream, &data );
        Pa_Sleep(100);
    } while( (Pa_GetStreamTime( stream ) - startTime) < (NUM_SECONDS/2) );
    
    /* Stop sound for 2 seconds. */
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    
    printf("Pause for 2 seconds.\n"); fflush(stdout);
    Pa_Sleep( 2000 );

    data.outTime = -1.0; /* mark time for callback as undefined */
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    startTime = Pa_GetStreamTime( stream );
    
    printf("Play until sound is finished.\n"); fflush(stdout);
    do
    {
        ReportStreamTime( stream, &data );
        Pa_Sleep(100);
    } while( (Pa_GetStreamTime( stream ) - startTime) < (NUM_SECONDS/2) );
    
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    Pa_Terminate();
    printf("Test finished.\n");
    return err;
    
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
