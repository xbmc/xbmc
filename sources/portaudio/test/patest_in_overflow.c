/** @file patest_in_overflow.c
	@ingroup test_src
	@brief Count input overflows (using paInputOverflow flag) under 
	overloaded and normal conditions.
    This test uses the same method to overload the stream as does
    patest_out_underflow.c -- it generates sine waves until the cpu load
    exceeds a certain level. However this test is only concerned with
    input and so doesn't ouput any sound.
    
    @author Ross Bencina <rossb@audiomulch.com>
	@author Phil Burk <philburk@softsynth.com>
*/
/*
 * $Id: patest_in_overflow.c 1097 2006-08-26 08:27:53Z rossb $
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2004 Ross Bencina and Phil Burk
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

#define MAX_SINES     (500)
#define MAX_LOAD      (1.2)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (512)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TWOPI (M_PI * 2.0)

typedef struct paTestData
{
    int sineCount;
    double phases[MAX_SINES];
    int countOverflows;
    int inputOverflowCount;
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
    float out;          /* variable to hold dummy output */
    unsigned long i;
    int j;
    int finished = paContinue;
    (void) timeInfo;    /* Prevent unused variable warning. */
    (void) inputBuffer; /* Prevent unused variable warning. */
    (void) outputBuffer; /* Prevent unused variable warning. */

    if( data->countOverflows && (statusFlags & paInputOverflow) )
        data->inputOverflowCount++;

    for( i=0; i<framesPerBuffer; i++ )
    {
        float output = 0.0;
        double phaseInc = 0.02;
        double phase;

        for( j=0; j<data->sineCount; j++ )
        {
            /* Advance phase of next oscillator. */
            phase = data->phases[j];
            phase += phaseInc;
            if( phase > TWOPI ) phase -= TWOPI;

            phaseInc *= 1.02;
            if( phaseInc > 0.5 ) phaseInc *= 0.5;

            /* This is not a very efficient way to calc sines. */
            output += (float) sin( phase );
            data->phases[j] = phase;
        }
        /* this is an input-only stream so we don't actually use the output */
        out = (float) (output / data->sineCount);
        (void) out; /* suppress unused variable warning*/
    }

    return finished;
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters inputParameters;
    PaStream *stream;
    PaError err;
    int safeSineCount, stressedSineCount;
    int safeOverflowCount, stressedOverflowCount;
    paTestData data = {0};
    double load;


    printf("PortAudio Test: input only, no sound output. Load callback by performing calculations, count input overflows. SR = %d, BufSize = %d. MAX_LOAD = %f\n",
        SAMPLE_RATE, FRAMES_PER_BUFFER, (float)MAX_LOAD );

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    inputParameters.device = Pa_GetDefaultInputDevice();  /* default input device */
    inputParameters.channelCount = 1;                      /* mono output */
    inputParameters.sampleFormat = paFloat32;              /* 32 bit floating point output */
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              NULL,    /* no output */
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,    /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );    
    if( err != paNoError ) goto error;
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    printf("Establishing load conditions...\n" );

    /* Determine number of sines required to get to 50% */
    do
    {
        data.sineCount++;
        Pa_Sleep( 100 );

        load = Pa_GetStreamCpuLoad( stream );
        printf("sineCount = %d, CPU load = %f\n", data.sineCount, load );
    }
    while( load < 0.5 && data.sineCount < (MAX_SINES-1));

    safeSineCount = data.sineCount;

    /* Calculate target stress value then ramp up to that level*/
    stressedSineCount = (int) (2.0 * data.sineCount * MAX_LOAD );
    if( stressedSineCount > MAX_SINES )
        stressedSineCount = MAX_SINES;
    for( ; data.sineCount < stressedSineCount; data.sineCount++ )
    {
        Pa_Sleep( 100 );
        load = Pa_GetStreamCpuLoad( stream );
        printf("STRESSING: sineCount = %d, CPU load = %f\n", data.sineCount, load );
    }
    
    printf("Counting overflows for 5 seconds.\n");
    data.countOverflows = 1;
    Pa_Sleep( 5000 );

    stressedOverflowCount = data.inputOverflowCount;

    data.countOverflows = 0;
    data.sineCount = safeSineCount;

    printf("Resuming safe load...\n");
    Pa_Sleep( 1500 );
    data.inputOverflowCount = 0;
    Pa_Sleep( 1500 );
    load = Pa_GetStreamCpuLoad( stream );
    printf("sineCount = %d, CPU load = %f\n", data.sineCount, load );

    printf("Counting overflows for 5 seconds.\n");
    data.countOverflows = 1;
    Pa_Sleep( 5000 );

    safeOverflowCount = data.inputOverflowCount;
    
    printf("Stop stream.\n");
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    
    Pa_Terminate();

    if( stressedOverflowCount == 0 )
        printf("Test failed, no input overflows detected under stress.\n");
    else if( safeOverflowCount != 0 )
        printf("Test failed, %d unexpected overflows detected under safe load.\n", safeOverflowCount);
    else
        printf("Test passed, %d expected input overflows detected under stress, 0 unexpected overflows detected under safe load.\n", stressedOverflowCount );

    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
