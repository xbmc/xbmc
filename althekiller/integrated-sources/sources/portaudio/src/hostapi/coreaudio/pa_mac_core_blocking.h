/*
 * Internal blocking interfaces for PortAudio Apple AUHAL implementation
 *
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 *
 * Written by Bjorn Roche of XO Audio LLC, from PA skeleton code.
 * Portions copied from code by Dominic Mazzoni (who wrote a HAL implementation)
 *
 * Dominic's code was based on code by Phil Burk, Darren Gibbs,
 * Gord Peters, Stephane Letz, and Greg Pfiel.
 *
 * The following people also deserve acknowledgements:
 *
 * Olivier Tristan for feedback and testing
 * Glenn Zelniker and Z-Systems engineering for sponsoring the Blocking I/O
 * interface.
 * 
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Ross Bencina, Phil Burk
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

/**
 @file
 @ingroup hostaip_src
*/

#ifndef PA_MAC_CORE_BLOCKING_H_
#define PA_MAC_CORE_BLOCKING_H_

#include "pa_ringbuffer.h"
#include "portaudio.h"
#include "pa_mac_core_utilities.h"

/*
 * Number of miliseconds to busy wait whil waiting for data in blocking calls.
 */
#define PA_MAC_BLIO_BUSY_WAIT_SLEEP_INTERVAL (5)
/*
 * Define exactly one of these blocking methods
 * PA_MAC_BLIO_MUTEX is not actively maintained.
 */
#define PA_MAC_BLIO_BUSY_WAIT
/*
#define PA_MAC_BLIO_MUTEX
*/

typedef struct {
    PaUtilRingBuffer inputRingBuffer;
    PaUtilRingBuffer outputRingBuffer;
    size_t ringBufferFrames;
    PaSampleFormat inputSampleFormat;
    size_t inputSampleSizeActual;
    size_t inputSampleSizePow2;
    PaSampleFormat outputSampleFormat;
    size_t outputSampleSizeActual;
    size_t outputSampleSizePow2;

    size_t framesPerBuffer;

    int inChan;
    int outChan;

    //PaStreamCallbackFlags statusFlags;
    uint32_t statusFlags;
    PaError errors;

    /* Here we handle blocking, using condition variables. */
#ifdef  PA_MAC_BLIO_MUTEX
    volatile bool isInputEmpty;
    pthread_mutex_t inputMutex;
    pthread_cond_t inputCond;

    volatile bool isOutputFull;
    pthread_mutex_t outputMutex;
    pthread_cond_t outputCond;
#endif
}
PaMacBlio;

/*
 * These functions operate on condition and related variables.
 */

PaError initializeBlioRingBuffers(
                                       PaMacBlio *blio,
                                       PaSampleFormat inputSampleFormat,
                                       PaSampleFormat outputSampleFormat,
                                       size_t framesPerBuffer,
                                       long ringBufferSize,
                                       int inChan,
                                       int outChan );
PaError destroyBlioRingBuffers( PaMacBlio *blio );
PaError resetBlioRingBuffers( PaMacBlio *blio );

int BlioCallback(
        const void *input, void *output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData );

void waitUntilBlioWriteBufferIsFlushed( PaMacBlio *blio );

#endif /*PA_MAC_CORE_BLOCKING_H_*/
