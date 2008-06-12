/** @file patest_sync.c
	@ingroup test_src
	@brief Test time stamping and synchronization of audio and video.

	A high latency is used so we can hear the difference in time.
	Random durations are used so we know we are hearing the right beep
	and not the one before or after.

	Sequence of events:
		-# Foreground requests a beep.
		-# Background randomly schedules a beep.
		-# Foreground waits for the beep to be heard based on PaUtil_GetTime().
		-# Foreground outputs video (printf) in sync with audio.
		-# Repeat.
	
	@author Phil Burk  http://www.softsynth.com
*/
/*
 * $Id: patest_sync.c 1097 2006-08-26 08:27:53Z rossb $
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
#define NUM_BEEPS           (6)
#define SAMPLE_RATE         (44100)
#define SAMPLE_PERIOD       (1.0/44100.0)
#define FRAMES_PER_BUFFER   (256)
#define BEEP_DURATION       (400)
#define LATENCY_MSEC        (2000)
#define SLEEP_MSEC          (10)
#define TIMEOUT_MSEC        (15000)

#define STATE_BKG_IDLE      (0)
#define STATE_BKG_PENDING   (1)
#define STATE_BKG_BEEPING   (2)
typedef struct
{
    float        left_phase;
    float        right_phase;
    int          state;
    volatile int requestBeep;  /* Set by foreground, cleared by background. */
    PaTime       beepTime;
    int          beepCount;
    double       latency;    /* For debugging. */
}
paTestData;

static unsigned long GenerateRandomNumber( void );
/************************************************************/
/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static unsigned long GenerateRandomNumber( void )
{
    static unsigned long randSeed = 99887766;  /* Change this for different random sequences. */
    randSeed = (randSeed * 196314165) + 907633515;
    return randSeed;
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
    (void) inputBuffer;

    data->latency = timeInfo->outputBufferDacTime - timeInfo->currentTime;

    for( i=0; i<framesPerBuffer; i++ )
    {
        switch( data->state )
        {
        case STATE_BKG_IDLE:
            /* Schedule beep at some random time in the future. */
            if( data->requestBeep )
            {
                int random = GenerateRandomNumber() >> 14;
                data->beepTime = timeInfo->outputBufferDacTime + (( (double)(random + SAMPLE_RATE)) * SAMPLE_PERIOD );
                data->state = STATE_BKG_PENDING;
            }
            *out++ = 0.0;  /* left */
            *out++ = 0.0;  /* right */
            break;

        case STATE_BKG_PENDING:
            if( (timeInfo->outputBufferDacTime + (i*SAMPLE_PERIOD)) >= data->beepTime )
            {
                data->state = STATE_BKG_BEEPING;
                data->beepCount = BEEP_DURATION;
                data->left_phase = data->right_phase = 0.0;
            }
            *out++ = 0.0;  /* left */
            *out++ = 0.0;  /* right */
            break;

        case STATE_BKG_BEEPING:
            if( data->beepCount <= 0 )
            {
                data->state = STATE_BKG_IDLE;
                data->requestBeep = 0;
                *out++ = 0.0;  /* left */
                *out++ = 0.0;  /* right */
            }
            else
            {
                /* Play sawtooth wave. */
                *out++ = data->left_phase;  /* left */
                *out++ = data->right_phase;  /* right */
                /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
                data->left_phase += 0.01f;
                /* When signal reaches top, drop back down. */
                if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
                /* higher pitch so we can distinguish left and right. */
                data->right_phase += 0.03f;
                if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
            }
            data->beepCount -= 1;
            break;

        default:
            data->state = STATE_BKG_IDLE;
            break;
        }
    }
    return 0;
}
/*******************************************************************/
int main(void);
int main(void)
{
    PaStream *stream;
    PaError    err;
    paTestData DATA;
    int        i, timeout;
    PaTime     previousTime;
    PaStreamParameters outputParameters;
    printf("PortAudio Test: you should see BEEP at the same time you hear it.\n");
    printf("Wait for a few seconds random delay between BEEPs.\n");
    printf("BEEP %d times.\n", NUM_BEEPS );
    /* Initialize our DATA for use by callback. */
    DATA.left_phase = DATA.right_phase = 0.0;
    DATA.state = STATE_BKG_IDLE;
    DATA.requestBeep = 0;
    /* Initialize library before making any other calls. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;
    
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = 2;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = (double)LATENCY_MSEC / 1000;

    /* Open an audio I/O stream. */
     err = Pa_OpenStream(
              &stream,
	      NULL,                         /* no input */
	      &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,            /* frames per buffer */
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &DATA );
   if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    printf("started\n");
    fflush(stdout);

    previousTime = Pa_GetStreamTime( stream );
    for( i=0; i<NUM_BEEPS; i++ )
    {
        /* Request a beep from background. */
        DATA.requestBeep = 1;

        /* Wait for background to acknowledge request. */
        timeout = TIMEOUT_MSEC;
        while( (DATA.requestBeep == 1) && (timeout-- > 0 ) ) Pa_Sleep(SLEEP_MSEC);
        if( timeout <= 0 )
        {
            fprintf( stderr, "Timed out waiting for background to acknowledge request.\n" );
            goto error;
        }
        printf("calc beep for %9.3f, latency = %6.3f\n", DATA.beepTime, DATA.latency );
        fflush(stdout);

        /* Wait for scheduled beep time. */
        timeout =  TIMEOUT_MSEC + (10000/SLEEP_MSEC);
        while( (Pa_GetStreamTime( stream ) < DATA.beepTime) && (timeout-- > 0 ) )
        {
            Pa_Sleep(SLEEP_MSEC);
        }
        if( timeout <= 0 )
        {
            fprintf( stderr, "Timed out waiting for time. Now = %9.3f, Beep for %9.3f.\n",
                     PaUtil_GetTime(), DATA.beepTime );
            goto error;
        }

        /* Beep should be sounding now so print synchronized BEEP. */
        printf("hear \"BEEP\" at %9.3f, delta = %9.3f\n",
               Pa_GetStreamTime( stream ), (DATA.beepTime - previousTime) );
        fflush(stdout);

        previousTime = DATA.beepTime;
    }

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
