#ifndef PA_HOSTAPI_H
#define PA_HOSTAPI_H
/*
 * $Id: pa_hostapi.h 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library
 * host api representation
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

/** @file
 @ingroup common_src

 @brief Interface used by pa_front to virtualize functions which operate on
 host APIs.
*/


#include "portaudio.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** **FOR THE USE OF pa_front.c ONLY**
    Do NOT use fields in this structure, they my change at any time.
    Use functions defined in pa_util.h if you think you need functionality
    which can be derived from here.
*/
typedef struct PaUtilPrivatePaFrontHostApiInfo {


    unsigned long baseDeviceIndex;
}PaUtilPrivatePaFrontHostApiInfo;


/** The common header for all data structures whose pointers are passed through
 the hostApiSpecificStreamInfo field of the PaStreamParameters structure.
 Note that in order to keep the public PortAudio interface clean, this structure
 is not used explicitly when declaring hostApiSpecificStreamInfo data structures.
 However, some code in pa_front depends on the first 3 members being equivalent
 with this structure.
 @see PaStreamParameters
*/
typedef struct PaUtilHostApiSpecificStreamInfoHeader
{
    unsigned long size;             /**< size of whole structure including this header */
    PaHostApiTypeId hostApiType;    /**< host API for which this data is intended */
    unsigned long version;          /**< structure version */
} PaUtilHostApiSpecificStreamInfoHeader;



/** A structure representing the interface to a host API. Contains both
 concrete data and pointers to functions which implement the interface.
*/
typedef struct PaUtilHostApiRepresentation {
    PaUtilPrivatePaFrontHostApiInfo privatePaFrontInfo;

    /** The host api implementation should populate the info field. In the
        case of info.defaultInputDevice and info.defaultOutputDevice the
        values stored should be 0 based indices within the host api's own
        device index range (0 to deviceCount). These values will be converted
        to global device indices by pa_front after PaUtilHostApiInitializer()
        returns.
    */
    PaHostApiInfo info;

    PaDeviceInfo** deviceInfos;

    /**
        (*Terminate)() is guaranteed to be called with a valid <hostApi>
        parameter, which was previously returned from the same implementation's
        initializer.
    */
    void (*Terminate)( struct PaUtilHostApiRepresentation *hostApi );

    /**
        The inputParameters and outputParameters pointers should not be saved
        as they will not remain valid after OpenStream is called.

        
        The following guarantees are made about parameters to (*OpenStream)():

            [NOTE: the following list up to *END PA FRONT VALIDATIONS* should be
                kept in sync with the one for ValidateOpenStreamParameters and
                Pa_OpenStream in pa_front.c]
                
            PaHostApiRepresentation *hostApi
                - is valid for this implementation

            PaStream** stream
                - is non-null

            - at least one of inputParameters & outputParmeters is valid (not NULL)

            - if inputParameters & outputParmeters are both valid, that
                inputParameters->device & outputParmeters->device  both use the same host api
 
            PaDeviceIndex inputParameters->device
                - is within range (0 to Pa_CountDevices-1) Or:
                - is paUseHostApiSpecificDeviceSpecification and
                    inputParameters->hostApiSpecificStreamInfo is non-NULL and refers
                    to a valid host api

            int inputParameters->numChannels
                - if inputParameters->device is not paUseHostApiSpecificDeviceSpecification, numInputChannels is > 0
                - upper bound is NOT validated against device capabilities
 
            PaSampleFormat inputParameters->sampleFormat
                - is one of the sample formats defined in portaudio.h

            void *inputParameters->hostApiSpecificStreamInfo
                - if supplied its hostApi field matches the input device's host Api
 
            PaDeviceIndex outputParmeters->device
                - is within range (0 to Pa_CountDevices-1)
 
            int outputParmeters->numChannels
                - if inputDevice is valid, numInputChannels is > 0
                - upper bound is NOT validated against device capabilities
 
            PaSampleFormat outputParmeters->sampleFormat
                - is one of the sample formats defined in portaudio.h
        
            void *outputParmeters->hostApiSpecificStreamInfo
                - if supplied its hostApi field matches the output device's host Api
 
            double sampleRate
                - is not an 'absurd' rate (less than 1000. or greater than 200000.)
                - sampleRate is NOT validated against device capabilities
 
            PaStreamFlags streamFlags
                - unused platform neutral flags are zero
                - paNeverDropInput is only used for full-duplex callback streams
                    with variable buffer size (paFramesPerBufferUnspecified)

            [*END PA FRONT VALIDATIONS*]


        The following validations MUST be performed by (*OpenStream)():

            - check that input device can support numInputChannels
            
            - check that input device can support inputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format

            - if inputStreamInfo is supplied, validate its contents,
                or return an error if no inputStreamInfo is expected

            - check that output device can support numOutputChannels
            
            - check that output device can support outputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format

            - if outputStreamInfo is supplied, validate its contents,
                or return an error if no outputStreamInfo is expected

            - if a full duplex stream is requested, check that the combination
                of input and output parameters is supported

            - check that the device supports sampleRate

            - alter sampleRate to a close allowable rate if necessary

            - validate inputLatency and outputLatency

            - validate any platform specific flags, if flags are supplied they
                must be valid.
    */
    PaError (*OpenStream)( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** stream,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerCallback,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData );


    PaError (*IsFormatSupported)( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate );
} PaUtilHostApiRepresentation;


/** Prototype for the initialization function which must be implemented by every
 host API.
 
 @see paHostApiInitializers
*/
typedef PaError PaUtilHostApiInitializer( PaUtilHostApiRepresentation**, PaHostApiIndex );


/** paHostApiInitializers is a NULL-terminated array of host API initialization
 functions. These functions are called by pa_front to initialize the host APIs
 when the client calls Pa_Initialize().

 There is a platform specific file which defines paHostApiInitializers for that
 platform, pa_win/pa_win_hostapis.c contains the Win32 definitions for example.
*/
extern PaUtilHostApiInitializer *paHostApiInitializers[];


/** The index of the default host API in the paHostApiInitializers array.
 
 There is a platform specific file which defines paDefaultHostApiIndex for that
 platform, see pa_win/pa_win_hostapis.c for example.
*/
extern int paDefaultHostApiIndex;


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_HOSTAPI_H */
