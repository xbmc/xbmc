#ifndef PORTAUDIO_H
#define PORTAUDIO_H
/*
 * $Id: portaudio.h 1247 2007-08-11 16:29:09Z rossb $
 * PortAudio Portable Real-Time Audio Library
 * PortAudio API Header File
 * Latest version available at: http://www.portaudio.com/
 *
 * Copyright (c) 1999-2002 Ross Bencina and Phil Burk
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
 @brief The PortAudio API.
*/


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

 
/** Retrieve the release number of the currently running PortAudio build,
 eg 1900.
*/
int Pa_GetVersion( void );


/** Retrieve a textual description of the current PortAudio build,
 eg "PortAudio V19-devel 13 October 2002".
*/
const char* Pa_GetVersionText( void );


/** Error codes returned by PortAudio functions.
 Note that with the exception of paNoError, all PaErrorCodes are negative.
*/

typedef int PaError;
typedef enum PaErrorCode
{
    paNoError = 0,

    paNotInitialized = -10000,
    paUnanticipatedHostError,
    paInvalidChannelCount,
    paInvalidSampleRate,
    paInvalidDevice,
    paInvalidFlag,
    paSampleFormatNotSupported,
    paBadIODeviceCombination,
    paInsufficientMemory,
    paBufferTooBig,
    paBufferTooSmall,
    paNullCallback,
    paBadStreamPtr,
    paTimedOut,
    paInternalError,
    paDeviceUnavailable,
    paIncompatibleHostApiSpecificStreamInfo,
    paStreamIsStopped,
    paStreamIsNotStopped,
    paInputOverflowed,
    paOutputUnderflowed,
    paHostApiNotFound,
    paInvalidHostApi,
    paCanNotReadFromACallbackStream,      /**< @todo review error code name */
    paCanNotWriteToACallbackStream,       /**< @todo review error code name */
    paCanNotReadFromAnOutputOnlyStream,   /**< @todo review error code name */
    paCanNotWriteToAnInputOnlyStream,     /**< @todo review error code name */
    paIncompatibleStreamHostApi,
    paBadBufferPtr
} PaErrorCode;


/** Translate the supplied PortAudio error code into a human readable
 message.
*/
const char *Pa_GetErrorText( PaError errorCode );


/** Library initialization function - call this before using PortAudio.
 This function initialises internal data structures and prepares underlying
 host APIs for use.  With the exception of Pa_GetVersion(), Pa_GetVersionText(),
 and Pa_GetErrorText(), this function MUST be called before using any other
 PortAudio API functions.

 If Pa_Initialize() is called multiple times, each successful 
 call must be matched with a corresponding call to Pa_Terminate(). 
 Pairs of calls to Pa_Initialize()/Pa_Terminate() may overlap, and are not 
 required to be fully nested.

 Note that if Pa_Initialize() returns an error code, Pa_Terminate() should
 NOT be called.

 @return paNoError if successful, otherwise an error code indicating the cause
 of failure.

 @see Pa_Terminate
*/
PaError Pa_Initialize( void );


/** Library termination function - call this when finished using PortAudio.
 This function deallocates all resources allocated by PortAudio since it was
 initializied by a call to Pa_Initialize(). In cases where Pa_Initialise() has
 been called multiple times, each call must be matched with a corresponding call
 to Pa_Terminate(). The final matching call to Pa_Terminate() will automatically
 close any PortAudio streams that are still open.

 Pa_Terminate() MUST be called before exiting a program which uses PortAudio.
 Failure to do so may result in serious resource leaks, such as audio devices
 not being available until the next reboot.

 @return paNoError if successful, otherwise an error code indicating the cause
 of failure.
 
 @see Pa_Initialize
*/
PaError Pa_Terminate( void );



/** The type used to refer to audio devices. Values of this type usually
 range from 0 to (Pa_GetDeviceCount()-1), and may also take on the PaNoDevice
 and paUseHostApiSpecificDeviceSpecification values.

 @see Pa_GetDeviceCount, paNoDevice, paUseHostApiSpecificDeviceSpecification
*/
typedef int PaDeviceIndex;


/** A special PaDeviceIndex value indicating that no device is available,
 or should be used.

 @see PaDeviceIndex
*/
#define paNoDevice ((PaDeviceIndex)-1)


/** A special PaDeviceIndex value indicating that the device(s) to be used
 are specified in the host api specific stream info structure.

 @see PaDeviceIndex
*/
#define paUseHostApiSpecificDeviceSpecification ((PaDeviceIndex)-2)


/* Host API enumeration mechanism */

/** The type used to enumerate to host APIs at runtime. Values of this type
 range from 0 to (Pa_GetHostApiCount()-1).

 @see Pa_GetHostApiCount
*/
typedef int PaHostApiIndex;


