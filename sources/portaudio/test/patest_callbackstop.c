/** @file patest_callbackstop.c
	@ingroup test_src
	@brief Test the paComplete callback result code.
	@author Ross Bencina <rossb@audiomulch.com>
*/
/*
 * $Id: patest_callbackstop.c 1294 2007-10-24 20:51:22Z bjornroche $
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com/
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

#define NUM_SECONDS   (5)
#define NUM_LOOPS     (4)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (67)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)
typedef struct
{
    float sine[TABLE_SIZE];
    int phase;
    unsigned long generatedFramesCount;
    volatile int callbackReturnedPaComplete;
    volatile int callbackInvokedAfterReturningPaComplete;
    char message[100];
}
TestData;

/*
   This routine will be called by the PortAudio stream when audio is needed.
   It may be called at interrupt level on some machines so don't do anything
   that could mess up the system like calling malloc() or free().
*/
static int TestCallback( const void *input, void *output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    TestData *data = (TestData*)userData;
    float *out = (float*)output;
    unsigned long i;
    float x;

    (void) input;       /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;

    
    if( data->callbackReturnedPaComplete )
        data->callbackInvokedAfterReturningPaComplete = 1;

    for( i=0; i<frameCount; i++ )
    {
        /* generate tone */
        
        x = data->sine[ data->phase++ ];
        if( data->phase >= TABLE_SIZE )
            data->phase -= TABLE_SIZE;
        
        *out++ = x;  /* left */
        *out++ = x;  /* right */
    }

    data->generatedFramesCount += frameCount;
    if( data->generatedFramesCount >= (NUM_SECONDS * SAMPLE_RATE) )
    {
        data->callbackReturnedPaComplete = 1;
        return paComplete;
    }
    else
    {
        return paContinue;
    }
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
   TestData *data = (TestData *) userData;
   printf( "Stream Completed: %s\n", data->message );
}


/*----------------------------------------------------------------------------*/
int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    TestData data;
    int i, j;

    
    printf( "PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
            SAMPLE_RATE, FRAMES_PER_BUFFER );
    
    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device                    = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount              = 2;               /* stereo output */
    outputParameters.sampleFormat              = paFloat32;       /* 32 bit floating point output */
    outputParameters.suggestedLatency          = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* output will be in-range, so no need to clip */
              TestCallback,
              &data );
    if( err != paNoError ) goto error;

    sprintf( data.message, "Loop: XX" );
    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
    if( err != paNoError ) goto error;

    printf("Repeating test %d times.\n", NUM_LOOPS );
    
    for( i=0; i < NUM_LOOPS; ++i )
    {
        data.phase = 0;
        data.generatedFramesCount = 0;
        data.callbackReturnedPaComplete = 0;
        data.callbackInvokedAfterReturningPaComplete = 0;
        sprintf( data.message, "Loop: %d", i );

        err = Pa_StartStream( stream );
        if( err != paNoError ) goto error;

        printf("Play for %d seconds.\n", NUM_SECONDS );

        /* wait for the callback to complete generating NUM_SECONDS of tone */

        do
        {
            Pa_Sleep( 500 );
        }
        while( !data.callbackReturnedPaComplete );

        printf( "Callback returned paComplete.\n" );
        printf( "Waiting for buffers to finish playing...\n" );

        /* wait for stream to become inactive,
           or for a timeout of approximately NUM_SECONDS
         */
     
        j = 0;
        while( (err = Pa_IsStreamActive( stream )) == 1 && j < NUM_SECONDS * 2 )
        {
            printf(".\n" );
            Pa_Sleep( 500 );
            ++j;
        }

        if( err < 0 )
        {
            goto error;
        }
        else if( err == 1 )
        {
            printf( "TEST FAILED: Timed out waiting for buffers to finish playing.\n" );
        }
        else
        {
            printf("Buffers finished.\n" );
        }

        if( data.callbackInvokedAfterReturningPaComplete )
        {
            printf( "TEST FAILED: Callback was invoked after returning paComplete.\n" );
        }


        err = Pa_StopStream( stream );
        if( err != paNoError ) goto error;

        printf( "sleeping for 1 second...\n" );
        Pa_Sleep( 1000 );
    }

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
