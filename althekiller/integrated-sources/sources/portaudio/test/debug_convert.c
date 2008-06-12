/*
 * $Id: debug_convert.c 1083 2006-08-23 07:30:49Z rossb $
 * Convert tagged values.
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
#define OUTPUT_DEVICE       (Pa_GetDefaultOutputDeviceID())
//#define OUTPUT_DEVICE       (11)
#define NUM_SECONDS         (8)
#define SLEEP_DUR           (800)
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (256)

#define NUM_BUFFERS         (0)

typedef struct
{
    unsigned int framesToGo;
}
paTestData;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback( void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           PaTimestamp outTime, void *userData )
{
    paTestData *data = (paTestData*)userData;
    short *out = (short*)outputBuffer;
    int i;
    int finished = 0;
    (void) outTime; /* Prevent unused variable warnings. */
    (void) inputBuffer;

    if( data->framesToGo < framesPerBuffer )  finished = 1;

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = 0x0000 + i;  /* left */
        *out++ = 0x1000 + i;  /* right */
    }
    return finished;
}
/*******************************************************************/
int main(void);
int main(void)
{
    PortAudioStream *stream;
    PaError err;
    paTestData data;
    int i;
    int totalSamps;
    printf("PortAudio Test: output debug values\n" );
    data.framesToGo = totalSamps =  NUM_SECONDS * SAMPLE_RATE; /* Play for a few seconds. */
    printf("totalSamps = %d\n", totalSamps );
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    printf("PortAudio Test: output device = %d\n", OUTPUT_DEVICE );
    
    err = Pa_OpenStream(
              &stream,
              paNoDevice,
              0,              /* no input */
              paFloat32,  /* 32 bit floating point input */
              NULL,
              OUTPUT_DEVICE,
              2,              /* stereo output */
              paInt16,      /* 32 bit floating point output */
              NULL,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              NUM_BUFFERS,    /* number of buffers, if zero then use default minimum */
              paClipOff|paDitherOff, /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) goto error;
    
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    printf("Is callback being called?\n");
    for( i=0; i<((NUM_SECONDS+1)*1000); i+=SLEEP_DUR )
    {
        printf("data.framesToGo = %d\n", data.framesToGo ); fflush(stdout);
        Pa_Sleep( SLEEP_DUR );
    }
    /* Stop sound until ENTER hit. */
    printf("Call Pa_StopStream()\n");
    err = Pa_StopStream( stream );
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
