/** @file paqa_devs.c
	@ingroup test_src
    @brief Self Testing Quality Assurance app for PortAudio
 	Try to open each device and run through all the
 	possible configurations.

	@author Phil Burk  http://www.softsynth.com
    
    Pieter adapted to V19 API. Test now relies heavily on 
    Pa_IsFormatSupported(). Uses same 'standard' sample rates
    as in test pa_devs.c.
*/
/*
 * $Id: paqa_devs.c 1097 2006-08-26 08:27:53Z rossb $
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
#include "pa_trace.h"

/****************************************** Definitions ***********/
#define MODE_INPUT      (0)
#define MODE_OUTPUT     (1)

typedef struct PaQaData
{
    unsigned long  framesLeft;
    int            numChannels;
    int            bytesPerSample;
    int            mode;
    short          sawPhase;
    PaSampleFormat format;
}
PaQaData;

/****************************************** Prototypes ***********/
static void TestDevices( int mode );
static void TestFormats( int mode, PaDeviceIndex deviceID, double sampleRate,
                         int numChannels );
static int TestAdvance( int mode, PaDeviceIndex deviceID, double sampleRate,
                        int numChannels, PaSampleFormat format );
static int QaCallback( const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData );

/****************************************** Globals ***********/
static int gNumPassed = 0;
static int gNumFailed = 0;

