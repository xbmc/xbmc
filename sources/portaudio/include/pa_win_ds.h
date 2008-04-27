#ifndef PA_WIN_DS_H
#define PA_WIN_DS_H
/*
 * $Id:  $
 * PortAudio Portable Real-Time Audio Library
 * DirectSound specific extensions
 *
 * Copyright (c) 1999-2007 Ross Bencina and Phil Burk
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
 @brief DirectSound-specific PortAudio API extension header file.
*/


#include "portaudio.h"
#include "pa_win_waveformat.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#define paWinDirectSoundUseLowLevelLatencyParameters            (0x01)
#define paWinDirectSoundUseChannelMask                          (0x04)


typedef struct PaWinDirectSoundStreamInfo{
    unsigned long size;             /**< sizeof(PaWinDirectSoundStreamInfo) */
    PaHostApiTypeId hostApiType;    /**< paDirectSound */
    unsigned long version;          /**< 1 */

    unsigned long flags;

    /* low-level latency setting support
        TODO ** NOT IMPLEMENTED **
        These settings control the number and size of host buffers in order
        to set latency. They will be used instead of the generic parameters
        to Pa_OpenStream() if flags contains the paWinDirectSoundUseLowLevelLatencyParameters
        flag.

        If PaWinDirectSoundStreamInfo structures with paWinDirectSoundUseLowLevelLatencyParameters
        are supplied for both input and output in a full duplex stream, then the
        input and output framesPerBuffer must be the same, or the larger of the
        two must be a multiple of the smaller, otherwise a
        paIncompatibleHostApiSpecificStreamInfo error will be returned from
        Pa_OpenStream().

    unsigned long framesPerBuffer;
    */

    /*
        support for WAVEFORMATEXTENSIBLE channel masks. If flags contains
        paWinDirectSoundUseChannelMask this allows you to specify which speakers 
        to address in a multichannel stream. Constants for channelMask
        are specified in pa_win_waveformat.h

    */
    PaWinWaveFormatChannelMask channelMask;

}PaWinDirectSoundStreamInfo;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_WIN_DS_H */                                  
