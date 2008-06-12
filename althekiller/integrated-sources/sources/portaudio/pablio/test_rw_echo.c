/*
 * $Id: test_rw_echo.c 1083 2006-08-23 07:30:49Z rossb $
 * test_rw_echo.c
 * Echo delayed input to output.
 *
 * Author: Phil Burk, http://www.softsynth.com/portaudio/
 *
 * This program uses PABLIO, the Portable Audio Blocking I/O Library.
 * PABLIO is built on top of PortAudio, the Portable Audio Library.
 *
 * Note that if you need low latency, you should not use PABLIO.
 * Use the PA_OpenStream callback technique which is lower level
 * than PABLIO.
 *
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
#include "pablio.h"
#include <string.h>

/*
** Note that many of the older ISA sound cards on PCs do NOT support
** full duplex audio (simultaneous record and playback).
** And some only support full duplex at lower sample rates.
*/
#define SAMPLE_RATE         (22050)
#define NUM_SECONDS            (20)
#define SAMPLES_PER_FRAME       (2)

/* Select whether we will use floats or shorts. */
#if 1
#define SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#else
#define SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#endif

#define NUM_ECHO_FRAMES   (2*SAMPLE_RATE)
SAMPLE   samples[NUM_ECHO_FRAMES][SAMPLES_PER_FRAME] = {0.0};

/*******************************************************************/
int main(void);
int main(void)
{
    int      i;
    PaError  err;
    PABLIO_Stream     *aInStream;
    PABLIO_Stream     *aOutStream;
    int      index;

    printf("Full duplex sound test using PABLIO\n");
    fflush(stdout);

    /* Open simplified blocking I/O layer on top of PortAudio. */
    /* Open input first so it can start to fill buffers. */
    err = OpenAudioStream( &aInStream, SAMPLE_RATE, SAMPLE_TYPE,
                           (PABLIO_READ | PABLIO_STEREO) );
    if( err != paNoError ) goto error;
    /* printf("opened input\n");  fflush(stdout); /**/

    err = OpenAudioStream( &aOutStream, SAMPLE_RATE, SAMPLE_TYPE,
                           (PABLIO_WRITE | PABLIO_STEREO) );
    if( err != paNoError ) goto error;
    /* printf("opened output\n");  fflush(stdout); /**/

    /* Process samples in the foreground. */
    index = 0;
    for( i=0; i<(NUM_SECONDS * SAMPLE_RATE); i++ )
    {
        /* Write old frame of data to output. */
        /* samples[index][1] = (i&256) * (1.0f/256.0f); /* sawtooth */
        WriteAudioStream( aOutStream, &samples[index][0], 1 );

        /* Read one frame of data into sample array for later output. */
        ReadAudioStream( aInStream, &samples[index][0], 1 );
        index += 1;
        if( index >= NUM_ECHO_FRAMES ) index = 0;

        if( (i & 0xFFFF) == 0 ) printf("i = %d\n", i ); fflush(stdout); /**/
    }

    CloseAudioStream( aOutStream );
    CloseAudioStream( aInStream );

    printf("R/W echo sound test complete.\n" );
    fflush(stdout);
    return 0;

error:
    fprintf( stderr, "An error occured while using PortAudio\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return -1;
}