/****************************************** Macros ***********/
/* Print ERROR if it fails. Tally success or failure. */
/* Odd do-while wrapper seems to be needed for some compilers. */
#define EXPECT(_exp) \
    do \
    { \
        if ((_exp)) {\
            /* printf("SUCCESS for %s\n", #_exp ); */ \
            gNumPassed++; \
        } \
        else { \
            printf("ERROR - 0x%x - %s for %s\n", result, \
                   ((result == 0) ? "-" : Pa_GetErrorText(result)), \
                   #_exp ); \
            gNumFailed++; \
            goto error; \
        } \
    } while(0)

/*******************************************************************/
/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int QaCallback( const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData )
{
    unsigned long  i;
    short          phase;
    PaQaData *data = (PaQaData *) userData;
    (void) inputBuffer;

    /* Play simle sawtooth wave. */
    if( data->mode == MODE_OUTPUT )
    {
        phase = data->sawPhase;
        switch( data->format )
        {
        case paFloat32:
            {
                float *out =  (float *) outputBuffer;
                for( i=0; i<framesPerBuffer; i++ )
                {
                    phase += 0x123;
                    *out++ = (float) (phase * (1.0 / 32768.0));
                    if( data->numChannels == 2 )
                    {
                        *out++ = (float) (phase * (1.0 / 32768.0));
                    }
                }
            }
            break;

        case paInt32:
            {
                int *out =  (int *) outputBuffer;
                for( i=0; i<framesPerBuffer; i++ )
                {
                    phase += 0x123;
                    *out++ = ((int) phase ) << 16;
                    if( data->numChannels == 2 )
                    {
                        *out++ = ((int) phase ) << 16;
                    }
                }
            }
            break;
        case paInt16:
            {
                short *out =  (short *) outputBuffer;
                for( i=0; i<framesPerBuffer; i++ )
                {
                    phase += 0x123;
                    *out++ = phase;
                    if( data->numChannels == 2 )
                    {
                        *out++ = phase;
                    }
                }
            }
            break;

        default:
            {
                unsigned char *out =  (unsigned char *) outputBuffer;
                unsigned long numBytes = framesPerBuffer * data->numChannels * data->bytesPerSample;
                for( i=0; i<numBytes; i++ )
                {
                    *out++ = 0;
                }
            }
            break;
        }
        data->sawPhase = phase;
    }
    /* Are we through yet? */
    if( data->framesLeft > framesPerBuffer )
    {
        PaUtil_AddTraceMessage("QaCallback: running. framesLeft", data->framesLeft );
        data->framesLeft -= framesPerBuffer;
        return 0;
    }
    else
    {
        PaUtil_AddTraceMessage("QaCallback: DONE! framesLeft", data->framesLeft );
        data->framesLeft = 0;
        return 1;
    }
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaError result;
    EXPECT( ((result=Pa_Initialize()) == 0) );
    printf("Test OUTPUT ---------------\n");
    TestDevices( MODE_OUTPUT );
    printf("Test INPUT ---------------\n");
    TestDevices( MODE_INPUT );
error:
    Pa_Terminate();
    printf("QA Report: %d passed, %d failed.\n", gNumPassed, gNumFailed );
    return 0;
}

/*******************************************************************
* Try each output device, through its full range of capabilities. */
static void TestDevices( int mode )
{
    int id, jc, i;
    int maxChannels;
    const PaDeviceInfo *pdi;
    static double standardSampleRates[] = {  8000.0,  9600.0, 11025.0, 12000.0,
                                            16000.0,          22050.0, 24000.0,
                                            32000.0,          44100.0, 48000.0,
                                                              88200.0, 96000.0,
                                               -1.0 }; /* Negative terminated list. */
    int numDevices = Pa_GetDeviceCount();
    for( id=0; id<numDevices; id++ )            /* Iterate through all devices. */
    {
        pdi = Pa_GetDeviceInfo( id );
        /* Try 1 to maxChannels on each device. */
        maxChannels = (( mode == MODE_INPUT ) ? pdi->maxInputChannels : pdi->maxOutputChannels);
        
        for( jc=1; jc<=maxChannels; jc++ )
        {
            printf("Name         = %s\n", pdi->name );
            /* Try each standard sample rate. */
            for( i=0; standardSampleRates[i] > 0; i++ )
            {
                TestFormats( mode, (PaDeviceIndex)id, standardSampleRates[i], jc );
            }
        }
    }
}

/*******************************************************************/
static void TestFormats( int mode, PaDeviceIndex deviceID, double sampleRate,
                         int numChannels )
{
    TestAdvance( mode, deviceID, sampleRate, numChannels, paFloat32 );
    TestAdvance( mode, deviceID, sampleRate, numChannels, paInt16 );
    TestAdvance( mode, deviceID, sampleRate, numChannels, paInt32 );
    /* TestAdvance( mode, deviceID, sampleRate, numChannels, paInt24 ); */
}

/*******************************************************************/
static int TestAdvance( int mode, PaDeviceIndex deviceID, double sampleRate,
                        int numChannels, PaSampleFormat format )
{
    PaStreamParameters inputParameters, outputParameters, *ipp, *opp;
    PaStream *stream = NULL;
    PaError result = paNoError;
    PaQaData myData;
    #define FRAMES_PER_BUFFER  (64)
    
    /* Setup data for synthesis thread. */
    myData.framesLeft = (unsigned long) (sampleRate * 100); /* 100 seconds */
    myData.numChannels = numChannels;
    myData.mode = mode;
    myData.format = format;
    switch( format )
    {
    case paFloat32:
    case paInt32:
    case paInt24:
        myData.bytesPerSample = 4;
        break;
/*  case paPackedInt24:
        myData.bytesPerSample = 3;
        break; */
    default:
        myData.bytesPerSample = 2;
        break;
    }

    if( mode == MODE_INPUT )
    {
        inputParameters.device       = deviceID;
        inputParameters.channelCount = numChannels;
        inputParameters.sampleFormat = format;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;
        ipp = &inputParameters;
    }
    else
        ipp = NULL;
    if( mode == MODE_OUTPUT )           /* Pa_GetDeviceInfo(paNoDevice) COREDUMPS!!! */
    {
        outputParameters.device       = deviceID;
        outputParameters.channelCount = numChannels;
        outputParameters.sampleFormat = format;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;
        opp = &outputParameters;
    }
    else
        opp = NULL;

    if(paFormatIsSupported == Pa_IsFormatSupported( ipp, opp, sampleRate ))
    {
        printf("------ TestAdvance: %s, device = %d, rate = %g, numChannels = %d, format = %lu -------\n",
                ( mode == MODE_INPUT ) ? "INPUT" : "OUTPUT",
                deviceID, sampleRate, numChannels, (unsigned long)format);
        EXPECT( ((result = Pa_OpenStream( &stream,
                                          ipp,
                                          opp,
                                          sampleRate,
                                          FRAMES_PER_BUFFER,
                                          paClipOff,  /* we won't output out of range samples so don't bother clipping them */
                                          QaCallback,
                                          &myData ) ) == 0) );
        if( stream )
        {
            PaTime oldStamp, newStamp;
            unsigned long oldFrames;
            int minDelay = ( mode == MODE_INPUT ) ? 1000 : 400;
            /* Was:
            int minNumBuffers = Pa_GetMinNumBuffers( FRAMES_PER_BUFFER, sampleRate );
            int msec = (int) ((minNumBuffers * 3 * 1000.0 * FRAMES_PER_BUFFER) / sampleRate);
            */
            int msec = (int)( 3.0 *
                       (( mode == MODE_INPUT ) ? inputParameters.suggestedLatency : outputParameters.suggestedLatency ));
            if( msec < minDelay ) msec = minDelay;
            printf("msec = %d\n", msec);  /**/
            EXPECT( ((result=Pa_StartStream( stream )) == 0) );
            /* Check to make sure PortAudio is advancing timeStamp. */
            oldStamp = Pa_GetStreamTime(stream);
            Pa_Sleep(msec);
            newStamp = Pa_GetStreamTime(stream);
            printf("oldStamp = %g,newStamp = %g\n", oldStamp, newStamp ); /**/
            EXPECT( (oldStamp < newStamp) );
            /* Check to make sure callback is decrementing framesLeft. */
            oldFrames = myData.framesLeft;
            Pa_Sleep(msec);
            printf("oldFrames = %lu, myData.framesLeft = %lu\n", oldFrames,  myData.framesLeft ); /**/
            EXPECT( (oldFrames > myData.framesLeft) );
            EXPECT( ((result=Pa_CloseStream( stream )) == 0) );
            stream = NULL;
        }
    }
error:
    if( stream != NULL ) Pa_CloseStream( stream );
    return result;
}
