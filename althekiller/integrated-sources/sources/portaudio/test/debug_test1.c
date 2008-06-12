/*
 * $Id: debug_test1.c 178 2002-06-04 23:07:01Z  $
 patest1.c
 Ring modulate the audio input with a 441hz sine wave for 20 seconds
    using the Portable Audio api
    Author: Ross Bencina <rossb@audiomulch.com>
    Modifications:
 April 5th, 2001 - PLB - Check for NULL inputBuffer.
*/
#include <stdio.h>
#include <math.h>
#include "portaudio.h"
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
typedef struct
{
    float sine[100];
    int phase;
    int sampsToGo;
}
patest1data;
static int patest1Callback( void *inputBuffer, void *outputBuffer,
                            unsigned long bufferFrames,
                            PaTimestamp outTime, void *userData )
{
    patest1data *data = (patest1data*)userData;
    float *in = (float*)inputBuffer;
    float *out = (float*)outputBuffer;
    int framesToCalc = bufferFrames;
    unsigned long i;
    int finished = 0;
    if(inputBuffer == NULL) return 0;
    if( data->sampsToGo < bufferFrames )
    {
        finished = 1;
    }
    for( i=0; i<bufferFrames; i++ )
    {
        *out++ = *in++;
        *out++ = *in++;
        if( data->phase >= 100 )
            data->phase = 0;
    }
    data->sampsToGo -= bufferFrames;
    /* zero remainder of final buffer if not already done */
    for( ; i<bufferFrames; i++ )
    {
        *out++ = 0; /* left */
        *out++ = 0; /* right */
    }
    return finished;
}
int main(int argc, char* argv[]);
int main(int argc, char* argv[])
{
    PaStream *stream;
    PaError err;
    patest1data data;
    int i;
    int inputDevice = Pa_GetDefaultInputDeviceID();
    int outputDevice = Pa_GetDefaultOutputDeviceID();
    /* initialise sinusoidal wavetable */
    for( i=0; i<100; i++ )
        data.sine[i] = sin( ((double)i/100.) * M_PI * 2. );
    data.phase = 0;
    data.sampsToGo = 44100 * 4;   // 20 seconds
    /* initialise portaudio subsytem */
    Pa_Initialize();
    err = Pa_OpenStream(
              &stream,
              inputDevice,
              2,              /* stereo input */
              paFloat32,  /* 32 bit floating point input */
              NULL,
              outputDevice,
              2,              /* stereo output */
              paFloat32,      /* 32 bit floating point output */
              NULL,
              44100.,
              //    22050,          /* half second buffers */
              //    4,              /* four buffers */
              512,          /* half second buffers */
              0,              /* four buffers */
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patest1Callback,
              &data );
    if( err == paNoError )
    {
        err = Pa_StartStream( stream );
        //       printf( "Press any key to end.\n" );
        //       getc( stdin ); //wait for input before exiting
        //       Pa_AbortStream( stream );

        printf( "Waiting for stream to complete...\n" );

        while( Pa_StreamActive( stream ) )
            Pa_Sleep(1000); /* sleep until playback has finished */

        err = Pa_CloseStream( stream );
    }
    else
    {
        fprintf( stderr, "An error occured while opening the portaudio stream\n" );
        if( err == paHostError )
            fprintf( stderr, "Host error number: %d\n", Pa_GetHostError() );
        else
            fprintf( stderr, "Error number: %d\n", err );
    }
    Pa_Terminate();
    printf( "bye\n" );

    return 0;
}
