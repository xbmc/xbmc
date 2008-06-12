/*
 * $Id: debug_record_reuse.c 1083 2006-08-23 07:30:49Z rossb $
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
#define SAMPLE_RATE     (22050)
#define NUM_SECONDS     (4)
#define SLEEP_DUR_MSEC  (200)
#define FRAMES_PER_BUFFER  (256)
#define NUM_REC_BUFS    (0)

#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#else
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#endif

typedef struct
{
    long         frameIndex;  /* Index into sample array. */
    long         maxFrameIndex;
    long         samplesPerFrame;
    long         numSamples;
    PortAudioStream *outputStream;
    PortAudioStream *inputStream;
    SAMPLE      *recordedSamples;
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
    paTestData *data = (paTestData*)userData;
    SAMPLE *rptr = (SAMPLE*)inputBuffer;
    SAMPLE *wptr = &data->recordedSamples[data->frameIndex * data->samplesPerFrame];
    long framesToCalc;
    unsigned long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) outTime;

    if( framesLeft < framesPerBuffer )
    {
        framesToCalc = framesLeft;
        finished = 1;
    }
    else
    {
        framesToCalc = framesPerBuffer;
        finished = 0;
    }
    if( inputBuffer == NULL )
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = 0;  /* left */
            *wptr++ = 0;  /* right */
        }
    }
    else
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            *wptr++ = *rptr++;  /* right */
        }
    }
    data->frameIndex += framesToCalc;
    return finished;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int playCallback( void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         PaTimestamp outTime, void *userData )
{
    paTestData *data = (paTestData*)userData;
    SAMPLE *rptr = &data->recordedSamples[data->frameIndex * data->samplesPerFrame];
    SAMPLE *wptr = (SAMPLE*)outputBuffer;
    unsigned long i;
    int finished;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;
    if( outputBuffer == NULL ) return 0;
    (void) inputBuffer; /* Prevent unused variable warnings. */
    (void) outTime;

    if( framesLeft < framesPerBuffer )
    {
        /* final buffer... */
        for( i=0; i<framesLeft; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            *wptr++ = *rptr++;  /* right */
        }
        for( ; i<framesPerBuffer; i++ )
        {
            *wptr++ = 0;  /* left */
            *wptr++ = 0;  /* right */
        }
        data->frameIndex += framesLeft;
        finished = 1;
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            *wptr++ = *rptr++;  /* right */
        }
        data->frameIndex += framesPerBuffer;
        finished = 0;
    }
    return finished;
}

/****************************************************************/
PaError TestRecording( paTestData *dataPtr )
{
    PaError    err;
    int        i;
    int        lastIndex = 0;

/* Open input stream if not already open. */
    if( dataPtr->inputStream == NULL )
    {
        /* Record some audio. */
        err = Pa_OpenStream(
                  &dataPtr->inputStream,
                  Pa_GetDefaultInputDeviceID(),
                  dataPtr->samplesPerFrame,               /* stereo input */
                  PA_SAMPLE_TYPE,
                  NULL,
                  paNoDevice,
                  0,
                  PA_SAMPLE_TYPE,
                  NULL,
                  SAMPLE_RATE,
                  FRAMES_PER_BUFFER,            /* frames per buffer */
                  NUM_REC_BUFS,               /* number of buffers, if zero then use default minimum */
                  paClipOff,       /* we won't output out of range samples so don't bother clipping them */
                  recordCallback,
                  dataPtr );
        if( err != paNoError ) goto error;
    }

    dataPtr->frameIndex = 0;

    err = Pa_StartStream( dataPtr->inputStream );
    if( err != paNoError ) goto error;

    printf("Now recording!\n"); fflush(stdout);
    for( i=0; i<(NUM_SECONDS*1000/SLEEP_DUR_MSEC); i++ )
    {
        int frameIndex, delta;
        Pa_Sleep(SLEEP_DUR_MSEC);

        frameIndex = dataPtr->frameIndex;
        if( Pa_StreamActive( dataPtr->inputStream ) <= 0)
        {
            printf("Stream inactive!\n");
            break;
        }
        if( dataPtr->maxFrameIndex <= frameIndex )
        {
            printf("Buffer recording complete.\n");
            break;
        }

        delta = frameIndex - lastIndex;
        lastIndex = frameIndex;
        printf("index = %d, delta = %d\n", frameIndex, delta ); fflush(stdout);
    }

    err = Pa_StopStream( dataPtr->inputStream );
    if( err != paNoError ) goto error;

    printf("Done.\n"); fflush(stdout);

error:
    return err;
}

