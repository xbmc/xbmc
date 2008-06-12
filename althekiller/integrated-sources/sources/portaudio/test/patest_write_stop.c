/** @file patest_write_stop.c
	@brief Play a few seconds of silence followed by a few cycles of a sine wave. Tests to make sure that pa_StopStream() completes playback in blocking I/O
	@author Bjorn Roche of XO Audio (www.xoaudio.com)
	@author Ross Bencina
	@author Phil Burk
*/
/*
 * $Id: patest_write_stop.c 1083 2006-08-23 07:30:49Z rossb $
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

#define NUM_SECONDS         (5)
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (1024)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)


int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    float buffer[FRAMES_PER_BUFFER][2]; /* stereo output buffer */
    float sine[TABLE_SIZE]; /* sine wavetable */
    int left_phase = 0;
    int right_phase = 0;
    int left_inc = 1;
    int right_inc = 3; /* higher pitch so we can distinguish left and right. */
    int i, j;
    int bufferCount;
    const int   framesBy2  = FRAMES_PER_BUFFER >> 1;
    const float framesBy2f = (float) framesBy2 ;

    
    printf( "PortAudio Test: output silence, followed by one buffer of a ramped sine wave. SR = %d, BufSize = %d\n",
            SAMPLE_RATE, FRAMES_PER_BUFFER);
    
    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }

    
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency * 5;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* open the stream */
    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              NULL, /* no callback, use blocking API */
              NULL ); /* no callback, so no callback userData */
    if( err != paNoError ) goto error;

    /* start the stream */
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    printf("Playing %d seconds of silence followed by one buffer of a ramped sinusoid.\n", NUM_SECONDS );

    bufferCount = ((NUM_SECONDS * SAMPLE_RATE) / FRAMES_PER_BUFFER);

    /* clear buffer */
    for( j=0; j < FRAMES_PER_BUFFER; j++ )
    {
        buffer[j][0] = 0;  /* left */
        buffer[j][1] = 0;  /* right */
    }
    /* play the silent buffer a bunch o' times */
    for( i=0; i < bufferCount; i++ )
    {
        err = Pa_WriteStream( stream, buffer, FRAMES_PER_BUFFER );
        if( err != paNoError ) goto error;
    }   
    /* play a non-silent buffer once */
    for( j=0; j < FRAMES_PER_BUFFER; j++ )
    {
        float ramp = 1;
        if( j < framesBy2 )
           ramp = j / framesBy2f;
        else
           ramp = (FRAMES_PER_BUFFER - j) / framesBy2f ;

        buffer[j][0] = sine[left_phase] * ramp;  /* left */
        buffer[j][1] = sine[right_phase] * ramp;  /* right */
        left_phase += left_inc;
        if( left_phase >= TABLE_SIZE ) left_phase -= TABLE_SIZE;
        right_phase += right_inc;
        if( right_phase >= TABLE_SIZE ) right_phase -= TABLE_SIZE;
    }
    err = Pa_WriteStream( stream, buffer, FRAMES_PER_BUFFER );
    if( err != paNoError ) goto error;

    /* stop stream, close, and terminate */
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

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
