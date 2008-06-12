/** @file patest1.c
	@ingroup test_src
	@brief Ring modulate the audio input with a sine wave for 20 seconds.
	@author Ross Bencina <rossb@audiomulch.com>
*/
/*
 * $Id: patest1.c 1097 2006-08-26 08:27:53Z rossb $
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

#ifndef M_PI
#define M_PI (3.14159265)
#endif

#define SAMPLE_RATE (44100)

typedef struct
{
    float sine[100];
    int phase;
    int sampsToGo;
}
patest1data;

static int patest1Callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    patest1data *data = (patest1data*)userData;
    float *in = (float*)inputBuffer;
    float *out = (float*)outputBuffer;
    int framesToCalc = framesPerBuffer;
    unsigned long i = 0;
    int finished;

    if( data->sampsToGo < framesPerBuffer )
    {
        framesToCalc = data->sampsToGo;
        finished = paComplete;
    }
    else
    {
        finished = paContinue;
    }

    for( ; i<framesToCalc; i++ )
    {
        *out++ = *in++ * data->sine[data->phase];  /* left */
        *out++ = *in++ * data->sine[data->phase++];  /* right */
        if( data->phase >= 100 )
            data->phase = 0;
    }

    data->sampsToGo -= framesToCalc;

    /* zero remainder of final buffer if not already done */
    for( ; i<framesPerBuffer; i++ )
    {
        *out++ = 0; /* left */
        *out++ = 0; /* right */
    }
    
    return finished;
}

int main(int argc, char* argv[]);
int main(int argc, char* argv[])
{
    PaStream                *stream;
    PaError                 err;
    patest1data             data;
    int                     i;
    PaStreamParameters      inputParameters, outputParameters;
    const PaHostErrorInfo*  herr;

    printf("patest1.c\n"); fflush(stdout);
    printf("Ring modulate input for 20 seconds.\n"); fflush(stdout);
    
    /* initialise sinusoidal wavetable */
    for( i=0; i<100; i++ )
        data.sine[i] = sin( ((double)i/100.) * M_PI * 2. );
    data.phase = 0;
    data.sampsToGo = SAMPLE_RATE * 20;        /* 20 seconds. */

    /* initialise portaudio subsytem */
    err = Pa_Initialize();

    inputParameters.device = Pa_GetDefaultInputDevice();    /* default input device */
    inputParameters.channelCount = 2;                       /* stereo input */
    inputParameters.sampleFormat = paFloat32;               /* 32 bit floating point input */
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = Pa_GetDefaultOutputDevice();  /* default output device */
    outputParameters.channelCount = 2;                      /* stereo output */
    outputParameters.sampleFormat = paFloat32;              /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
                        &stream,
                        &inputParameters,
                        &outputParameters,
                        (double)SAMPLE_RATE, /* Samplerate in Hertz. */
                        512,                 /* Small buffers */
                        paClipOff,           /* We won't output out of range samples so don't bother clipping them. */
                        patest1Callback,
                        &data );
    if( err != paNoError ) goto done;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto done;
    
    printf( "Press any key to end.\n" ); fflush(stdout);
         
    getc( stdin ); /* wait for input before exiting */

    err = Pa_AbortStream( stream );
    if( err != paNoError ) goto done;
    
    printf( "Waiting for stream to complete...\n" );

    /* sleep until playback has finished */
    while( ( err = Pa_IsStreamActive( stream ) ) == 1 ) Pa_Sleep(1000);
    if( err < 0 ) goto done;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto done;

done:
    Pa_Terminate();

    if( err != paNoError )
    {
        fprintf( stderr, "An error occured while using portaudio\n" );
        if( err == paUnanticipatedHostError )
        {
            fprintf( stderr, " unanticipated host error.\n");
            herr = Pa_GetLastHostErrorInfo();
            if (herr)
            {
                fprintf( stderr, " Error number: %ld\n", herr->errorCode );
                if (herr->errorText)
                    fprintf( stderr, " Error text: %s\n", herr->errorText );
            }
            else
                fprintf( stderr, " Pa_GetLastHostErrorInfo() failed!\n" );
        }
        else
        {
            fprintf( stderr, " Error number: %d\n", err );
            fprintf( stderr, " Error text: %s\n", Pa_GetErrorText( err ) );
        }

        err = 1;          /* Always return 0 or 1, but no other return codes. */
    }

    printf( "bye\n" );

    return err;
}