/****************************************************************/
PaError TestPlayback( paTestData *dataPtr )
{
    PaError    err;
    int        i;
    int        lastIndex = 0;

    /* Playback recorded data. */
    dataPtr->frameIndex = 0;
    printf("Begin playback.\n"); fflush(stdout);

/* Open output stream if not already open. */
    if( dataPtr->outputStream == NULL )
    {
        err = Pa_OpenStream(
                  &dataPtr->outputStream,
                  paNoDevice,
                  0,               /* NO input */
                  PA_SAMPLE_TYPE,
                  NULL,
                  Pa_GetDefaultOutputDeviceID(),
                  dataPtr->samplesPerFrame,               /* stereo output */
                  PA_SAMPLE_TYPE,
                  NULL,
                  SAMPLE_RATE,
                  FRAMES_PER_BUFFER,            /* frames per buffer */
                  0,               /* number of buffers, if zero then use default minimum */
                  paClipOff,       /* we won't output out of range samples so don't bother clipping them */
                  playCallback,
                  dataPtr );
        if( err != paNoError ) goto error;
    }

    err = Pa_StartStream( dataPtr->outputStream );
    if( err != paNoError ) goto error;

    printf("Waiting for playback to finish.\n"); fflush(stdout);
    for( i=0; i<(NUM_SECONDS*1000/SLEEP_DUR_MSEC); i++ )
    {
        int frameIndex, delta;
        Pa_Sleep(SLEEP_DUR_MSEC);
        frameIndex = dataPtr->frameIndex;
        delta = frameIndex - lastIndex;
        lastIndex = frameIndex;
        printf("index = %d, delta = %d\n", frameIndex, delta ); fflush(stdout);
    }

    err = Pa_StopStream( dataPtr->outputStream );
    if( err != paNoError ) goto error;
    
error:
    return err;
}
/*******************************************************************/
int main(void);
int main(void)
{
    PaError    err;
    paTestData data = { 0 };
    long       totalFrames;
    long       numBytes;
    long       i;
    printf("patest_record.c\n"); fflush(stdout);

/* Set up test data structure and sample array. */
    data.frameIndex = 0;
    data.samplesPerFrame = 2;
    data.maxFrameIndex = totalFrames = NUM_SECONDS*SAMPLE_RATE;

    printf("totalFrames = %d\n", totalFrames ); fflush(stdout);
    data.numSamples = totalFrames * data.samplesPerFrame;

    numBytes = data.numSamples * sizeof(SAMPLE);
    data.recordedSamples = (SAMPLE *) malloc( numBytes );
    if( data.recordedSamples == NULL )
    {
        printf("Could not allocate record array.\n");
        exit(1);
    }
    for( i=0; i<data.numSamples; i++ ) data.recordedSamples[i] = 0;

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

/* Record and playback multiple times. */
    for( i=0; i<2; i++ )
    {
        err = TestRecording( &data );
        if( err != paNoError ) goto error;

        err = TestPlayback( &data );
        if( err != paNoError ) goto error;
    }

/* Clean up. */
    err = Pa_CloseStream( data.inputStream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( data.outputStream );
    if( err != paNoError ) goto error;

    if( err != paNoError ) goto error;

    free( data.recordedSamples );
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
