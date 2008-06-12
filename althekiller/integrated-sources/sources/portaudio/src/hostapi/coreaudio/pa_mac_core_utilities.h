/*
 * Helper and utility functions for pa_mac_core.c (Apple AUHAL implementation)
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
 @ingroup hostapi_src
*/

#ifndef PA_MAC_CORE_UTILITIES_H__
#define PA_MAC_CORE_UTILITIES_H__

#include <pthread.h>
#include "portaudio.h"
#include "pa_util.h"
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

#ifndef MIN
#define MIN(a, b)  (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a, b)  (((a)<(b))?(b):(a))
#endif

#define ERR(mac_error) PaMacCore_SetError(mac_error, __LINE__, 1 ) 
#define WARNING(mac_error) PaMacCore_SetError(mac_error, __LINE__, 0 )


/* Help keep track of AUHAL element numbers */
#define INPUT_ELEMENT  (1)
#define OUTPUT_ELEMENT (0)

/* Normal level of debugging: fine for most apps that don't mind the occational warning being printf'ed */
/*
 */
#define MAC_CORE_DEBUG
#ifdef MAC_CORE_DEBUG
# define DBUG(MSG) do { printf("||PaMacCore (AUHAL)|| "); printf MSG ; fflush(stdout); } while(0)
#else
# define DBUG(MSG)
#endif

/* Verbose Debugging: useful for developement */
/*
#define MAC_CORE_VERBOSE_DEBUG
*/
#ifdef MAC_CORE_VERBOSE_DEBUG
# define VDBUG(MSG) do { printf("||PaMacCore (v )|| "); printf MSG ; fflush(stdout); } while(0)
#else
# define VDBUG(MSG)
#endif

/* Very Verbose Debugging: Traces every call. */
/*
#define MAC_CORE_VERY_VERBOSE_DEBUG
 */
#ifdef MAC_CORE_VERY_VERBOSE_DEBUG
# define VVDBUG(MSG) do { printf("||PaMacCore (vv)|| "); printf MSG ; fflush(stdout); } while(0)
#else
# define VVDBUG(MSG)
#endif





#define UNIX_ERR(err) PaMacCore_SetUnixError( err, __LINE__ )

PaError PaMacCore_SetUnixError( int err, int line );

/*
 * Translates MacOS generated errors into PaErrors
 */
PaError PaMacCore_SetError(OSStatus error, int line, int isError);

/*
 * This function computes an appropriate ring buffer size given
 * a requested latency (in seconds), sample rate and framesPerBuffer.
 *
 * The returned ringBufferSize is computed using the following
 * constraints:
 *   - it must be at least 4.
 *   - it must be at least 3x framesPerBuffer.
 *   - it must be at least 2x the suggestedLatency.
 *   - it must be a power of 2.
 * This function attempts to compute the minimum such size.
 *
 */
long computeRingBufferSize( const PaStreamParameters *inputParameters,
                                   const PaStreamParameters *outputParameters,
                                   long inputFramesPerBuffer,
                                   long outputFramesPerBuffer,
                                   double sampleRate );

/*
 * Durring testing of core audio, I found that serious crashes could occur
 * if properties such as sample rate were changed multiple times in rapid
 * succession. The function below has some fancy logic to make sure that changes
 * are acknowledged before another is requested. That seems to help a lot.
 */

typedef struct {
   bool once; /* I didn't end up using this. bdr */
   pthread_mutex_t mutex;
} MutexAndBool ;

OSStatus propertyProc(
    AudioDeviceID inDevice, 
    UInt32 inChannel, 
    Boolean isInput, 
    AudioDevicePropertyID inPropertyID, 
    void* inClientData );

/* sets the value of the given property and waits for the change to 
   be acknowledged, and returns the final value, which is not guaranteed
   by this function to be the same as the desired value. Obviously, this
   function can only be used for data whose input and output are the
   same size and format, and their size and format are known in advance.*/
PaError AudioDeviceSetPropertyNowAndWaitForChange(
    AudioDeviceID inDevice,
    UInt32 inChannel, 
    Boolean isInput, 
    AudioDevicePropertyID inPropertyID,
    UInt32 inPropertyDataSize, 
    const void *inPropertyData,
    void *outPropertyData );

/*
 * Sets the sample rate the HAL device.
 * if requireExact: set the sample rate or fail.
 *
 * otherwise      : set the exact sample rate.
 *             If that fails, check for available sample rates, and choose one
 *             higher than the requested rate. If there isn't a higher one,
 *             just use the highest available.
 */
PaError setBestSampleRateForDevice( const AudioDeviceID device,
                                    const bool isOutput,
                                    const bool requireExact,
                                    const Float64 desiredSrate );
/*
   Attempts to set the requestedFramesPerBuffer. If it can't set the exact
   value, it settles for something smaller if available. If nothing smaller
   is available, it uses the smallest available size.
   actualFramesPerBuffer will be set to the actual value on successful return.
   OK to pass NULL to actualFramesPerBuffer.
   The logic is very simmilar too setBestSampleRate only failure here is
   not usually catastrophic.
*/
PaError setBestFramesPerBuffer( const AudioDeviceID device,
                                const bool isOutput,
                                UInt32 requestedFramesPerBuffer, 
                                UInt32 *actualFramesPerBuffer );
#endif /* PA_MAC_CORE_UTILITIES_H__*/
