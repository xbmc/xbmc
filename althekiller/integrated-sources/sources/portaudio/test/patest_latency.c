/** @file
	@ingroup test_src
	@brief Hear the latency caused by big buffers.
	Play a sine wave and change frequency based on letter input.
	@author Phil Burk <philburk@softsynth.com>, and Darren Gibbs
*/
/*
 * $Id: patest_latency.c 1097 2006-08-26 08:27:53Z rossb $
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

#define OUTPUT_DEVICE       (Pa_GetDefaultOutputDevice())
#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (64)

#define MIN_FREQ            (100.0f)
#define CalcPhaseIncrement(freq)  ((freq)/SAMPLE_RATE)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TABLE_SIZE   (400)

typedef struct
{
    float sine[TABLE_SIZE + 1]; /* add one for guard point for interpolation */
    float phase_increment;
    float left_phase;
    float right_phase;
}
paTestData;

float LookupSine( paTestData *data, float phase );
/* Convert phase between and 1.0 to sine value
 * using linear interpolation.
 */
float LookupSine( paTestData *data, float phase )
{
    float fIndex = phase*TABLE_SIZE;
    int   index = (int) fIndex;
    float fract = fIndex - index;
    float lo = data->sine[index];
    float hi = data->sine[index+1];
    float val = lo + fract*(hi-lo);
    return val;
}

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
    int i;

    (void) inputBuffer; /* Prevent unused variable warning. */

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = LookupSine(data, data->left_phase);  /* left */
        *out++ = LookupSine(data, data->right_phase);  /* right */
        data->left_phase += data->phase_increment;
        if( data->left_phase >= 1.0f ) data->left_phase -= 1.0f;
        data->right_phase += (data->phase_increment * 1.5f); /* fifth above */
        if( data->right_phase >= 1.0f ) data->right_phase -= 1.0f;
    }
    return 0;
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStream *stream;
    PaStreamParameters outputParameters;
    PaError err;
    paTestData data;
    int i;
    int done = 0;

    printf("PortAudio Test: enter letter then hit ENTER.\n" );
    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = 0.90f * (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.sine[TABLE_SIZE] = data.sine[0]; /* set guard point. */
    data.left_phase = data.right_phase = 0.0;
    data.phase_increment = CalcPhaseIncrement(MIN_FREQ);

    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    printf("PortAudio Test: output device = %d\n", OUTPUT_DEVICE );

    outputParameters.device = OUTPUT_DEVICE;
    outputParameters.channelCount = 2;         /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
    printf("Requested output latency = %.4f seconds.\n", outputParameters.suggestedLatency );
    printf("%d frames per buffer.\n.", FRAMES_PER_BUFFER );

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff|paDitherOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    printf("Play ASCII keyboard. Hit 'q' to stop. (Use RETURN key on Mac)\n");
    fflush(stdout);
    while ( !done )
    {
        float  freq;
        int index;
        char c;
        do
        {
            c = getchar();
        }
        while( c < ' '); /* Strip white space and control chars. */

        if( c == 'q' ) done = 1;
        index = c % 26;
        freq = MIN_FREQ + (index * 40.0);
        data.phase_increment = CalcPhaseIncrement(freq);
    }
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
