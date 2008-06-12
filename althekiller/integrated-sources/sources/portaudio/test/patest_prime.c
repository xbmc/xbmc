/** @file patest_prime.c
	@ingroup test_src
	@brief Test stream priming mode.
	@author Ross Bencina http://www.audiomulch.com/~rossb
*/

/*
 * $Id: patest_prime.c 1097 2006-08-26 08:27:53Z rossb $
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
#include "pa_util.h"

#define NUM_BEEPS           (3)
#define SAMPLE_RATE         (44100)
#define SAMPLE_PERIOD       (1.0/44100.0)
#define FRAMES_PER_BUFFER   (256)
#define BEEP_DURATION       (400)
#define IDLE_DURATION       (SAMPLE_RATE*2)      /* 2 seconds */
#define SLEEP_MSEC          (50)

#define STATE_BKG_IDLE      (0)
#define STATE_BKG_BEEPING   (1)

typedef struct
{
    float        leftPhase;
    float        rightPhase;
    int          state;
    int          beepCountdown;
    int          idleCountdown;
}
paTestData;

static void InitializeTestData( paTestData *testData )
{
    testData->leftPhase = 0;
    testData->rightPhase = 0;
    testData->state = STATE_BKG_BEEPING;
    testData->beepCountdown = BEEP_DURATION;
    testData->idleCountdown = IDLE_DURATION;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
			               const PaStreamCallbackTimeInfo *timeInfo,
			               PaStreamCallbackFlags statusFlags, void *userData )
{
    /* Cast data passed through stream to our structure. */
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned int i;
    int result = paContinue;

    /* supress unused parameter warnings */
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;

    for( i=0; i<framesPerBuffer; i++ )
    {
        switch( data->state )
        {
        case STATE_BKG_IDLE:
            *out++ = 0.0;  /* left */
            *out++ = 0.0;  /* right */
            --data->idleCountdown;
            
            if( data->idleCountdown <= 0 ) result = paComplete;
            break;

        case STATE_BKG_BEEPING:
            if( data->beepCountdown <= 0 )
            {
                data->state = STATE_BKG_IDLE;
                *out++ = 0.0;  /* left */
                *out++ = 0.0;  /* right */
            }
            else
            {
                /* Play sawtooth wave. */
                *out++ = data->leftPhase;  /* left */
                *out++ = data->rightPhase;  /* right */
                /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
                data->leftPhase += 0.01f;
                /* When signal reaches top, drop back down. */
                if( data->leftPhase >= 1.0f ) data->leftPhase -= 2.0f;
                /* higher pitch so we can distinguish left and right. */
                data->rightPhase += 0.03f;
                if( data->rightPhase >= 1.0f ) data->rightPhase -= 2.0f;
            }
            --data->beepCountdown;
            break;
        }
    }
    
    return result;
}

/*******************************************************************/
static PaError DoTest( int flags )
{
    PaStream *stream;
    PaError    err;
    paTestData data;
    PaStreamParameters outputParameters;

    InitializeTestData( &data );       

    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = 2;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;

    /* Open an audio I/O stream. */
    err = Pa_OpenStream(
        &stream,
        NULL,                         /* no input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,            /* frames per buffer */
        paClipOff | flags,      /* we won't output out of range samples so don't bother clipping them */
        patestCallback,
        &data );
    if( err != paNoError ) goto error;


    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    printf("hear \"BEEP\"\n" );
    fflush(stdout);

    while( ( err = Pa_IsStreamActive( stream ) ) == 1 ) Pa_Sleep(SLEEP_MSEC);
    if( err < 0 ) goto error;

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    return err;
error:
    return err;
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaError    err = paNoError;
    int        i;

    /* Initialize library before making any other calls. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    
    printf("PortAudio Test: Testing stream playback with no priming.\n");
    printf("PortAudio Test: you should see BEEP before you hear it.\n");
    printf("BEEP %d times.\n", NUM_BEEPS );

    for( i=0; i< NUM_BEEPS; ++i )
    {
        err = DoTest( 0 );
        if( err != paNoError )
            goto error;
    }

    printf("PortAudio Test: Testing stream playback with priming.\n");
    printf("PortAudio Test: you should see BEEP around the same time you hear it.\n");
    for( i=0; i< NUM_BEEPS; ++i )
    {
        err = DoTest( paPrimeOutputBuffersUsingStreamCallback );
        if( err != paNoError )
            goto error;
    }

    printf("Test finished.\n");

    Pa_Terminate();
    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