/** Retrieve the number of available host APIs. Even if a host API is
 available it may have no devices available.

 @return A non-negative value indicating the number of available host APIs
 or, a PaErrorCode (which are always negative) if PortAudio is not initialized
 or an error is encountered.

 @see PaHostApiIndex
*/
PaHostApiIndex Pa_GetHostApiCount( void );


/** Retrieve the index of the default host API. The default host API will be
 the lowest common denominator host API on the current platform and is
 unlikely to provide the best performance.

 @return A non-negative value ranging from 0 to (Pa_GetHostApiCount()-1)
 indicating the default host API index or, a PaErrorCode (which are always
 negative) if PortAudio is not initialized or an error is encountered.
*/
PaHostApiIndex Pa_GetDefaultHostApi( void );


/** Unchanging unique identifiers for each supported host API. This type
 is used in the PaHostApiInfo structure. The values are guaranteed to be
 unique and to never change, thus allowing code to be written that
 conditionally uses host API specific extensions.

 New type ids will be allocated when support for a host API reaches
 "public alpha" status, prior to that developers should use the
 paInDevelopment type id.

 @see PaHostApiInfo
*/
typedef enum PaHostApiTypeId
{
    paInDevelopment=0, /* use while developing support for a new host API */
    paDirectSound=1,
    paMME=2,
    paASIO=3,
    paSoundManager=4,
    paCoreAudio=5,
    paOSS=7,
    paALSA=8,
    paAL=9,
    paBeOS=10,
    paWDMKS=11,
    paJACK=12,
    paWASAPI=13,
    paAudioScienceHPI=14
} PaHostApiTypeId;


/** A structure containing information about a particular host API. */

typedef struct PaHostApiInfo
{
    /** this is struct version 1 */
    int structVersion;
    /** The well known unique identifier of this host API @see PaHostApiTypeId */
    PaHostApiTypeId type;
    /** A textual description of the host API for display on user interfaces. */
    const char *name;

    /**  The number of devices belonging to this host API. This field may be
     used in conjunction with Pa_HostApiDeviceIndexToDeviceIndex() to enumerate
     all devices for this host API.
     @see Pa_HostApiDeviceIndexToDeviceIndex
    */
    int deviceCount;

    /** The default input device for this host API. The value will be a
     device index ranging from 0 to (Pa_GetDeviceCount()-1), or paNoDevice
     if no default input device is available.
    */
    PaDeviceIndex defaultInputDevice;

    /** The default output device for this host API. The value will be a
     device index ranging from 0 to (Pa_GetDeviceCount()-1), or paNoDevice
     if no default output device is available.
    */
    PaDeviceIndex defaultOutputDevice;
    
} PaHostApiInfo;


/** Retrieve a pointer to a structure containing information about a specific
 host Api.

 @param hostApi A valid host API index ranging from 0 to (Pa_GetHostApiCount()-1)

 @return A pointer to an immutable PaHostApiInfo structure describing
 a specific host API. If the hostApi parameter is out of range or an error
 is encountered, the function returns NULL.

 The returned structure is owned by the PortAudio implementation and must not
 be manipulated or freed. The pointer is only guaranteed to be valid between
 calls to Pa_Initialize() and Pa_Terminate().
*/
const PaHostApiInfo * Pa_GetHostApiInfo( PaHostApiIndex hostApi );


/** Convert a static host API unique identifier, into a runtime
 host API index.

 @param type A unique host API identifier belonging to the PaHostApiTypeId
 enumeration.

 @return A valid PaHostApiIndex ranging from 0 to (Pa_GetHostApiCount()-1) or,
 a PaErrorCode (which are always negative) if PortAudio is not initialized
 or an error is encountered.
 
 The paHostApiNotFound error code indicates that the host API specified by the
 type parameter is not available.

 @see PaHostApiTypeId
*/
PaHostApiIndex Pa_HostApiTypeIdToHostApiIndex( PaHostApiTypeId type );


/** Convert a host-API-specific device index to standard PortAudio device index.
 This function may be used in conjunction with the deviceCount field of
 PaHostApiInfo to enumerate all devices for the specified host API.

 @param hostApi A valid host API index ranging from 0 to (Pa_GetHostApiCount()-1)

 @param hostApiDeviceIndex A valid per-host device index in the range
 0 to (Pa_GetHostApiInfo(hostApi)->deviceCount-1)

 @return A non-negative PaDeviceIndex ranging from 0 to (Pa_GetDeviceCount()-1)
 or, a PaErrorCode (which are always negative) if PortAudio is not initialized
 or an error is encountered.

 A paInvalidHostApi error code indicates that the host API index specified by
 the hostApi parameter is out of range.

 A paInvalidDevice error code indicates that the hostApiDeviceIndex parameter
 is out of range.
 
 @see PaHostApiInfo
*/
PaDeviceIndex Pa_HostApiDeviceIndexToDeviceIndex( PaHostApiIndex hostApi,
        int hostApiDeviceIndex );



