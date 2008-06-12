/** @file patest_ringmix.c
	@ingroup test_src
	@brief Ring modulate inputs to left output, mix inputs to right output.
*/
/*
 * $Id: patest_ringmix.c 1097 2006-08-26 08:27:53Z rossb $ 
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


#include "stdio.h"
#include "portaudio.h"
/* This will be called asynchronously by the PortAudio engine. */
static int myCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    const float *in  = (const float *) inputBuffer;
	float *out = (float *) outputBuffer;    
    float leftInput, rightInput;
    unsigned int i;

    /* Read input buffer, process data, and fill output buffer. */
    for( i=0; i<framesPerBuffer; i++ )
    {
        leftInput = *in++;      /* Get interleaved samples from input buffer. */
        rightInput = *in++;
        *out++ = leftInput * rightInput;            /* ring modulation */
        *out++ = 0.5f * (leftInput + rightInput);   /* mix */
    }
    return 0;
}

/* Open a PortAudioStream to input and output audio data. */
int main(void)
{
    PaStream *stream;
    Pa_Initialize();
    Pa_OpenDefaultStream(
        &stream,
        2, 2,               /* stereo input and output */
        paFloat32,  44100.0,
        64,                 /* 64 frames per buffer */
        myCallback, NULL );
    Pa_StartStream( stream );
    Pa_Sleep( 10000 );    /* Sleep for 10 seconds while processing. */
    Pa_StopStream( stream );
    Pa_CloseStream( stream );
    Pa_Terminate();
    return 0;
}
