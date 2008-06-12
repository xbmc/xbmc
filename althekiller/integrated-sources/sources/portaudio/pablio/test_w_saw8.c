/*
 * $Id: test_w_saw8.c 1083 2006-08-23 07:30:49Z rossb $
 * test_w_saw8.c
 * Generate stereo 8 bit sawtooth waveforms.
 *
 * Author: Phil Burk, http://www.softsynth.com
 *
 * This program uses PABLIO, the Portable Audio Blocking I/O Library.
 * PABLIO is built on top of PortAudio, the Portable Audio Library.
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

#define SAMPLE_RATE         (22050)
#define NUM_SECONDS             (6)
#define SAMPLES_PER_FRAME       (2)


#define FRAMES_PER_BLOCK    (100)

unsigned char   samples[FRAMES_PER_BLOCK][SAMPLES_PER_FRAME];
unsigned char   phases[SAMPLES_PER_FRAME];

/*******************************************************************/
int main(void);
int main(void)
{
    int             i,j;
    PaError         err;
    PABLIO_Stream  *aOutStream;

    printf("Generate unsigned 8 bit sawtooth waves using PABLIO.\n");
    fflush(stdout);

    /* Open simplified blocking I/O layer on top of PortAudio. */
    err = OpenAudioStream( &aOutStream, SAMPLE_RATE, paUInt8,
                           (PABLIO_WRITE | PABLIO_STEREO) );
    if( err != paNoError ) goto error;

    /* Initialize oscillator phases to "ground" level for paUInt8. */
    phases[0] = 128;
    phases[1] = 128;

    for( i=0; i<(NUM_SECONDS * SAMPLE_RATE); i += FRAMES_PER_BLOCK )
    {
        /* Generate sawtooth waveforms in a block for efficiency. */
        for( j=0; j<FRAMES_PER_BLOCK; j++ )
        {
            /* Generate a sawtooth wave by incrementing a variable. */
            phases[0] += 1;
            /* We don't have to do anything special to wrap when using paUint8 because
             * 8 bit arithmetic automatically wraps. */
            samples[j][0] = phases[0];

            /* On the second channel, generate a higher sawtooth wave. */
            phases[1] += 3;
            samples[j][1] = phases[1];
        }

        /* Write samples to output. */
        WriteAudioStream( aOutStream, samples, FRAMES_PER_BLOCK );
    }

    CloseAudioStream( aOutStream );

    printf("Sawtooth sound test complete.\n" );
    fflush(stdout);
    return 0;

error:
    fprintf( stderr, "An error occured while using PABLIO\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return -1;
}