/** Structure used to return information about a host error condition.
*/
typedef struct PaHostErrorInfo{
    PaHostApiTypeId hostApiType;    /**< the host API which returned the error code */
    long errorCode;                 /**< the error code returned */
    const char *errorText;          /**< a textual description of the error if available, otherwise a zero-length string */
}PaHostErrorInfo;


/** Return information about the last host error encountered. The error
 information returned by Pa_GetLastHostErrorInfo() will never be modified
 asyncronously by errors occurring in other PortAudio owned threads
 (such as the thread that manages the stream callback.)

 This function is provided as a last resort, primarily to enhance debugging
 by providing clients with access to all available error information.

 @return A pointer to an immutable structure constaining information about
 the host error. The values in this structure will only be valid if a
 PortAudio function has previously returned the paUnanticipatedHostError
 error code.
*/
const PaHostErrorInfo* Pa_GetLastHostErrorInfo( void );



/* Device enumeration and capabilities */

/** Retrieve the number of available devices. The number of available devices
 may be zero.

 @return A non-negative value indicating the number of available devices or,
 a PaErrorCode (which are always negative) if PortAudio is not initialized
 or an error is encountered.
*/
PaDeviceIndex Pa_GetDeviceCount( void );


/** Retrieve the index of the default input device. The result can be
 used in the inputDevice parameter to Pa_OpenStream().

 @return The default input device index for the default host API, or paNoDevice
 if no default input device is available or an error was encountered.
*/
PaDeviceIndex Pa_GetDefaultInputDevice( void );


/** Retrieve the index of the default output device. The result can be
 used in the outputDevice parameter to Pa_OpenStream().

 @return The default output device index for the defualt host API, or paNoDevice
 if no default output device is available or an error was encountered.

 @note
 On the PC, the user can specify a default device by
 setting an environment variable. For example, to use device #1.
<pre>
 set PA_RECOMMENDED_OUTPUT_DEVICE=1
</pre>
 The user should first determine the available device ids by using
 the supplied application "pa_devs".
*/
PaDeviceIndex Pa_GetDefaultOutputDevice( void );


/** The type used to represent monotonic time in seconds that can be used
 for syncronisation. The type is used for the outTime argument to the
 PaStreamCallback and as the result of Pa_GetStreamTime().
     
 @see PaStreamCallback, Pa_GetStreamTime
*/
typedef double PaTime;


/** A type used to specify one or more sample formats. Each value indicates
 a possible format for sound data passed to and from the stream callback,
 Pa_ReadStream and Pa_WriteStream.

 The standard formats paFloat32, paInt16, paInt32, paInt24, paInt8
 and aUInt8 are usually implemented by all implementations.

 The floating point representation (paFloat32) uses +1.0 and -1.0 as the
 maximum and minimum respectively.

 paUInt8 is an unsigned 8 bit format where 128 is considered "ground"

 The paNonInterleaved flag indicates that a multichannel buffer is passed
 as a set of non-interleaved pointers.

 @see Pa_OpenStream, Pa_OpenDefaultStream, PaDeviceInfo
 @see paFloat32, paInt16, paInt32, paInt24, paInt8
 @see paUInt8, paCustomFormat, paNonInterleaved
*/
typedef unsigned long PaSampleFormat;


#define paFloat32        ((PaSampleFormat) 0x00000001) /**< @see PaSampleFormat */
#define paInt32          ((PaSampleFormat) 0x00000002) /**< @see PaSampleFormat */
#define paInt24          ((PaSampleFormat) 0x00000004) /**< Packed 24 bit format. @see PaSampleFormat */
#define paInt16          ((PaSampleFormat) 0x00000008) /**< @see PaSampleFormat */
#define paInt8           ((PaSampleFormat) 0x00000010) /**< @see PaSampleFormat */
#define paUInt8          ((PaSampleFormat) 0x00000020) /**< @see PaSampleFormat */
#define paCustomFormat   ((PaSampleFormat) 0x00010000)/**< @see PaSampleFormat */

#define paNonInterleaved ((PaSampleFormat) 0x80000000)

/** A structure providing information and capabilities of PortAudio devices.
 Devices may support input, output or both input and output.
*/
typedef struct PaDeviceInfo
{
    int structVersion;  /* this is struct version 2 */
    const char *name;
    PaHostApiIndex hostApi; /* note this is a host API index, not a type id*/
    
    int maxInputChannels;
    int maxOutputChannels;

    /* Default latency values for interactive performance. */
    PaTime defaultLowInputLatency;
    PaTime defaultLowOutputLatency;
    /* Default latency values for robust non-interactive applications (eg. playing sound files). */
    PaTime defaultHighInputLatency;
    PaTime defaultHighOutputLatency;

    double defaultSampleRate;
} PaDeviceInfo;


