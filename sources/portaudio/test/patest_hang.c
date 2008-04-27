/** @file patest_hang.c
	@ingroup test_src
	@brief Play a sine then hang audio callback to test watchdog.
	@author Ross Bencina <rossb@audiomulch.com>
	@author Phil Burk <philburk@softsynth.com>
*/
/*
 * $Id: patest_hang.c 1097 2006-08-26 08:27:53Z rossb $
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

#define SAMPLE_RATE       (44100)
#define FRAMES_PER_BUFFER (1024)
#ifndef M_PI
#define M_PI              (3.14159265)
#endif
#define TWOPI             (M_PI * 2.0)

typedef struct paTestData
{
    int    sleepFor;
    double phase;
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
    float *out = (float*)outputBuffer;
    unsigned long i;
    int finished = 0;
    double phaseInc = 0.02;
    double phase = data->phase;
    
    (void) inputBuffer; /* Prevent unused argument warning. */

    for( i=0; i<framesPerBuffer; i++ )
    {
        phase += phaseInc;
        if( phase > TWOPI ) phase -= TWOPI;
        /* This is not a very efficient way to calc sines. */
        *out++ = (float) sin( phase ); /* mono */
    }
    
    if( data->sleepFor > 0 )
    {
        Pa_Sleep( data->sleepFor );
    }
    
    data->phase = phase;
    return finished;
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStream*           stream;
    PaStreamParameters  outputParameters;
    PaError             err;
    int                 i;
    paTestData          data = {0};
    
    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
        SAMPLE_RATE, FRAMES_PER_BUFFER );

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* Default output device. */
    outputParameters.channelCount = 1;                     /* Mono output. */
    outputParameters.sampleFormat = paFloat32;             /* 32 bit floating point. */
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.suggestedLatency          = Pa_GetDeviceInfo(outputParameters.device)
                                                 ->defaultLowOutputLatency;
    err = Pa_OpenStream(&stream,
                        NULL,                    /* No input. */
                        &outputParameters,
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,               /* No out of range samples. */
                        patestCallback,
                        &data);
    if (err != paNoError) goto error;
    
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    /* Gradually increase sleep time. */
    /* Was: for( i=0; i<10000; i+= 1000 ) */
    for(i=0; i <= 1000; i += 100)
    {
        printf("Sleep for %d milliseconds in audio callback.\n", i );
        data.sleepFor = i;
        Pa_Sleep( ((i<1000) ? 1000 : i) );
    }
    
    printf("Suffer for 10 seconds.\n");
    Pa_Sleep( 10000 );
    
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
