#ifndef PA_ASIO_H
#define PA_ASIO_H
/*
 * $Id: pa_asio.h 1083 2006-08-23 07:30:49Z rossb $
 * PortAudio Portable Real-Time Audio Library
 * ASIO specific extensions
 *
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


/** @file
 @brief ASIO-specific PortAudio API extension header file.
*/


#include "portaudio.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** Retrieve legal latency settings for the specificed device, in samples.

 @param device The global index of the device about which the query is being made.
 @param minLatency A pointer to the location which will recieve the minimum latency value.
 @param maxLatency A pointer to the location which will recieve the maximum latency value.
 @param preferredLatency A pointer to the location which will recieve the preferred latency value.
 @param granularity A pointer to the location which will recieve the granularity. This value 
 determines which values between minLatency and maxLatency are available. ie the step size,
 if granularity is -1 then available latency settings are powers of two.

 @see ASIOGetBufferSize in the ASIO SDK.

 @todo This function should have a better name, any suggestions?
*/
PaError PaAsio_GetAvailableLatencyValues( PaDeviceIndex device,
		long *minLatency, long *maxLatency, long *preferredLatency, long *granularity );

        
/** Display the ASIO control panel for the specified device.

  @param device The global index of the device whose control panel is to be displayed.
  @param systemSpecific On Windows, the calling application's main window handle,
  on Macintosh this value should be zero.
*/
PaError PaAsio_ShowControlPanel( PaDeviceIndex device, void* systemSpecific );




/** Retrieve a pointer to a string containing the name of the specified
 input channel. The string is valid until Pa_Terminate is called.

 The string will be no longer than 32 characters including the null terminator.
*/
PaError PaAsio_GetInputChannelName( PaDeviceIndex device, int channelIndex,
        const char** channelName );

        
/** Retrieve a pointer to a string containing the name of the specified
 input channel. The string is valid until Pa_Terminate is called.

 The string will be no longer than 32 characters including the null terminator.
*/
PaError PaAsio_GetOutputChannelName( PaDeviceIndex device, int channelIndex,
        const char** channelName );


#define paAsioUseChannelSelectors      (0x01)

typedef struct PaAsioStreamInfo{
    unsigned long size;             /**< sizeof(PaAsioStreamInfo) */
    PaHostApiTypeId hostApiType;    /**< paASIO */
    unsigned long version;          /**< 1 */

    unsigned long flags;

    /* Support for opening only specific channels of an ASIO device.
        If the paAsioUseChannelSelectors flag is set, channelSelectors is a
        pointer to an array of integers specifying the device channels to use.
        When used, the length of the channelSelectors array must match the
        corresponding channelCount parameter to Pa_OpenStream() otherwise a
        crash may result.
        The values in the selectors array must specify channels within the
        range of supported channels for the device or paInvalidChannelCount will
        result.
    */
    int *channelSelectors;
}PaAsioStreamInfo;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_ASIO_H */