/** Retrieve a pointer to a PaDeviceInfo structure containing information
 about the specified device.
 @return A pointer to an immutable PaDeviceInfo structure. If the device
 parameter is out of range the function returns NULL.

 @param device A valid device index in the range 0 to (Pa_GetDeviceCount()-1)

 @note PortAudio manages the memory referenced by the returned pointer,
 the client must not manipulate or free the memory. The pointer is only
 guaranteed to be valid between calls to Pa_Initialize() and Pa_Terminate().

 @see PaDeviceInfo, PaDeviceIndex
*/
const PaDeviceInfo* Pa_GetDeviceInfo( PaDeviceIndex device );


/** Parameters for one direction (input or output) of a stream.
*/
typedef struct PaStreamParameters
{
    /** A valid device index in the range 0 to (Pa_GetDeviceCount()-1)
     specifying the device to be used or the special constant
     paUseHostApiSpecificDeviceSpecification which indicates that the actual
     device(s) to use are specified in hostApiSpecificStreamInfo.
     This field must not be set to paNoDevice.
    */
    PaDeviceIndex device;
    
    /** The number of channels of sound to be delivered to the
     stream callback or accessed by Pa_ReadStream() or Pa_WriteStream().
     It can range from 1 to the value of maxInputChannels in the
     PaDeviceInfo record for the device specified by the device parameter.
    */
    int channelCount;

    /** The sample format of the buffer provided to the stream callback,
     a_ReadStream() or Pa_WriteStream(). It may be any of the formats described
     by the PaSampleFormat enumeration.
    */
    PaSampleFormat sampleFormat;

    /** The desired latency in seconds. Where practical, implementations should
     configure their latency based on these parameters, otherwise they may
     choose the closest viable latency instead. Unless the suggested latency
     is greater than the absolute upper limit for the device implementations
     should round the suggestedLatency up to the next practial value - ie to
     provide an equal or higher latency than suggestedLatency wherever possibe.
     Actual latency values for an open stream may be retrieved using the
     inputLatency and outputLatency fields of the PaStreamInfo structure
     returned by Pa_GetStreamInfo().
     @see default*Latency in PaDeviceInfo, *Latency in PaStreamInfo
    */
    PaTime suggestedLatency;

    /** An optional pointer to a host api specific data structure
     containing additional information for device setup and/or stream processing.
     hostApiSpecificStreamInfo is never required for correct operation,
     if not used it should be set to NULL.
    */
    void *hostApiSpecificStreamInfo;

} PaStreamParameters;


/** Return code for Pa_IsFormatSupported indicating success. */
#define paFormatIsSupported (0)

/** Determine whether it would be possible to open a stream with the specified
 parameters.

 @param inputParameters A structure that describes the input parameters used to
 open a stream. The suggestedLatency field is ignored. See PaStreamParameters
 for a description of these parameters. inputParameters must be NULL for
 output-only streams.

 @param outputParameters A structure that describes the output parameters used
 to open a stream. The suggestedLatency field is ignored. See PaStreamParameters
 for a description of these parameters. outputParameters must be NULL for
 input-only streams.

 @param sampleRate The required sampleRate. For full-duplex streams it is the
 sample rate for both input and output

 @return Returns 0 if the format is supported, and an error code indicating why
 the format is not supported otherwise. The constant paFormatIsSupported is
 provided to compare with the return value for success.

 @see paFormatIsSupported, PaStreamParameters
*/
PaError Pa_IsFormatSupported( const PaStreamParameters *inputParameters,
                              const PaStreamParameters *outputParameters,
                              double sampleRate );



/* Streaming types and functions */


/**
 A single PaStream can provide multiple channels of real-time
 streaming audio input and output to a client application. A stream
 provides access to audio hardware represented by one or more
 PaDevices. Depending on the underlying Host API, it may be possible 
 to open multiple streams using the same device, however this behavior 
 is implementation defined. Portable applications should assume that 
 a PaDevice may be simultaneously used by at most one PaStream.

 Pointers to PaStream objects are passed between PortAudio functions that
 operate on streams.

 @see Pa_OpenStream, Pa_OpenDefaultStream, Pa_OpenDefaultStream, Pa_CloseStream,
 Pa_StartStream, Pa_StopStream, Pa_AbortStream, Pa_IsStreamActive,
 Pa_GetStreamTime, Pa_GetStreamCpuLoad

*/
typedef void PaStream;


/** Can be passed as the framesPerBuffer parameter to Pa_OpenStream()
 or Pa_OpenDefaultStream() to indicate that the stream callback will
 accept buffers of any size.
*/
#define paFramesPerBufferUnspecified  (0)


/** Flags used to control the behavior of a stream. They are passed as
 parameters to Pa_OpenStream or Pa_OpenDefaultStream. Multiple flags may be
 ORed together.

 @see Pa_OpenStream, Pa_OpenDefaultStream
 @see paNoFlag, paClipOff, paDitherOff, paNeverDropInput,
  paPrimeOutputBuffersUsingStreamCallback, paPlatformSpecificFlags
*/
typedef unsigned long PaStreamFlags;

