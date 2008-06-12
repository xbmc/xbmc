#ifndef PA_WIN_WMME_H
#define PA_WIN_WMME_H
/*
 * $Id: pa_win_wmme.h 1247 2007-08-11 16:29:09Z rossb $
 * PortAudio Portable Real-Time Audio Library
 * MME specific extensions
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
 @brief WMME-specific PortAudio API extension header file.
*/


#include "portaudio.h"
#include "pa_win_waveformat.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#define paWinMmeUseLowLevelLatencyParameters            (0x01)
#define paWinMmeUseMultipleDevices                      (0x02)  /* use mme specific multiple device feature */
#define paWinMmeUseChannelMask                          (0x04)

/* By default, the mme implementation drops the processing thread's priority
    to THREAD_PRIORITY_NORMAL and sleeps the thread if the CPU load exceeds 100%
    This flag disables any priority throttling. The processing thread will always
    run at THREAD_PRIORITY_TIME_CRITICAL.
*/
#define paWinMmeDontThrottleOverloadedProcessingThread  (0x08)


typedef struct PaWinMmeDeviceAndChannelCount{
    PaDeviceIndex device;
    int channelCount;
}PaWinMmeDeviceAndChannelCount;


typedef struct PaWinMmeStreamInfo{
    unsigned long size;             /**< sizeof(PaWinMmeStreamInfo) */
    PaHostApiTypeId hostApiType;    /**< paMME */
    unsigned long version;          /**< 1 */

    unsigned long flags;

    /* low-level latency setting support
        These settings control the number and size of host buffers in order
        to set latency. They will be used instead of the generic parameters
        to Pa_OpenStream() if flags contains the PaWinMmeUseLowLevelLatencyParameters
        flag.

        If PaWinMmeStreamInfo structures with PaWinMmeUseLowLevelLatencyParameters
        are supplied for both input and output in a full duplex stream, then the
        input and output framesPerBuffer must be the same, or the larger of the
        two must be a multiple of the smaller, otherwise a
        paIncompatibleHostApiSpecificStreamInfo error will be returned from
        Pa_OpenStream().
    */
    unsigned long framesPerBuffer;
    unsigned long bufferCount;  /* formerly numBuffers */ 

    /* multiple devices per direction support
        If flags contains the PaWinMmeUseMultipleDevices flag,
        this functionality will be used, otherwise the device parameter to
        Pa_OpenStream() will be used instead.
        If devices are specified here, the corresponding device parameter
        to Pa_OpenStream() should be set to paUseHostApiSpecificDeviceSpecification,
        otherwise an paInvalidDevice error will result.
        The total number of channels accross all specified devices
        must agree with the corresponding channelCount parameter to
        Pa_OpenStream() otherwise a paInvalidChannelCount error will result.
    */
    PaWinMmeDeviceAndChannelCount *devices;
    unsigned long deviceCount;

    /*
        support for WAVEFORMATEXTENSIBLE channel masks. If flags contains
        paWinMmeUseChannelMask this allows you to specify which speakers 
        to address in a multichannel stream. Constants for channelMask
        are specified in pa_win_waveformat.h

    */
    PaWinWaveFormatChannelMask channelMask;

}PaWinMmeStreamInfo;


/** Retrieve the number of wave in handles used by a PortAudio WinMME stream.
 Returns zero if the stream is output only.

 @return A non-negative value indicating the number of wave in handles
 or, a PaErrorCode (which are always negative) if PortAudio is not initialized
 or an error is encountered.

 @see PaWinMME_GetStreamInputHandle
*/
int PaWinMME_GetStreamInputHandleCount( PaStream* stream );


/** Retrieve a wave in handle used by a PortAudio WinMME stream.

 @param stream The stream to query.
 @param handleIndex The zero based index of the wave in handle to retrieve. This
    should be in the range [0, PaWinMME_GetStreamInputHandleCount(stream)-1].

 @return A valid wave in handle, or NULL if an error occurred.

 @see PaWinMME_GetStreamInputHandle
*/
HWAVEIN PaWinMME_GetStreamInputHandle( PaStream* stream, int handleIndex );


/** Retrieve the number of wave out handles used by a PortAudio WinMME stream.
 Returns zero if the stream is input only.
 
 @return A non-negative value indicating the number of wave out handles
 or, a PaErrorCode (which are always negative) if PortAudio is not initialized
 or an error is encountered.

 @see PaWinMME_GetStreamOutputHandle
*/
int PaWinMME_GetStreamOutputHandleCount( PaStream* stream );


/** Retrieve a wave out handle used by a PortAudio WinMME stream.

 @param stream The stream to query.
 @param handleIndex The zero based index of the wave out handle to retrieve.
    This should be in the range [0, PaWinMME_GetStreamOutputHandleCount(stream)-1].

 @return A valid wave out handle, or NULL if an error occurred.

 @see PaWinMME_GetStreamOutputHandleCount
*/
HWAVEOUT PaWinMME_GetStreamOutputHandle( PaStream* stream, int handleIndex );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_WIN_WMME_H */                                  
