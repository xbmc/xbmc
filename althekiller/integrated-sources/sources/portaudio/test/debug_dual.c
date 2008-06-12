/*
 * $Id: debug_dual.c 1083 2006-08-23 07:30:49Z rossb $
 * debug_dual.c
 * Try to open TWO streams on separate cards.
 * Play a sine sweep using the Portable Audio api for several seconds.
 * Hacked test for debugging PA.
 *
 * Author: Phil Burk <philburk@softsynth.com>
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
#define DEV_ID_1            (13)
#define DEV_ID_2            (15)
#define NUM_SECONDS         (8)
#define SLEEP_DUR           (800)
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (256)
#if 0
#define MIN_LATENCY_MSEC    (200)
#define NUM_BUFFERS         ((MIN_LATENCY_MSEC * SAMPLE_RATE) / (FRAMES_PER_BUFFER * 1000))
#else
#define NUM_BUFFERS         (0)
#endif
#define MIN_FREQ            (100.0f)
#define MAX_FREQ            (4000.0f)
#define FREQ_SCALAR         (1.00002f)
#define CalcPhaseIncrement(freq)  (freq/SAMPLE_RATE)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TABLE_SIZE   (400)
typedef struct
{
    float sine[TABLE_SIZE + 1]; // add one for guard point for interpolation
    float phase_increment;
    float left_phase;
    float right_phase;
}
paTestData;
/* Convert phase between and 1.0 to sine value
 * using linear interpolation.
 */
float LookupSine( paTestData *data, float phase );
float LookupSine( paTestData *data, float phase )
{
    float fIndex = phase*TABLE_SIZE;
    int   index = (int) fIndex;
    float fract = fIndex - index;
    float lo = data->sine[index];
    float hi = data->sine[index+1];
    float val = lo + fract*(hi-lo);
    return val;
}
/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback( void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           PaTimestamp outTime, void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;
    int finished = 0;
    (void) outTime; /* Prevent unused variable warnings. */
    (void) inputBuffer;


    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = LookupSine(data, data->left_phase);  /* left */
        *out++ = LookupSine(data, data->right_phase);  /* right */
        data->left_phase += data->phase_increment;
        if( data->left_phase >= 1.0f ) data->left_phase -= 1.0f;
        data->right_phase += (data->phase_increment * 1.5f); /* fifth above */
        if( data->right_phase >= 1.0f ) data->right_phase -= 1.0f;
        /* sweep frequency then start over. */
        data->phase_increment *= FREQ_SCALAR;
        if( data->phase_increment > CalcPhaseIncrement(MAX_FREQ) ) data->phase_increment = CalcPhaseIncrement(MIN_FREQ);
    }
    return 0;
}

PaError TestStart( PortAudioStream **streamPtr, PaDeviceID devID,
                   paTestData *data );
/*******************************************************************/
int main(void);
int main(void)
{
    PortAudioStream *stream1, *stream2;
    PaError err;
    paTestData DATA1, DATA2;
    printf("PortAudio Test: DUAL sine sweep. ask for %d buffers\n", NUM_BUFFERS );
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    err = TestStart( &stream1, DEV_ID_1, &DATA1 );
    if( err != paNoError ) goto error;
    err = TestStart( &stream2, DEV_ID_2, &DATA2 );
    if( err != paNoError ) goto error;
    printf("Hit ENTER\n");
    getchar();
    err = Pa_StopStream( stream1 );
    if( err != paNoError ) goto error;
    err = Pa_StopStream( stream2 );
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
PaError TestStart( PortAudioStream **streamPtr, PaDeviceID devID, paTestData *data )
{
    PortAudioStream *stream;
    PaError err;
    int i;
    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data->sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data->sine[TABLE_SIZE] = data->sine[0]; // set guard point
    data->left_phase = data->right_phase = 0.0;
    data->phase_increment = CalcPhaseIncrement(MIN_FREQ);
    printf("PortAudio Test: output device = %d\n", devID );
    err = Pa_OpenStream(
              &stream,
              paNoDevice,
              0,              /* no input */
              paFloat32,  /* 32 bit floating point input */
              NULL,
              devID,
              2,              /* stereo output */
              paFloat32,      /* 32 bit floating point output */
              NULL,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              NUM_BUFFERS,    /* number of buffers, if zero then use default minimum */
              paClipOff|paDitherOff, /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              data );
    if( err != paNoError ) goto error;
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    *streamPtr = stream;
    return 0;
error:
    return err;
}