/** @see PaStreamFlags */
#define   paNoFlag          ((PaStreamFlags) 0)

/** Disable default clipping of out of range samples.
 @see PaStreamFlags
*/
#define   paClipOff         ((PaStreamFlags) 0x00000001)

/** Disable default dithering.
 @see PaStreamFlags
*/
#define   paDitherOff       ((PaStreamFlags) 0x00000002)

/** Flag requests that where possible a full duplex stream will not discard
 overflowed input samples without calling the stream callback. This flag is
 only valid for full duplex callback streams and only when used in combination
 with the paFramesPerBufferUnspecified (0) framesPerBuffer parameter. Using
 this flag incorrectly results in a paInvalidFlag error being returned from
 Pa_OpenStream and Pa_OpenDefaultStream.

 @see PaStreamFlags, paFramesPerBufferUnspecified
*/
#define   paNeverDropInput  ((PaStreamFlags) 0x00000004)

/** Call the stream callback to fill initial output buffers, rather than the
 default behavior of priming the buffers with zeros (silence). This flag has
 no effect for input-only and blocking read/write streams.
 
 @see PaStreamFlags
*/
#define   paPrimeOutputBuffersUsingStreamCallback ((PaStreamFlags) 0x00000008)

/** A mask specifying the platform specific bits.
 @see PaStreamFlags
*/
#define   paPlatformSpecificFlags ((PaStreamFlags)0xFFFF0000)

/**
 Timing information for the buffers passed to the stream callback.
*/
typedef struct PaStreamCallbackTimeInfo{
    PaTime inputBufferAdcTime;
    PaTime currentTime;
    PaTime outputBufferDacTime;
} PaStreamCallbackTimeInfo;


/**
 Flag bit constants for the statusFlags to PaStreamCallback.

 @see paInputUnderflow, paInputOverflow, paOutputUnderflow, paOutputOverflow,
 paPrimingOutput
*/
typedef unsigned long PaStreamCallbackFlags;

/** In a stream opened with paFramesPerBufferUnspecified, indicates that
 input data is all silence (zeros) because no real data is available. In a
 stream opened without paFramesPerBufferUnspecified, it indicates that one or
 more zero samples have been inserted into the input buffer to compensate
 for an input underflow.
 @see PaStreamCallbackFlags
*/
#define paInputUnderflow   ((PaStreamCallbackFlags) 0x00000001)

/** In a stream opened with paFramesPerBufferUnspecified, indicates that data
 prior to the first sample of the input buffer was discarded due to an
 overflow, possibly because the stream callback is using too much CPU time.
 Otherwise indicates that data prior to one or more samples in the
 input buffer was discarded.
 @see PaStreamCallbackFlags
*/
#define paInputOverflow    ((PaStreamCallbackFlags) 0x00000002)

/** Indicates that output data (or a gap) was inserted, possibly because the
 stream callback is using too much CPU time.
 @see PaStreamCallbackFlags
*/
#define paOutputUnderflow  ((PaStreamCallbackFlags) 0x00000004)

/** Indicates that output data will be discarded because no room is available.
 @see PaStreamCallbackFlags
*/
#define paOutputOverflow   ((PaStreamCallbackFlags) 0x00000008)

/** Some of all of the output data will be used to prime the stream, input
 data may be zero.
 @see PaStreamCallbackFlags
*/
#define paPrimingOutput    ((PaStreamCallbackFlags) 0x00000010)

/**
 Allowable return values for the PaStreamCallback.
 @see PaStreamCallback
*/
typedef enum PaStreamCallbackResult
{
    paContinue=0,
    paComplete=1,
    paAbort=2
} PaStreamCallbackResult;


/**
 Functions of type PaStreamCallback are implemented by PortAudio clients.
 They consume, process or generate audio in response to requests from an
 active PortAudio stream.
     
 @param input and @param output are arrays of interleaved samples,
 the format, packing and number of channels used by the buffers are
 determined by parameters to Pa_OpenStream().
     
 @param frameCount The number of sample frames to be processed by
 the stream callback.

 @param timeInfo The time in seconds when the first sample of the input
 buffer was received at the audio input, the time in seconds when the first
 sample of the output buffer will begin being played at the audio output, and
 the time in seconds when the stream callback was called.
 See also Pa_GetStreamTime()

 @param statusFlags Flags indicating whether input and/or output buffers
 have been inserted or will be dropped to overcome underflow or overflow
 conditions.

 @param userData The value of a user supplied pointer passed to
 Pa_OpenStream() intended for storing synthesis data etc.

 @return
 The stream callback should return one of the values in the
 PaStreamCallbackResult enumeration. To ensure that the callback continues
 to be called, it should return paContinue (0). Either paComplete or paAbort
 can be returned to finish stream processing, after either of these values is
 returned the callback will not be called again. If paAbort is returned the
 stream will finish as soon as possible. If paComplete is returned, the stream
 will continue until all buffers generated by the callback have been played.
 This may be useful in applications such as soundfile players where a specific
 duration of output is required. However, it is not necessary to utilise this
 mechanism as Pa_StopStream(), Pa_AbortStream() or Pa_CloseStream() can also
 be used to stop the stream. The callback must always fill the entire output
 buffer irrespective of its return value.

 @see Pa_OpenStream, Pa_OpenDefaultStream

 @note With the exception of Pa_GetStreamCpuLoad() it is not permissable to call
 PortAudio API functions from within the stream callback.
*/
typedef int PaStreamCallback(
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData );


