/*
 * $Id: debug_multi_in.c 1083 2006-08-23 07:30:49Z rossb $
 * debug_multi_in.c
 * Pass output from each of multiple channels
 * to a stereo output using the Portable Audio api.
 * Hacked test for debugging PA.
 *
 * Author: Phil Burk  http://www.softsynth.com
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
#include <string.h>
#include "portaudio.h"
//#define INPUT_DEVICE_NAME   ("EWS88 MT Interleaved Rec")
#define OUTPUT_DEVICE       (Pa_GetDefaultOutputDeviceID())
//#define OUTPUT_DEVICE       (18)
#define SAMPLE_RATE         (22050)
#define FRAMES_PER_BUFFER   (256)
#define MIN_LATENCY_MSEC    (400)
#define NUM_BUFFERS         ((MIN_LATENCY_MSEC * SAMPLE_RATE) / (FRAMES_PER_BUFFER * 1000))
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
typedef struct
{
    int      liveChannel;
    int      numChannels;
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
    float *out = (float*)outputBuffer;
    float *in = (float*)inputBuffer;
    int i;
    int finished = 0;
    (void) outTime; /* Prevent unused variable warnings. */
    (void) inputBuffer;

    if( in == NULL ) return 0;
    for( i=0; i<(int)framesPerBuffer; i++ )
    {
        /* Copy one channel of input to output. */
        *out++ = in[data->liveChannel];
        *out++ = in[data->liveChannel];
        in += data->numChannels;
    }
    return 0;
}
/*******************************************************************/
int PaFindDeviceByName( const char *name )
{
    int   i;
    int   numDevices;
    const PaDeviceInfo *pdi;
    int   len = strlen( name );
    PaDeviceID   result = paNoDevice;
    numDevices = Pa_CountDevices();
    for( i=0; i<numDevices; i++ )
    {
        pdi = Pa_GetDeviceInfo( i );
        if( strncmp( name, pdi->name, len ) == 0 )
        {
            result = i;
            break;
        }
    }
    return result;
}
/*******************************************************************/
int main(void);
int main(void)
{
    PortAudioStream *stream;
    PaError err;
    paTestData data;
    int i;
    PaDeviceID inputDevice;
    const PaDeviceInfo *pdi;
    printf("PortAudio Test: input signal from each channel. %d buffers\n", NUM_BUFFERS );
    data.liveChannel = 0;
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
#ifdef INPUT_DEVICE_NAME
    printf("Try to use device: %s\n", INPUT_DEVICE_NAME );
    inputDevice = PaFindDeviceByName(INPUT_DEVICE_NAME);
    if( inputDevice == paNoDevice )
    {
        printf("Could not find %s. Using default instead.\n", INPUT_DEVICE_NAME );
        inputDevice = Pa_GetDefaultInputDeviceID();
    }
#else
    printf("Using default input device.\n");
    inputDevice = Pa_GetDefaultInputDeviceID();
#endif
    pdi = Pa_GetDeviceInfo( inputDevice );
    if( pdi == NULL )
    {
        printf("Could not get device info!\n");
        goto error;
    }
    data.numChannels = pdi->maxInputChannels;
    printf("Input Device name is %s\n", pdi->name );
    printf("Input Device has %d channels.\n", pdi->maxInputChannels);
    err = Pa_OpenStream(
              &stream,
              inputDevice,
              pdi->maxInputChannels,
              paFloat32,  /* 32 bit floating point input */
              NULL,
              OUTPUT_DEVICE,
              2,
              paFloat32,  /* 32 bit floating point output */
              NULL,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,  /* frames per buffer */
              NUM_BUFFERS,    /* number of buffers, if zero then use default minimum */
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) goto error;
    data.liveChannel = 0;
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    for( i=0; i<data.numChannels; i++ )
    {
        data.liveChannel = i;
        printf("Channel %d being sent to output. Hit ENTER for next channel.", i );
        fflush(stdout);
        getchar();
    }
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
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
