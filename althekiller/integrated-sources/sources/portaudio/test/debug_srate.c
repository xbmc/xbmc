/*
 * $Id: debug_srate.c 1083 2006-08-23 07:30:49Z rossb $
 * debug_record_reuse.c
 * Record input into an array.
 * Save array to a file.
 * Based on patest_record.c but with various ugly debug hacks thrown in.
 * Loop twice and reuse same streams.
 *
 * Author: Phil Burk  http://www.softsynth.com
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
#include <memory.h>
#include "portaudio.h"

#define EWS88MT_12_REC     (1)
#define EWS88MT_12_PLAY   (10)
#define SBLIVE_REC         (2)
#define SBLIVE_PLAY       (11)

#if 0
#define INPUT_DEVICE_ID    Pa_GetDefaultInputDeviceID()
#define OUTPUT_DEVICE_ID   Pa_GetDefaultOutputDeviceID()
#else
#define INPUT_DEVICE_ID    (EWS88MT_12_REC)
#define OUTPUT_DEVICE_ID   (SBLIVE_PLAY)
#endif

#define INPUT_SAMPLE_RATE     (22050.0)
#define OUTPUT_SAMPLE_RATE    (22050.0)
#define NUM_SECONDS               (4)
#define SLEEP_DUR_MSEC         (1000)
#define FRAMES_PER_BUFFER        (64)
#define NUM_REC_BUFS              (0)
#define SAMPLES_PER_FRAME         (2)

#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;

typedef struct
{
    long             frameIndex;  /* Index into sample array. */
}
paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int recordCallback( void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           PaTimestamp outTime, void *userData )
{
    paTestData *data = (paTestData *) userData;
    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) outTime;

    if( inputBuffer != NULL )
    {
        data->frameIndex += framesPerBuffer;
    }
    return 0;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int playCallback( void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         PaTimestamp outTime, void *userData )
{
    paTestData *data = (paTestData *) userData;
    (void) inputBuffer; /* Prevent unused variable warnings. */
    (void) outTime;

    if( outputBuffer != NULL )
    {
        data->frameIndex += framesPerBuffer;
    }
    return 0;
}

/****************************************************************/
PaError MeasureStreamRate( PortAudioStream *stream, paTestData *dataPtr, double *ratePtr )
{
    PaError    err;
    int        i;
    int        totalFrames = 0;
    int        totalMSec = 0;

    dataPtr->frameIndex = 0;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    for( i=0; i<(NUM_SECONDS*1000/SLEEP_DUR_MSEC); i++ )
    {
        int delta, endIndex;

        int startIndex = dataPtr->frameIndex;
        Pa_Sleep(SLEEP_DUR_MSEC);
        endIndex = dataPtr->frameIndex;

        delta = endIndex - startIndex;
        totalFrames += delta;
        totalMSec += SLEEP_DUR_MSEC;

        printf("index = %d, delta = %d\n", endIndex, delta ); fflush(stdout);
    }

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    *ratePtr = (totalFrames * 1000.0) / totalMSec;

error:
    return err;
}

void ReportRate( double measuredRate, double expectedRate )
{
    double error;

    error = (measuredRate - expectedRate) / expectedRate;
    error = (error < 0 ) ? -error : error;

    printf("Measured rate = %6.1f, expected rate = %6.1f\n",
            measuredRate, expectedRate );
    if( error > 0.1 )
    {
        printf("ERROR: unexpected rate! ---------------------   ERROR!\n");
    }
    else
    {
        printf("SUCCESS: rate within tolerance!\n");
    }
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaError    err;
    paTestData data = { 0 };
    long       i;
    double     rate;
    const    PaDeviceInfo *pdi;

    PortAudioStream *outputStream;
    PortAudioStream *inputStream;

    err = Pa_Initialize();
    if( err != paNoError ) goto error;


    pdi = Pa_GetDeviceInfo( INPUT_DEVICE_ID );
    printf("Input device  = %s\n", pdi->name );
    pdi = Pa_GetDeviceInfo( OUTPUT_DEVICE_ID );
    printf("Output device = %s\n", pdi->name );

/* Open input stream. */
    err = Pa_OpenStream(
              &inputStream,
              INPUT_DEVICE_ID,
              SAMPLES_PER_FRAME,               /* stereo input */
              PA_SAMPLE_TYPE,
              NULL,
              paNoDevice,
              0,
              PA_SAMPLE_TYPE,
              NULL,
              INPUT_SAMPLE_RATE,
              FRAMES_PER_BUFFER,            /* frames per buffer */
              NUM_REC_BUFS,               /* number of buffers, if zero then use default minimum */
              paClipOff,       /* we won't output out of range samples so don't bother clipping them */
              recordCallback,
              &data );
    if( err != paNoError ) goto error;

    err = Pa_OpenStream(
              &outputStream,
              paNoDevice,
              0,               /* NO input */
              PA_SAMPLE_TYPE,
              NULL,
              OUTPUT_DEVICE_ID,
              SAMPLES_PER_FRAME,               /* stereo output */
              PA_SAMPLE_TYPE,
              NULL,
              OUTPUT_SAMPLE_RATE,
              FRAMES_PER_BUFFER,            /* frames per buffer */
              0,               /* number of buffers, if zero then use default minimum */
              paClipOff,       /* we won't output out of range samples so don't bother clipping them */
              playCallback,
              &data );
    if( err != paNoError ) goto error;

/* Record and playback multiple times. */
    for( i=0; i<2; i++ )
    {
        printf("Measuring INPUT ------------------------- \n");
        err = MeasureStreamRate( inputStream, &data, &rate );
        if( err != paNoError ) goto error;
        ReportRate( rate, INPUT_SAMPLE_RATE );

        printf("Measuring OUTPUT ------------------------- \n");
        err = MeasureStreamRate( outputStream, &data, &rate );
        if( err != paNoError ) goto error;
        ReportRate( rate, OUTPUT_SAMPLE_RATE );
    }

/* Clean up. */
    err = Pa_CloseStream( inputStream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( outputStream );
    if( err != paNoError ) goto error;

    if( err != paNoError ) goto error;

    Pa_Terminate();
    
    printf("Test complete.\n"); fflush(stdout);
    return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    if( err == paHostError )
    {
        fprintf( stderr, "Host Error number: %d\n", Pa_GetHostError() );
    }
    return -1;
}