/** Opens a stream for either input, output or both.
     
 @param stream The address of a PaStream pointer which will receive
 a pointer to the newly opened stream.
     
 @param inputParameters A structure that describes the input parameters used by
 the opened stream. See PaStreamParameters for a description of these parameters.
 inputParameters must be NULL for output-only streams.

 @param outputParameters A structure that describes the output parameters used by
 the opened stream. See PaStreamParameters for a description of these parameters.
 outputParameters must be NULL for input-only streams.
 
 @param sampleRate The desired sampleRate. For full-duplex streams it is the
 sample rate for both input and output
     
 @param framesPerBuffer The number of frames passed to the stream callback
 function, or the preferred block granularity for a blocking read/write stream.
 The special value paFramesPerBufferUnspecified (0) may be used to request that
 the stream callback will recieve an optimal (and possibly varying) number of
 frames based on host requirements and the requested latency settings.
 Note: With some host APIs, the use of non-zero framesPerBuffer for a callback
 stream may introduce an additional layer of buffering which could introduce
 additional latency. PortAudio guarantees that the additional latency
 will be kept to the theoretical minimum however, it is strongly recommended
 that a non-zero framesPerBuffer value only be used when your algorithm
 requires a fixed number of frames per stream callback.
 
 @param streamFlags Flags which modify the behaviour of the streaming process.
 This parameter may contain a combination of flags ORed together. Some flags may
 only be relevant to certain buffer formats.
     
 @param streamCallback A pointer to a client supplied function that is responsible
 for processing and filling input and output buffers. If this parameter is NULL
 the stream will be opened in 'blocking read/write' mode. In blocking mode,
 the client can receive sample data using Pa_ReadStream and write sample data
 using Pa_WriteStream, the number of samples that may be read or written
 without blocking is returned by Pa_GetStreamReadAvailable and
 Pa_GetStreamWriteAvailable respectively.

 @param userData A client supplied pointer which is passed to the stream callback
 function. It could for example, contain a pointer to instance data necessary
 for processing the audio buffers. This parameter is ignored if streamCallback
 is NULL.
     
 @return
 Upon success Pa_OpenStream() returns paNoError and places a pointer to a
 valid PaStream in the stream argument. The stream is inactive (stopped).
 If a call to Pa_OpenStream() fails, a non-zero error code is returned (see
 PaError for possible error codes) and the value of stream is invalid.

 @see PaStreamParameters, PaStreamCallback, Pa_ReadStream, Pa_WriteStream,
 Pa_GetStreamReadAvailable, Pa_GetStreamWriteAvailable
*/
PaError Pa_OpenStream( PaStream** stream,
                       const PaStreamParameters *inputParameters,
                       const PaStreamParameters *outputParameters,
                       double sampleRate,
                       unsigned long framesPerBuffer,
                       PaStreamFlags streamFlags,
                       PaStreamCallback *streamCallback,
                       void *userData );


/** A simplified version of Pa_OpenStream() that opens the default input
 and/or output devices.

 @param stream The address of a PaStream pointer which will receive
 a pointer to the newly opened stream.
 
 @param numInputChannels  The number of channels of sound that will be supplied
 to the stream callback or returned by Pa_ReadStream. It can range from 1 to
 the value of maxInputChannels in the PaDeviceInfo record for the default input
 device. If 0 the stream is opened as an output-only stream.

 @param numOutputChannels The number of channels of sound to be delivered to the
 stream callback or passed to Pa_WriteStream. It can range from 1 to the value
 of maxOutputChannels in the PaDeviceInfo record for the default output dvice.
 If 0 the stream is opened as an output-only stream.

 @param sampleFormat The sample format of both the input and output buffers
 provided to the callback or passed to and from Pa_ReadStream and Pa_WriteStream.
 sampleFormat may be any of the formats described by the PaSampleFormat
 enumeration.
 
 @param sampleRate Same as Pa_OpenStream parameter of the same name.
 @param framesPerBuffer Same as Pa_OpenStream parameter of the same name.
 @param streamCallback Same as Pa_OpenStream parameter of the same name.
 @param userData Same as Pa_OpenStream parameter of the same name.

 @return As for Pa_OpenStream

 @see Pa_OpenStream, PaStreamCallback
*/
PaError Pa_OpenDefaultStream( PaStream** stream,
                              int numInputChannels,
                              int numOutputChannels,
                              PaSampleFormat sampleFormat,
                              double sampleRate,
                              unsigned long framesPerBuffer,
                              PaStreamCallback *streamCallback,
                              void *userData );


