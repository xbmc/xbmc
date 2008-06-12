/*
 * $Id: test_rw.c 1083 2006-08-23 07:30:49Z rossb $
 * test_rw.c
 * Read input from one stream and write it to another.
 *
 * Author: Phil Burk, http://www.softsynth.com/portaudio/
 *
 * This program uses PABLIO, the Portable Audio Blocking I/O Library.
 * PABLIO is built on top of PortAudio, the Portable Audio Library.
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
#include "pablio.h"

/*
** Note that many of the older ISA sound cards on PCs do NOT support
** full duplex audio (simultaneous record and playback).
** And some only support full duplex at lower sample rates.
*/
#define SAMPLE_RATE          (44100)
#define NUM_SECONDS              (5)
#define SAMPLES_PER_FRAME        (2)
#define FRAMES_PER_BLOCK        (64)

/* Select whether we will use floats or shorts. */
#if 1
#define SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#else
#define SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#endif

/*******************************************************************/
int main(void);
int main(void)
{
    int      i;
    SAMPLE   samples[SAMPLES_PER_FRAME * FRAMES_PER_BLOCK];
    PaError  err;
    PABLIO_Stream     *aStream;

    printf("Full duplex sound test using PortAudio and RingBuffers\n");
    fflush(stdout);

    /* Open simplified blocking I/O layer on top of PortAudio. */
    err = OpenAudioStream( &aStream, SAMPLE_RATE, SAMPLE_TYPE,
                           (PABLIO_READ_WRITE | PABLIO_STEREO) );
    if( err != paNoError ) goto error;

    /* Process samples in the foreground. */
    for( i=0; i<(NUM_SECONDS * SAMPLE_RATE); i += FRAMES_PER_BLOCK )
    {
        /* Read one block of data into sample array from audio input. */
        ReadAudioStream( aStream, samples, FRAMES_PER_BLOCK );
        /* Write that same block of data to output. */
        WriteAudioStream( aStream, samples, FRAMES_PER_BLOCK );
    }

    CloseAudioStream( aStream );

    printf("Full duplex sound test complete.\n" );
    fflush(stdout);
    return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return -1;
}