/** Closes an audio stream. If the audio stream is active it
 discards any pending buffers as if Pa_AbortStream() had been called.
*/
PaError Pa_CloseStream( PaStream *stream );


/** Functions of type PaStreamFinishedCallback are implemented by PortAudio 
 clients. They can be registered with a stream using the Pa_SetStreamFinishedCallback
 function. Once registered they are called when the stream becomes inactive
 (ie once a call to Pa_StopStream() will not block).
 A stream will become inactive after the stream callback returns non-zero,
 or when Pa_StopStream or Pa_AbortStream is called. For a stream providing audio
 output, if the stream callback returns paComplete, or Pa_StopStream is called,
 the stream finished callback will not be called until all generated sample data
 has been played.
 
 @param userData The userData parameter supplied to Pa_OpenStream()

 @see Pa_SetStreamFinishedCallback
*/
typedef void PaStreamFinishedCallback( void *userData );


/** Register a stream finished callback function which will be called when the 
 stream becomes inactive. See the description of PaStreamFinishedCallback for 
 further details about when the callback will be called.

 @param stream a pointer to a PaStream that is in the stopped state - if the
 stream is not stopped, the stream's finished callback will remain unchanged 
 and an error code will be returned.

 @param streamFinishedCallback a pointer to a function with the same signature
 as PaStreamFinishedCallback, that will be called when the stream becomes
 inactive. Passing NULL for this parameter will un-register a previously
 registered stream finished callback function.

 @return on success returns paNoError, otherwise an error code indicating the cause
 of the error.

 @see PaStreamFinishedCallback
*/
PaError Pa_SetStreamFinishedCallback( PaStream *stream, PaStreamFinishedCallback* streamFinishedCallback ); 


/** Commences audio processing.
*/
PaError Pa_StartStream( PaStream *stream );


/** Terminates audio processing. It waits until all pending
 audio buffers have been played before it returns.
*/
PaError Pa_StopStream( PaStream *stream );


/** Terminates audio processing immediately without waiting for pending
 buffers to complete.
*/
PaError Pa_AbortStream( PaStream *stream );


/** Determine whether the stream is stopped.
 A stream is considered to be stopped prior to a successful call to
 Pa_StartStream and after a successful call to Pa_StopStream or Pa_AbortStream.
 If a stream callback returns a value other than paContinue the stream is NOT
 considered to be stopped.

 @return Returns one (1) when the stream is stopped, zero (0) when
 the stream is running or, a PaErrorCode (which are always negative) if
 PortAudio is not initialized or an error is encountered.

 @see Pa_StopStream, Pa_AbortStream, Pa_IsStreamActive
*/
PaError Pa_IsStreamStopped( PaStream *stream );


/** Determine whether the stream is active.
 A stream is active after a successful call to Pa_StartStream(), until it
 becomes inactive either as a result of a call to Pa_StopStream() or
 Pa_AbortStream(), or as a result of a return value other than paContinue from
 the stream callback. In the latter case, the stream is considered inactive
 after the last buffer has finished playing.

 @return Returns one (1) when the stream is active (ie playing or recording
 audio), zero (0) when not playing or, a PaErrorCode (which are always negative)
 if PortAudio is not initialized or an error is encountered.

 @see Pa_StopStream, Pa_AbortStream, Pa_IsStreamStopped
*/
PaError Pa_IsStreamActive( PaStream *stream );



/** A structure containing unchanging information about an open stream.
 @see Pa_GetStreamInfo
*/

typedef struct PaStreamInfo
{
    /** this is struct version 1 */
    int structVersion;

    /** The input latency of the stream in seconds. This value provides the most
     accurate estimate of input latency available to the implementation. It may
     differ significantly from the suggestedLatency value passed to Pa_OpenStream().
     The value of this field will be zero (0.) for output-only streams.
     @see PaTime
    */
    PaTime inputLatency;

    /** The output latency of the stream in seconds. This value provides the most
     accurate estimate of output latency available to the implementation. It may
     differ significantly from the suggestedLatency value passed to Pa_OpenStream().
     The value of this field will be zero (0.) for input-only streams.
     @see PaTime
    */
    PaTime outputLatency;

    /** The sample rate of the stream in Hertz (samples per second). In cases
     where the hardware sample rate is inaccurate and PortAudio is aware of it,
     the value of this field may be different from the sampleRate parameter
     passed to Pa_OpenStream(). If information about the actual hardware sample
     rate is not available, this field will have the same value as the sampleRate
     parameter passed to Pa_OpenStream().
    */
    double sampleRate;
    
} PaStreamInfo;


/** Retrieve a pointer to a PaStreamInfo structure containing information
 about the specified stream.
 @return A pointer to an immutable PaStreamInfo structure. If the stream
 parameter invalid, or an error is encountered, the function returns NULL.

 @param stream A pointer to an open stream previously created with Pa_OpenStream.

 @note PortAudio manages the memory referenced by the returned pointer,
 the client must not manipulate or free the memory. The pointer is only
 guaranteed to be valid until the specified stream is closed.

 @see PaStreamInfo
*/
const PaStreamInfo* Pa_GetStreamInfo( PaStream *stream );


/** Determine the current time for the stream according to the same clock used
 to generate buffer timestamps. This time may be used for syncronising other
 events to the audio stream, for example synchronizing audio to MIDI.
                                        
 @return The stream's current time in seconds, or 0 if an error occurred.

 @see PaTime, PaStreamCallback
*/
PaTime Pa_GetStreamTime( PaStream *stream );


/** Retrieve CPU usage information for the specified stream.
 The "CPU Load" is a fraction of total CPU time consumed by a callback stream's
 audio processing routines including, but not limited to the client supplied
 stream callback. This function does not work with blocking read/write streams.

 This function may be called from the stream callback function or the
 application.
     
 @return
 A floating point value, typically between 0.0 and 1.0, where 1.0 indicates
 that the stream callback is consuming the maximum number of CPU cycles possible
 to maintain real-time operation. A value of 0.5 would imply that PortAudio and
 the stream callback was consuming roughly 50% of the available CPU time. The
 return value may exceed 1.0. A value of 0.0 will always be returned for a
 blocking read/write stream, or if an error occurrs.
*/
double Pa_GetStreamCpuLoad( PaStream* stream );


/** Read samples from an input stream. The function doesn't return until
 the entire buffer has been filled - this may involve waiting for the operating
 system to supply the data.

 @param stream A pointer to an open stream previously created with Pa_OpenStream.
 
 @param buffer A pointer to a buffer of sample frames. The buffer contains
 samples in the format specified by the inputParameters->sampleFormat field
 used to open the stream, and the number of channels specified by
 inputParameters->numChannels. If non-interleaved samples were requested,
 buffer is a pointer to the first element of an array of non-interleaved
 buffer pointers, one for each channel.

 @param frames The number of frames to be read into buffer. This parameter
 is not constrained to a specific range, however high performance applications
 will want to match this parameter to the framesPerBuffer parameter used
 when opening the stream.

 @return On success PaNoError will be returned, or PaInputOverflowed if input
 data was discarded by PortAudio after the previous call and before this call.
*/
PaError Pa_ReadStream( PaStream* stream,
                       void *buffer,
                       unsigned long frames );


/** Write samples to an output stream. This function doesn't return until the
 entire buffer has been consumed - this may involve waiting for the operating
 system to consume the data.

 @param stream A pointer to an open stream previously created with Pa_OpenStream.

 @param buffer A pointer to a buffer of sample frames. The buffer contains
 samples in the format specified by the outputParameters->sampleFormat field
 used to open the stream, and the number of channels specified by
 outputParameters->numChannels. If non-interleaved samples were requested,
 buffer is a pointer to the first element of an array of non-interleaved
 buffer pointers, one for each channel.

 @param frames The number of frames to be written from buffer. This parameter
 is not constrained to a specific range, however high performance applications
 will want to match this parameter to the framesPerBuffer parameter used
 when opening the stream.

 @return On success PaNoError will be returned, or paOutputUnderflowed if
 additional output data was inserted after the previous call and before this
 call.
*/
PaError Pa_WriteStream( PaStream* stream,
                        const void *buffer,
                        unsigned long frames );


/** Retrieve the number of frames that can be read from the stream without
 waiting.

 @return Returns a non-negative value representing the maximum number of frames
 that can be read from the stream without blocking or busy waiting or, a
 PaErrorCode (which are always negative) if PortAudio is not initialized or an
 error is encountered.
*/
signed long Pa_GetStreamReadAvailable( PaStream* stream );


/** Retrieve the number of frames that can be written to the stream without
 waiting.

 @return Returns a non-negative value representing the maximum number of frames
 that can be written to the stream without blocking or busy waiting or, a
 PaErrorCode (which are always negative) if PortAudio is not initialized or an
 error is encountered.
*/
signed long Pa_GetStreamWriteAvailable( PaStream* stream );


/* Miscellaneous utilities */


/** Retrieve the size of a given sample format in bytes.

 @return The size in bytes of a single sample in the specified format,
 or paSampleFormatNotSupported if the format is not supported.
*/
PaError Pa_GetSampleSize( PaSampleFormat format );


/** Put the caller to sleep for at least 'msec' milliseconds. This function is
 provided only as a convenience for authors of portable code (such as the tests
 and examples in the PortAudio distribution.)

 The function may sleep longer than requested so don't rely on this for accurate
 musical timing.
*/
void Pa_Sleep( long msec );



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PORTAUDIO_H */
