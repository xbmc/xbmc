/*
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 *
 * PortAudio v18 version of AudioScience HPI driver by Fred Gleason <fredg@salemradiolabs.com>
 * PortAudio v19 version of AudioScience HPI driver by Ludwig Schwardt <schwardt@sun.ac.za>
 *
 * Copyright (c) 2003 Fred Gleason
 * Copyright (c) 2005,2006 Ludwig Schwardt
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

/*
 * Modification History
 * 12/2003 - Initial version
 * 09/2005 - v19 version [rewrite]
 */

/** @file
 @ingroup hostapi_src
 @brief Host API implementation supporting AudioScience cards
        via the Linux HPI interface.

 <h3>Overview</h3>

 This is a PortAudio implementation for the AudioScience HPI Audio API
 on the Linux platform. AudioScience makes a range of audio adapters customised
 for the broadcasting industry, with support for both Windows and Linux.
 More information on their products can be found on their website:

     http://www.audioscience.com

 Documentation for the HPI API can be found at:

     http://www.audioscience.com/internet/download/sdk/spchpi.pdf

 The Linux HPI driver itself (a kernel module + library) can be downloaded from:

     http://www.audioscience.com/internet/download/linux_drivers.htm

 <h3>Implementation strategy</h3>

 *Note* Ideally, AudioScience cards should be handled by the PortAudio ALSA
 implementation on Linux, as ALSA is the preferred Linux soundcard API. The existence
 of this host API implementation might therefore seem a bit flawed. Unfortunately, at
 the time of the creation of this implementation (June 2006), the PA ALSA implementation
 could not make use of the existing AudioScience ALSA driver. PA ALSA uses the
 "memory-mapped" (mmap) ALSA access mode to interact with the ALSA library, while the
 AudioScience ALSA driver only supports the "read-write" access mode. The appropriate
 solution to this problem is to add "read-write" support to PortAudio ALSA, thereby
 extending the range of soundcards it supports (AudioScience cards are not the only
 ones with this problem). Given the author's limited knowledge of ALSA and the
 simplicity of the HPI API, the second-best solution was born...

 The following mapping between HPI and PA was followed:
 HPI subsystem => PortAudio host API
 HPI adapter => nothing specific
 HPI stream => PortAudio device

 Each HPI stream is either input or output (not both), and can support
 different channel counts, sampling rates and sample formats. It is therefore
 a more natural fit to a PA device. A PA stream can therefore combine two
 HPI streams (one input and one output) into a "full-duplex" stream. These
 HPI streams can even be on different physical adapters. The two streams ought to be
 sample-synchronised when they reside on the same adapter, as most AudioScience adapters
 derive their ADC and DAC clocks from one master clock. When combining two adapters
 into one full-duplex stream, however, the use of a word clock connection between the
 adapters is strongly recommended.

 The HPI interface is inherently blocking, making use of read and write calls to
 transfer data between user buffers and driver buffers. The callback interface therefore
 requires a helper thread ("callback engine") which periodically transfers data (one thread
 per PA stream, in fact). The current implementation explicitly sleeps via Pa_Sleep() until
 enough samples can be transferred (select() or poll() would be better, but currently seems
 impossible...). The thread implementation makes use of the Unix thread helper functions
 and some pthread calls here and there. If a unified PA thread exists, this host API
 implementation might also compile on Windows, as this is the only real Linux-specific
 part of the code.

 There is no inherent fixed buffer size in the HPI interface, as in some other host APIs.
 The PortAudio implementation contains a buffer that is allocated during OpenStream and
 used to transfer data between the callback and the HPI driver buffer. The size of this
 buffer is quite flexible and is derived from latency suggestions and matched to the
 requested callback buffer size as far as possible. It can become quite huge, as the
 AudioScience cards are typically geared towards higher-latency applications and contain
 large hardware buffers.

 The HPI interface natively supports most common sample formats and sample rates (some
 conversion is done on the adapter itself).

 Stream time is measured based on the number of processed frames, which is adjusted by the
 number of frames currently buffered by the HPI driver.

 There is basic support for detecting overflow and underflow. The HPI interface does not
 explicitly indicate this, so thresholds on buffer levels are used in combination with
 stream state. Recovery from overflow and underflow is left to the PA client.

 Blocking streams are also implemented. It makes use of the same polling routines that
 the callback interface uses, in order to prevent the allocation of variable-sized
 buffers during reading and writing. The framesPerBuffer parameter is therefore still
 relevant, and this can be increased in the blocking case to improve efficiency.

 The implementation contains extensive reporting macros (slightly modified PA_ENSURE and
 PA_UNLESS versions) and a useful stream dump routine to provide debugging feedback.

 Output buffer priming via the user callback (i.e. paPrimeOutputBuffersUsingStreamCallback
 and friends) is not implemented yet. All output is primed with silence.

 Please send bug reports etc. to Ludwig Schwardt <schwardt@sun.ac.za>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>          /* strlen() */
#include <pthread.h>         /* pthreads and friends */
#include <assert.h>          /* assert */
#include <math.h>            /* ceil, floor */

#include <asihpi/hpi.h>      /* HPI API */

#include "portaudio.h"       /* PortAudio API */
#include "pa_util.h"         /* PA_DEBUG, other small utilities */
#include "pa_unix_util.h"    /* Unix threading utilities */
#include "pa_allocation.h"   /* Group memory allocation */
#include "pa_hostapi.h"      /* Host API structs */
#include "pa_stream.h"       /* Stream interface structs */
#include "pa_cpuload.h"      /* CPU load measurer */
#include "pa_process.h"      /* Buffer processor */
#include "pa_converters.h"   /* PaUtilZeroer */
#include "pa_debugprint.h"

/* -------------------------------------------------------------------------- */

/*
 * Defines
 */

/* Error reporting and assertions */

/** Evaluate expression, and return on any PortAudio errors */
#define PA_ENSURE_(expr) \
    do { \
        PaError paError = (expr); \
        if( UNLIKELY( paError < paNoError ) ) \
        { \
            PA_DEBUG(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            result = paError; \
            goto error; \
        } \
    } while (0);

/** Assert expression, else return the provided PaError */
#define PA_UNLESS_(expr, paError) \
    do { \
        if( UNLIKELY( (expr) == 0 ) ) \
        { \
            PA_DEBUG(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            result = (paError); \
            goto error; \
        } \
    } while( 0 );

/** Check return value of HPI function, and map it to PaError */
#define PA_ASIHPI_UNLESS_(expr, paError) \
    do { \
        HW16 hpiError = (expr); \
        /* If HPI error occurred */ \
        if( UNLIKELY( hpiError ) ) \
        { \
	    char szError[256]; \
	    HPI_GetErrorText( hpiError, szError ); \
	    PA_DEBUG(( "HPI error %d occurred: %s\n", hpiError, szError )); \
	    /* This message will always be displayed, even if debug info is disabled */ \
            PA_DEBUG(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            if( (paError) == paUnanticipatedHostError ) \
	    { \
	        PA_DEBUG(( "Host error description: %s\n", szError )); \
	        /* PaUtil_SetLastHostErrorInfo should only be used in the main thread */ \
	        if( pthread_equal( pthread_self(), paUnixMainThread ) ) \
                { \
		    PaUtil_SetLastHostErrorInfo( paInDevelopment, hpiError, szError ); \
                } \
	    } \
	    /* If paNoError is specified, continue as usual */ \
            /* (useful if you only want to print out the debug messages above) */ \
	    if( (paError) < 0 ) \
	    { \
	        result = (paError); \
	        goto error; \
	    } \
        } \
    } while( 0 );

/** Report HPI error code and text */
#define PA_ASIHPI_REPORT_ERROR_(hpiErrorCode) \
    do { \
        char szError[256]; \
        HPI_GetErrorText( hpiError, szError ); \
        PA_DEBUG(( "HPI error %d occurred: %s\n", hpiError, szError )); \
        /* PaUtil_SetLastHostErrorInfo should only be used in the main thread */ \
        if( pthread_equal( pthread_self(), paUnixMainThread ) ) \
	{ \
	    PaUtil_SetLastHostErrorInfo( paInDevelopment, (hpiErrorCode), szError ); \
	} \
    } while( 0 );

/* Defaults */

/** Sample formats available natively on AudioScience hardware */
#define PA_ASIHPI_AVAILABLE_FORMATS_ (paFloat32 | paInt32 | paInt24 | paInt16 | paUInt8)
/** Enable background bus mastering (BBM) for buffer transfers, if available (see HPI docs) */
#define PA_ASIHPI_USE_BBM_ 1
/** Minimum number of frames in HPI buffer (for either data or available space).
 If buffer contains less data/space, it indicates xrun or completion. */
#define PA_ASIHPI_MIN_FRAMES_ 1152
/** Minimum polling interval in milliseconds, which determines minimum host buffer size */
#define PA_ASIHPI_MIN_POLLING_INTERVAL_ 10

/* -------------------------------------------------------------------------- */

/*
 * Structures
 */

/** Host API global data */
typedef struct PaAsiHpiHostApiRepresentation
{
    /* PortAudio "base class" - keep the baseRep first! (C-style inheritance) */
    PaUtilHostApiRepresentation baseHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;

    PaUtilAllocationGroup *allocations;

    /* implementation specific data goes here */

    PaHostApiIndex hostApiIndex;
    /** HPI subsystem pointer */
    HPI_HSUBSYS *subSys;
}
PaAsiHpiHostApiRepresentation;


/** Device data */
typedef struct PaAsiHpiDeviceInfo
{
    /* PortAudio "base class" - keep the baseRep first! (C-style inheritance) */
    /** Common PortAudio device information */
    PaDeviceInfo baseDeviceInfo;

    /* implementation specific data goes here */

    /** HPI subsystem (required for most HPI calls) */
    HPI_HSUBSYS *subSys;
    /** Adapter index */
    HW16 adapterIndex;
    /** Adapter model number (hex) */
    HW16 adapterType;
    /** Adapter HW/SW version */
    HW16 adapterVersion;
    /** Adapter serial number */
    HW32 adapterSerialNumber;
    /** Stream number */
    HW16 streamIndex;
    /** 0=Input, 1=Output (HPI streams are either input or output but not both) */
    HW16 streamIsOutput;
}
PaAsiHpiDeviceInfo;


/** Stream state as defined by PortAudio.
 It seems that the host API implementation has to keep track of the PortAudio stream state.
 Please note that this is NOT the same as the state of the underlying HPI stream. By separating
 these two concepts, a lot of flexibility is gained. There is a rough match between the two,
 of course, but forcing a precise match is difficult. For example, HPI_STATE_DRAINED can occur
 during the Active state of PortAudio (due to underruns) and also during CallBackFinished in
 the case of an output stream. Similarly, HPI_STATE_STOPPED mostly coincides with the Stopped
 PortAudio state, by may also occur in the CallbackFinished state when recording is finished.

 Here is a rough match-up:

 PortAudio state   =>     HPI state
 ---------------          ---------
 Active            =>     HPI_STATE_RECORDING, HPI_STATE_PLAYING, (HPI_STATE_DRAINED)
 Stopped           =>     HPI_STATE_STOPPED
 CallbackFinished  =>     HPI_STATE_STOPPED, HPI_STATE_DRAINED */
typedef enum PaAsiHpiStreamState
{
    paAsiHpiStoppedState=0,
    paAsiHpiActiveState=1,
    paAsiHpiCallbackFinishedState=2
}
PaAsiHpiStreamState;


/** Stream component data (associated with one direction, i.e. either input or output) */
typedef struct PaAsiHpiStreamComponent
{
    /** Device information (HPI handles, etc) */
    PaAsiHpiDeviceInfo *hpiDevice;
    /** Stream handle, as passed to HPI interface.
     HACK: we assume types HPI_HISTREAM and HPI_HOSTREAM are the same...
     (both are HW32 up to version 3.00 of ASIHPI, and hopefully they stay that way) */
    HPI_HISTREAM hpiStream;
    /** Stream format, as passed to HPI interface */
    HPI_FORMAT hpiFormat;
    /** Number of bytes per frame, derived from hpiFormat and saved for convenience */
    HW32 bytesPerFrame;
    /** Size of hardware (on-card) buffer of stream in bytes */
    HW32 hardwareBufferSize;
    /** Size of host (BBM) buffer of stream in bytes (if used) */
    HW32 hostBufferSize;
    /** Upper limit on the utilization of output stream buffer (both hardware and host).
     This prevents large latencies in an output-only stream with a potentially huge buffer
     and a fast data generator, which would otherwise keep the hardware buffer filled to
     capacity. See also the "Hardware Buffering=off" option in the AudioScience WAV driver. */
    HW32 outputBufferCap;
    /** Sample buffer (halfway station between HPI and buffer processor) */
    HW8 *tempBuffer;
    /** Sample buffer size, in bytes */
    HW32 tempBufferSize;
}
PaAsiHpiStreamComponent;


/** Stream data */
typedef struct PaAsiHpiStream
{
    /* PortAudio "base class" - keep the baseRep first! (C-style inheritance) */
    PaUtilStreamRepresentation baseStreamRep;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    PaUtilAllocationGroup *allocations;

    /* implementation specific data goes here */

    /** Separate structs for input and output sides of stream */
    PaAsiHpiStreamComponent *input, *output;

    /** Polling interval (in milliseconds) */
    HW32 pollingInterval;
    /** Are we running in callback mode? */
    int callbackMode;
    /** Number of frames to transfer at a time to/from HPI */
    unsigned long maxFramesPerHostBuffer;
    /** Indicates that the stream is in the paNeverDropInput mode */
    int neverDropInput;
    /** Contains copy of user buffers, used by blocking interface to transfer non-interleaved data.
     It went here instead of to each stream component, as the stream component buffer setup in
     PaAsiHpi_SetupBuffers doesn't know the stream details such as callbackMode.
     (Maybe a problem later if ReadStream and WriteStream happens concurrently on same stream.) */
    void **blockingUserBufferCopy;

    /* Thread-related variables */

    /** Helper thread which will deliver data to user callback */
    PaUnixThread thread;
    /** PortAudio stream state (Active/Stopped/CallbackFinished) */
    volatile sig_atomic_t state;
    /** Hard abort, i.e. drop frames? */
    volatile sig_atomic_t callbackAbort;
    /** True if stream stopped via exiting callback with paComplete/paAbort flag
     (as opposed to explicit call to StopStream/AbortStream) */
    volatile sig_atomic_t callbackFinished;
}
PaAsiHpiStream;


/** Stream state information, collected together for convenience */
typedef struct PaAsiHpiStreamInfo
{
    /** HPI stream state (HPI_STATE_STOPPED, HPI_STATE_PLAYING, etc.) */
    HW16 state;
    /** Size (in bytes) of recording/playback data buffer in HPI driver */
    HW32 bufferSize;
    /** Amount of data (in bytes) available in the buffer */
    HW32 dataSize;
    /** Number of frames played/recorded since last stream reset */
    HW32 frameCounter;
    /** Amount of data (in bytes) in hardware (on-card) buffer.
     This differs from dataSize if bus mastering (BBM) is used, which introduces another
     driver-level buffer to which dataSize/bufferSize then refers. */
    HW32 auxDataSize;
    /** Total number of data frames currently buffered by HPI driver (host + hw buffers) */
    HW32 totalBufferedData;
    /** Size of immediately available data (for input) or space (for output) in frames.
     This only checks the first-level buffer (typically host buffer). This amount can be
     transferred immediately. */
    HW32 availableFrames;
    /** Indicates that hardware buffer is getting too full */
    int overflow;
    /** Indicates that hardware buffer is getting too empty */
    int underflow;
}
PaAsiHpiStreamInfo;

/* -------------------------------------------------------------------------- */

/*
 * Function prototypes
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* The only exposed function in the entire host API implementation */
    PaError PaAsiHpi_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );

#ifdef __cplusplus
}
#endif /* __cplusplus */

static void Terminate( struct PaUtilHostApiRepresentation *hostApi );
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate );

/* Stream prototypes */
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream **s,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData );
static PaError CloseStream( PaStream *s );
static PaError StartStream( PaStream *s );
static PaError StopStream( PaStream *s );
static PaError AbortStream( PaStream *s );
static PaError IsStreamStopped( PaStream *s );
static PaError IsStreamActive( PaStream *s );
static PaTime GetStreamTime( PaStream *s );
static double GetStreamCpuLoad( PaStream *s );

/* Blocking prototypes */
static PaError ReadStream( PaStream *s, void *buffer, unsigned long frames );
static PaError WriteStream( PaStream *s, const void *buffer, unsigned long frames );
static signed long GetStreamReadAvailable( PaStream *s );
static signed long GetStreamWriteAvailable( PaStream *s );

/* Callback prototypes */
static void *CallbackThreadFunc( void *userData );

/* Functions specific to this API */
static PaError PaAsiHpi_BuildDeviceList( PaAsiHpiHostApiRepresentation *hpiHostApi );
static HW16 PaAsiHpi_PaToHpiFormat( PaSampleFormat paFormat );
static PaSampleFormat PaAsiHpi_HpiToPaFormat( HW16 hpiFormat );
static PaError PaAsiHpi_CreateFormat( struct PaUtilHostApiRepresentation *hostApi,
                                      const PaStreamParameters *parameters, double sampleRate,
                                      PaAsiHpiDeviceInfo **hpiDevice, HPI_FORMAT *hpiFormat );
static PaError PaAsiHpi_OpenInput( struct PaUtilHostApiRepresentation *hostApi,
                                   const PaAsiHpiDeviceInfo *hpiDevice, const HPI_FORMAT *hpiFormat,
                                   HPI_HISTREAM *hpiStream );
static PaError PaAsiHpi_OpenOutput( struct PaUtilHostApiRepresentation *hostApi,
                                    const PaAsiHpiDeviceInfo *hpiDevice, const HPI_FORMAT *hpiFormat,
                                    HPI_HOSTREAM *hpiStream );
static PaError PaAsiHpi_GetStreamInfo( PaAsiHpiStreamComponent *streamComp, PaAsiHpiStreamInfo *info );
static void PaAsiHpi_StreamComponentDump( PaAsiHpiStreamComponent *streamComp, PaAsiHpiStream *stream );
static void PaAsiHpi_StreamDump( PaAsiHpiStream *stream );
static PaError PaAsiHpi_SetupBuffers( PaAsiHpiStreamComponent *streamComp, HW32 pollingInterval,
                                      unsigned long framesPerPaHostBuffer, PaTime suggestedLatency );
static PaError PaAsiHpi_PrimeOutputWithSilence( PaAsiHpiStream *stream );
static PaError PaAsiHpi_StartStream( PaAsiHpiStream *stream, int outputPrimed );
static PaError PaAsiHpi_StopStream( PaAsiHpiStream *stream, int abort );
static PaError PaAsiHpi_ExplicitStop( PaAsiHpiStream *stream, int abort );
static void PaAsiHpi_OnThreadExit( void *userData );
static PaError PaAsiHpi_WaitForFrames( PaAsiHpiStream *stream, unsigned long *framesAvail,
                                       PaStreamCallbackFlags *cbFlags );
static void PaAsiHpi_CalculateTimeInfo( PaAsiHpiStream *stream, PaStreamCallbackTimeInfo *timeInfo );
static PaError PaAsiHpi_BeginProcessing( PaAsiHpiStream* stream, unsigned long* numFrames,
        PaStreamCallbackFlags *cbFlags );
static PaError PaAsiHpi_EndProcessing( PaAsiHpiStream *stream, unsigned long numFrames,
                                       PaStreamCallbackFlags *cbFlags );

/* ==========================================================================
 * ============================= IMPLEMENTATION =============================
 * ========================================================================== */

/* --------------------------- Host API Interface --------------------------- */

/** Enumerate all PA devices (= HPI streams).
 This compiles a list of all HPI adapters, and registers a PA device for each input and
 output stream it finds. Most errors are ignored, as missing or erroneous devices are
 simply skipped.

 @param hpiHostApi Pointer to HPI host API struct

 @return PortAudio error code (only paInsufficientMemory in practice)
 */
static PaError PaAsiHpi_BuildDeviceList( PaAsiHpiHostApiRepresentation *hpiHostApi )
{
    PaError result = paNoError;
    PaUtilHostApiRepresentation *hostApi = &hpiHostApi->baseHostApiRep;
    PaHostApiInfo *baseApiInfo = &hostApi->info;
    PaAsiHpiDeviceInfo *hpiDeviceList;
    HW16 adapterList[ HPI_MAX_ADAPTERS ];
    HW16 numAdapters;
    HW16 hpiError = 0;
    int i, j, deviceCount = 0, deviceIndex = 0;

    assert( hpiHostApi );
    assert( hpiHostApi->subSys );

    /* Look for adapters (not strictly necessary, as AdapterOpen can do the same, but this */
    /* way we have less errors since we do not try to open adapters we know aren't there) */
    /* Errors not considered critical here (subsystem may report 0 devices), but report them */
    /* in debug mode. */
    PA_ASIHPI_UNLESS_( HPI_SubSysFindAdapters( hpiHostApi->subSys, &numAdapters,
                       adapterList, HPI_MAX_ADAPTERS ), paNoError );

    /* First open and count the number of devices (= number of streams), to ease memory allocation */
    for( i=0; i < HPI_MAX_ADAPTERS; ++i )
    {
        HW16 inStreams, outStreams;
        HW16 version;
        HW32 serial;
        HW16 type;

        /* If no adapter found at this index, skip it */
        if( adapterList[i] == 0 )
            continue;

        /* Try to open adapter */
        hpiError = HPI_AdapterOpen( hpiHostApi->subSys, i );
        /* Report error and skip to next device on failure */
        if( hpiError )
        {
            PA_ASIHPI_REPORT_ERROR_( hpiError );
            continue;
        }
        hpiError = HPI_AdapterGetInfo( hpiHostApi->subSys, i,
                                       &outStreams, &inStreams, &version, &serial, &type );
        /* Skip to next device on failure */
        if( hpiError )
        {
            PA_ASIHPI_REPORT_ERROR_( hpiError );
            continue;
        }
        else
        {
            /* Assign default devices if available and increment device count */
            if( (baseApiInfo->defaultInputDevice == paNoDevice) && (inStreams > 0) )
                baseApiInfo->defaultInputDevice = deviceCount;
            deviceCount += inStreams;
            if( (baseApiInfo->defaultOutputDevice == paNoDevice) && (outStreams > 0) )
                baseApiInfo->defaultOutputDevice = deviceCount;
            deviceCount += outStreams;
        }
    }

    /* Register any discovered devices */
    if( deviceCount > 0 )
    {
        /* Memory allocation */
        PA_UNLESS_( hostApi->deviceInfos = (PaDeviceInfo**) PaUtil_GroupAllocateMemory(
                                               hpiHostApi->allocations, sizeof(PaDeviceInfo*) * deviceCount ),
                    paInsufficientMemory );
        /* Allocate all device info structs in a contiguous block */
        PA_UNLESS_( hpiDeviceList = (PaAsiHpiDeviceInfo*) PaUtil_GroupAllocateMemory(
                                        hpiHostApi->allocations, sizeof(PaAsiHpiDeviceInfo) * deviceCount ),
                    paInsufficientMemory );

        /* Now query devices again for information */
        for( i=0; i < HPI_MAX_ADAPTERS; ++i )
        {
            HW16 inStreams, outStreams;
            HW16 version;
            HW32 serial;
            HW16 type;

            /* If no adapter found at this index, skip it */
            if( adapterList[i] == 0 )
                continue;

            /* Assume adapter is still open from previous round */
            hpiError = HPI_AdapterGetInfo( hpiHostApi->subSys, i,
                                           &outStreams, &inStreams, &version, &serial, &type );
            /* Report error and skip to next device on failure */
            if( hpiError )
            {
                PA_ASIHPI_REPORT_ERROR_( hpiError );
                continue;
            }
            else
            {
                PA_DEBUG(( "Found HPI Adapter ID=%4X Idx=%d #In=%d #Out=%d S/N=%d HWver=%c%d DSPver=%03d\n",
                           type, i, inStreams, outStreams, serial,
                           ((version>>3)&0xf)+'A',                  /* Hw version major */
                           version&0x7,                             /* Hw version minor */
                           ((version>>13)*100)+((version>>7)&0x3f)  /* DSP code version */
                         ));
            }

            /* First add all input streams as devices */
            for( j=0; j < inStreams; ++j )
            {
                PaAsiHpiDeviceInfo *hpiDevice = &hpiDeviceList[deviceIndex];
                PaDeviceInfo *baseDeviceInfo = &hpiDevice->baseDeviceInfo;
                char srcName[72];
                char *deviceName;

                memset( hpiDevice, 0, sizeof(PaAsiHpiDeviceInfo) );
                /* Set implementation-specific device details */
                hpiDevice->subSys = hpiHostApi->subSys;
                hpiDevice->adapterIndex = i;
                hpiDevice->adapterType = type;
                hpiDevice->adapterVersion = version;
                hpiDevice->adapterSerialNumber = serial;
                hpiDevice->streamIndex = j;
                hpiDevice->streamIsOutput = 0;
                /* Set common PortAudio device stats */
                baseDeviceInfo->structVersion = 2;
                /* Make sure name string is owned by API info structure */
                sprintf( srcName,
                         "Adapter %d (%4X) - Input Stream %d", i+1, type, j+1 );
                PA_UNLESS_( deviceName = (char *) PaUtil_GroupAllocateMemory(
                                             hpiHostApi->allocations, strlen(srcName) + 1 ), paInsufficientMemory );
                strcpy( deviceName, srcName );
                baseDeviceInfo->name = deviceName;
                baseDeviceInfo->hostApi = hpiHostApi->hostApiIndex;
                baseDeviceInfo->maxInputChannels = HPI_MAX_CHANNELS;
                baseDeviceInfo->maxOutputChannels = 0;
                /* Default latency values for interactive performance */
                baseDeviceInfo->defaultLowInputLatency = 0.01;
                baseDeviceInfo->defaultLowOutputLatency = -1.0;
                /* Default latency values for robust non-interactive applications (eg. playing sound files) */
                baseDeviceInfo->defaultHighInputLatency = 0.2;
                baseDeviceInfo->defaultHighOutputLatency = -1.0;
                /* HPI interface can actually handle any sampling rate to 1 Hz accuracy,
                * so this default is as good as any */
                baseDeviceInfo->defaultSampleRate = 44100;

                /* Store device in global PortAudio list */
                hostApi->deviceInfos[deviceIndex++] = (PaDeviceInfo *) hpiDevice;
            }

            /* Now add all output streams as devices (I know, the repetition is painful) */
            for( j=0; j < outStreams; ++j )
            {
                PaAsiHpiDeviceInfo *hpiDevice = &hpiDeviceList[deviceIndex];
                PaDeviceInfo *baseDeviceInfo = &hpiDevice->baseDeviceInfo;
                char srcName[72];
                char *deviceName;

                memset( hpiDevice, 0, sizeof(PaAsiHpiDeviceInfo) );
                /* Set implementation-specific device details */
                hpiDevice->subSys = hpiHostApi->subSys;
                hpiDevice->adapterIndex = i;
                hpiDevice->adapterType = type;
                hpiDevice->adapterVersion = version;
                hpiDevice->adapterSerialNumber = serial;
                hpiDevice->streamIndex = j;
                hpiDevice->streamIsOutput = 1;
                /* Set common PortAudio device stats */
                baseDeviceInfo->structVersion = 2;
                /* Make sure name string is owned by API info structure */
                sprintf( srcName,
                         "Adapter %d (%4X) - Output Stream %d", i+1, type, j+1 );
                PA_UNLESS_( deviceName = (char *) PaUtil_GroupAllocateMemory(
                                             hpiHostApi->allocations, strlen(srcName) + 1 ), paInsufficientMemory );
                strcpy( deviceName, srcName );
                baseDeviceInfo->name = deviceName;
                baseDeviceInfo->hostApi = hpiHostApi->hostApiIndex;
                baseDeviceInfo->maxInputChannels = 0;
                baseDeviceInfo->maxOutputChannels = HPI_MAX_CHANNELS;
                /* Default latency values for interactive performance. */
                baseDeviceInfo->defaultLowInputLatency = -1.0;
                baseDeviceInfo->defaultLowOutputLatency = 0.01;
                /* Default latency values for robust non-interactive applications (eg. playing sound files). */
                baseDeviceInfo->defaultHighInputLatency = -1.0;
                baseDeviceInfo->defaultHighOutputLatency = 0.2;
                /* HPI interface can actually handle any sampling rate to 1 Hz accuracy,
                * so this default is as good as any */
                baseDeviceInfo->defaultSampleRate = 44100;

                /* Store device in global PortAudio list */
                hostApi->deviceInfos[deviceIndex++] = (PaDeviceInfo *) hpiDevice;
            }
        }
    }

    /* Finally acknowledge checked devices */
    baseApiInfo->deviceCount = deviceIndex;

error:
    return result;
}


/** Initialize host API implementation.
 This is the only function exported beyond this file. It is called by PortAudio to initialize
 the host API. It stores API info, finds and registers all devices, and sets up callback and
 blocking interfaces.

 @param hostApi Pointer to host API struct

 @param hostApiIndex Index of current (HPI) host API

 @return PortAudio error code
 */
PaError PaAsiHpi_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    PaAsiHpiHostApiRepresentation *hpiHostApi = NULL;
    PaHostApiInfo *baseApiInfo;

    /* Allocate host API structure */
    PA_UNLESS_( hpiHostApi = (PaAsiHpiHostApiRepresentation*) PaUtil_AllocateMemory(
                                 sizeof(PaAsiHpiHostApiRepresentation) ), paInsufficientMemory );
    PA_UNLESS_( hpiHostApi->allocations = PaUtil_CreateAllocationGroup(), paInsufficientMemory );

    hpiHostApi->hostApiIndex = hostApiIndex;
    hpiHostApi->subSys = NULL;

    /* Try to initialize HPI subsystem */
    if( ( hpiHostApi->subSys = HPI_SubSysCreate() ) == NULL)
    {
        /* the V19 development docs say that if an implementation
         * detects that it cannot be used, it should return a NULL
         * interface and paNoError */
        PA_DEBUG(( "Could not open HPI interface\n" ));
        result = paNoError;
        *hostApi = NULL;
        goto error;
    }
    else
    {
        HW32 hpiVersion;
        PA_ASIHPI_UNLESS_( HPI_SubSysGetVersion( hpiHostApi->subSys, &hpiVersion ), paUnanticipatedHostError );
        PA_DEBUG(( "HPI interface v%d.%02d\n",
                   hpiVersion >> 8, 10*((hpiVersion & 0xF0) >> 4) + (hpiVersion & 0x0F) ));
    }

    *hostApi = &hpiHostApi->baseHostApiRep;
    baseApiInfo = &((*hostApi)->info);
    /* Fill in common API details */
    baseApiInfo->structVersion = 1;
    baseApiInfo->type = paAudioScienceHPI;
    baseApiInfo->name = "AudioScience HPI";
    baseApiInfo->deviceCount = 0;
    baseApiInfo->defaultInputDevice = paNoDevice;
    baseApiInfo->defaultOutputDevice = paNoDevice;

    PA_ENSURE_( PaAsiHpi_BuildDeviceList( hpiHostApi ) );

    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &hpiHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &hpiHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );

    /* Store identity of main thread */
    PA_ENSURE_( PaUnixThreading_Initialize() );

    return result;
error:
    /* Clean up memory */
    Terminate( (PaUtilHostApiRepresentation *)hpiHostApi );
    return result;
}


/** Terminate host API implementation.
 This closes all HPI adapters and frees the HPI subsystem. It also frees the host API struct
 memory. It should be called once for every PaAsiHpi_Initialize call.

 @param Pointer to host API struct
 */
static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaAsiHpiHostApiRepresentation *hpiHostApi = (PaAsiHpiHostApiRepresentation*)hostApi;
    int i;
    PaError result = paNoError;

    if( hpiHostApi )
    {
        /* Get rid of HPI-specific structures */
        if( hpiHostApi->subSys )
        {
            HW16 lastAdapterIndex = HPI_MAX_ADAPTERS;
            /* Iterate through device list and close adapters */
            for( i=0; i < hostApi->info.deviceCount; ++i )
            {
                PaAsiHpiDeviceInfo *hpiDevice = (PaAsiHpiDeviceInfo *) hostApi->deviceInfos[ i ];
                /* Close adapter only if it differs from previous one */
                if( hpiDevice->adapterIndex != lastAdapterIndex )
                {
                    /* Ignore errors (report only during debugging) */
                    PA_ASIHPI_UNLESS_( HPI_AdapterClose( hpiHostApi->subSys,
                                                         hpiDevice->adapterIndex ), paNoError );
                    lastAdapterIndex = hpiDevice->adapterIndex;
                }
            }
            /* Finally dismantle HPI subsystem */
            HPI_SubSysFree( hpiHostApi->subSys );
        }

        if( hpiHostApi->allocations )
        {
            PaUtil_FreeAllAllocations( hpiHostApi->allocations );
            PaUtil_DestroyAllocationGroup( hpiHostApi->allocations );
        }

        PaUtil_FreeMemory( hpiHostApi );
    }
error:
    return;
}


/** Converts PortAudio sample format to equivalent HPI format.

 @param paFormat PortAudio sample format

 @return HPI sample format
 */
static HW16 PaAsiHpi_PaToHpiFormat( PaSampleFormat paFormat )
{
    /* Ignore interleaving flag */
    switch( paFormat & ~paNonInterleaved )
    {
    case paFloat32:
        return HPI_FORMAT_PCM32_FLOAT;

    case paInt32:
        return HPI_FORMAT_PCM32_SIGNED;

    case paInt24:
        return HPI_FORMAT_PCM24_SIGNED;

    case paInt16:
        return HPI_FORMAT_PCM16_SIGNED;

    case paUInt8:
        return HPI_FORMAT_PCM8_UNSIGNED;

        /* Default is 16-bit signed */
    case paInt8:
    default:
        return HPI_FORMAT_PCM16_SIGNED;
    }
}


/** Converts HPI sample format to equivalent PortAudio format.

 @param paFormat HPI sample format

 @return PortAudio sample format
 */
static PaSampleFormat PaAsiHpi_HpiToPaFormat( HW16 hpiFormat )
{
    switch( hpiFormat )
    {
    case HPI_FORMAT_PCM32_FLOAT:
        return paFloat32;

    case HPI_FORMAT_PCM32_SIGNED:
        return paInt32;

    case HPI_FORMAT_PCM24_SIGNED:
        return paInt24;

    case HPI_FORMAT_PCM16_SIGNED:
        return paInt16;

    case HPI_FORMAT_PCM8_UNSIGNED:
        return paUInt8;

        /* Default is custom format (e.g. for HPI MP3 format) */
    default:
        return paCustomFormat;
    }
}


/** Creates HPI format struct based on PortAudio parameters.
 This also does some checks to see whether the desired format is valid, and whether
 the device allows it. This only checks the format of one half (input or output) of the
 PortAudio stream.

 @param hostApi Pointer to host API struct

 @param parameters Pointer to stream parameter struct

 @param sampleRate Desired sample rate

 @param hpiDevice Pointer to HPI device struct

 @param hpiFormat Resulting HPI format returned here

 @return PortAudio error code (typically indicating a problem with stream format)
 */
static PaError PaAsiHpi_CreateFormat( struct PaUtilHostApiRepresentation *hostApi,
                                      const PaStreamParameters *parameters, double sampleRate,
                                      PaAsiHpiDeviceInfo **hpiDevice, HPI_FORMAT *hpiFormat )
{
    int maxChannelCount = 0;
    PaSampleFormat hostSampleFormat = 0;
    HW16 hpiError = 0;

    /* Unless alternate device specification is supported, reject the use of
       paUseHostApiSpecificDeviceSpecification */
    if( parameters->device == paUseHostApiSpecificDeviceSpecification )
        return paInvalidDevice;
    else
    {
        assert( parameters->device < hostApi->info.deviceCount );
        *hpiDevice = (PaAsiHpiDeviceInfo*) hostApi->deviceInfos[ parameters->device ];
    }

    /* Validate streamInfo - this implementation doesn't use custom stream info */
    if( parameters->hostApiSpecificStreamInfo )
        return paIncompatibleHostApiSpecificStreamInfo;

    /* Check that device can support channel count */
    if( (*hpiDevice)->streamIsOutput )
    {
        maxChannelCount = (*hpiDevice)->baseDeviceInfo.maxOutputChannels;
    }
    else
    {
        maxChannelCount = (*hpiDevice)->baseDeviceInfo.maxInputChannels;
    }
    if( (maxChannelCount == 0) || (parameters->channelCount > maxChannelCount) )
        return paInvalidChannelCount;

    /* All standard sample formats are supported by the buffer adapter,
       and this implementation doesn't support any custom sample formats */
    if( parameters->sampleFormat & paCustomFormat )
        return paSampleFormatNotSupported;

    /* Switch to closest HPI native format */
    hostSampleFormat = PaUtil_SelectClosestAvailableFormat(PA_ASIHPI_AVAILABLE_FORMATS_,
                       parameters->sampleFormat );
    /* Setup format + info objects */
    hpiError = HPI_FormatCreate( hpiFormat, (HW16)parameters->channelCount,
                                 PaAsiHpi_PaToHpiFormat( hostSampleFormat ),
                                 (HW32)sampleRate, 0, 0 );
    if( hpiError )
    {
        PA_ASIHPI_REPORT_ERROR_( hpiError );
        switch( hpiError )
        {
        case HPI_ERROR_INVALID_FORMAT:
            return paSampleFormatNotSupported;

        case HPI_ERROR_INVALID_SAMPLERATE:
        case HPI_ERROR_INCOMPATIBLE_SAMPLERATE:
            return paInvalidSampleRate;

        case HPI_ERROR_INVALID_CHANNELS:
            return paInvalidChannelCount;
        }
    }

    return paNoError;
}


/** Open HPI input stream with given format.
 This attempts to open HPI input stream with desired format. If the format is not supported
 or the device is unavailable, the stream is closed and a PortAudio error code is returned.

 @param hostApi Pointer to host API struct

 @param hpiDevice Pointer to HPI device struct

 @param hpiFormat Pointer to HPI format struct

 @return PortAudio error code (typically indicating a problem with stream format or device)
*/
static PaError PaAsiHpi_OpenInput( struct PaUtilHostApiRepresentation *hostApi,
                                   const PaAsiHpiDeviceInfo *hpiDevice, const HPI_FORMAT *hpiFormat,
                                   HPI_HISTREAM *hpiStream )
{
    PaAsiHpiHostApiRepresentation *hpiHostApi = (PaAsiHpiHostApiRepresentation*)hostApi;
    PaError result = paNoError;
    HW16 hpiError = 0;

    /* Catch misplaced output devices, as they typically have 0 input channels */
    PA_UNLESS_( !hpiDevice->streamIsOutput, paInvalidChannelCount );
    /* Try to open input stream */
    PA_ASIHPI_UNLESS_( HPI_InStreamOpen( hpiHostApi->subSys, hpiDevice->adapterIndex,
                                         hpiDevice->streamIndex, hpiStream ), paDeviceUnavailable );
    /* Set input format (checking it in the process) */
    /* Could also use HPI_InStreamQueryFormat, but this economizes the process */
    hpiError = HPI_InStreamSetFormat( hpiHostApi->subSys, *hpiStream, (HPI_FORMAT*)hpiFormat );
    if( hpiError )
    {
        PA_ASIHPI_REPORT_ERROR_( hpiError );
        PA_ASIHPI_UNLESS_( HPI_InStreamClose( hpiHostApi->subSys, *hpiStream ), paNoError );
        switch( hpiError )
        {
        case HPI_ERROR_INVALID_FORMAT:
            return paSampleFormatNotSupported;

        case HPI_ERROR_INVALID_SAMPLERATE:
        case HPI_ERROR_INCOMPATIBLE_SAMPLERATE:
            return paInvalidSampleRate;

        case HPI_ERROR_INVALID_CHANNELS:
            return paInvalidChannelCount;

        default:
            /* In case anything else went wrong */
            return paInvalidDevice;
        }
    }

error:
    return result;
}


/** Open HPI output stream with given format.
 This attempts to open HPI output stream with desired format. If the format is not supported
 or the device is unavailable, the stream is closed and a PortAudio error code is returned.

 @param hostApi Pointer to host API struct

 @param hpiDevice Pointer to HPI device struct

 @param hpiFormat Pointer to HPI format struct

 @return PortAudio error code (typically indicating a problem with stream format or device)
*/
static PaError PaAsiHpi_OpenOutput( struct PaUtilHostApiRepresentation *hostApi,
                                    const PaAsiHpiDeviceInfo *hpiDevice, const HPI_FORMAT *hpiFormat,
                                    HPI_HOSTREAM *hpiStream )
{
    PaAsiHpiHostApiRepresentation *hpiHostApi = (PaAsiHpiHostApiRepresentation*)hostApi;
    PaError result = paNoError;
    HW16 hpiError = 0;

    /* Catch misplaced input devices, as they typically have 0 output channels */
    PA_UNLESS_( hpiDevice->streamIsOutput, paInvalidChannelCount );
    /* Try to open output stream */
    PA_ASIHPI_UNLESS_( HPI_OutStreamOpen( hpiHostApi->subSys, hpiDevice->adapterIndex,
                                          hpiDevice->streamIndex, hpiStream ), paDeviceUnavailable );

    /* Check output format (format is set on first write to output stream) */
    hpiError = HPI_OutStreamQueryFormat( hpiHostApi->subSys, *hpiStream, (HPI_FORMAT*)hpiFormat );
    if( hpiError )
    {
        PA_ASIHPI_REPORT_ERROR_( hpiError );
        PA_ASIHPI_UNLESS_( HPI_OutStreamClose( hpiHostApi->subSys, *hpiStream ), paNoError );
        switch( hpiError )
        {
        case HPI_ERROR_INVALID_FORMAT:
            return paSampleFormatNotSupported;

        case HPI_ERROR_INVALID_SAMPLERATE:
        case HPI_ERROR_INCOMPATIBLE_SAMPLERATE:
            return paInvalidSampleRate;

        case HPI_ERROR_INVALID_CHANNELS:
            return paInvalidChannelCount;

        default:
            /* In case anything else went wrong */
            return paInvalidDevice;
        }
    }

error:
    return result;
}


/** Checks whether the desired stream formats and devices are supported
 (for both input and output).
 This is done by actually opening the appropriate HPI streams and closing them again.

 @param hostApi Pointer to host API struct

 @param inputParameters Pointer to stream parameter struct for input side of stream

 @param outputParameters Pointer to stream parameter struct for output side of stream

 @param sampleRate Desired sample rate

 @return PortAudio error code (paFormatIsSupported on success)
 */
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    PaError result = paFormatIsSupported;
    PaAsiHpiHostApiRepresentation *hpiHostApi = (PaAsiHpiHostApiRepresentation*)hostApi;
    PaAsiHpiDeviceInfo *hpiDevice = NULL;
    HPI_FORMAT hpiFormat;

    /* Input stream */
    if( inputParameters )
    {
        HPI_HISTREAM hpiStream;
        PA_DEBUG(( "%s: Checking input params: dev=%d, sr=%d, chans=%d, fmt=%d\n",
                   __FUNCTION__, inputParameters->device, (int)sampleRate,
                   inputParameters->channelCount, inputParameters->sampleFormat ));
        /* Create and validate format */
        PA_ENSURE_( PaAsiHpi_CreateFormat( hostApi, inputParameters, sampleRate,
                                           &hpiDevice, &hpiFormat ) );
        /* Open stream to further check format */
        PA_ENSURE_( PaAsiHpi_OpenInput( hostApi, hpiDevice, &hpiFormat, &hpiStream ) );
        /* Close stream again */
        PA_ASIHPI_UNLESS_( HPI_InStreamClose( hpiHostApi->subSys, hpiStream ), paNoError );
    }

    /* Output stream */
    if( outputParameters )
    {
        HPI_HOSTREAM hpiStream;
        PA_DEBUG(( "%s: Checking output params: dev=%d, sr=%d, chans=%d, fmt=%d\n",
                   __FUNCTION__, outputParameters->device, (int)sampleRate,
                   outputParameters->channelCount, outputParameters->sampleFormat ));
        /* Create and validate format */
        PA_ENSURE_( PaAsiHpi_CreateFormat( hostApi, outputParameters, sampleRate,
                                           &hpiDevice, &hpiFormat ) );
        /* Open stream to further check format */
        PA_ENSURE_( PaAsiHpi_OpenOutput( hostApi, hpiDevice, &hpiFormat, &hpiStream ) );
        /* Close stream again */
        PA_ASIHPI_UNLESS_( HPI_OutStreamClose( hpiHostApi->subSys, hpiStream ), paNoError );
    }

error:
    return result;
}

/* ---------------------------- Stream Interface ---------------------------- */

/** Obtain HPI stream information.
 This obtains info such as stream state and available data/space in buffers. It also
 estimates whether an underflow or overflow occurred.

 @param streamComp Pointer to stream component (input or output) to query

 @param info Pointer to stream info struct that will contain result

 @return PortAudio error code (either paNoError, paDeviceUnavailable or paUnanticipatedHostError)
 */
static PaError PaAsiHpi_GetStreamInfo( PaAsiHpiStreamComponent *streamComp, PaAsiHpiStreamInfo *info )
{
    PaError result = paDeviceUnavailable;
    HW16 state;
    HW32 bufferSize, dataSize, frameCounter, auxDataSize, threshold;
    HW32 hwBufferSize, hwDataSize;

    assert( streamComp );
    assert( info );

    /* First blank the stream info struct, in case something goes wrong below.
       This saves the caller from initializing the struct. */
    info->state = 0;
    info->bufferSize = 0;
    info->dataSize = 0;
    info->frameCounter = 0;
    info->auxDataSize = 0;
    info->totalBufferedData = 0;
    info->availableFrames = 0;
    info->underflow = 0;
    info->overflow = 0;

    if( streamComp->hpiDevice && streamComp->hpiStream )
    {
        /* Obtain detailed stream info (either input or output) */
        if( streamComp->hpiDevice->streamIsOutput )
        {
            PA_ASIHPI_UNLESS_( HPI_OutStreamGetInfoEx( streamComp->hpiDevice->subSys,
                               streamComp->hpiStream,
                               &state, &bufferSize, &dataSize, &frameCounter,
                               &auxDataSize ), paUnanticipatedHostError );
        }
        else
        {
            PA_ASIHPI_UNLESS_( HPI_InStreamGetInfoEx( streamComp->hpiDevice->subSys,
                               streamComp->hpiStream,
                               &state, &bufferSize, &dataSize, &frameCounter,
                               &auxDataSize ), paUnanticipatedHostError );
        }
        /* Load stream info */
        info->state = state;
        info->bufferSize = bufferSize;
        info->dataSize = dataSize;
        info->frameCounter = frameCounter;
        info->auxDataSize = auxDataSize;
        /* Determine total buffered data */
        info->totalBufferedData = dataSize;
        if( streamComp->hostBufferSize > 0 )
            info->totalBufferedData += auxDataSize;
        info->totalBufferedData /= streamComp->bytesPerFrame;
        /* Determine immediately available frames */
        info->availableFrames = streamComp->hpiDevice->streamIsOutput ?
                                bufferSize - dataSize : dataSize;
        info->availableFrames /= streamComp->bytesPerFrame;
        /* Minimum space/data required in buffers */
        threshold = PA_MIN( streamComp->tempBufferSize,
                            streamComp->bytesPerFrame * PA_ASIHPI_MIN_FRAMES_ );
        /* Obtain hardware buffer stats first, to simplify things */
        hwBufferSize = streamComp->hardwareBufferSize;
        hwDataSize = streamComp->hostBufferSize > 0 ? auxDataSize : dataSize;
        /* Underflow is a bit tricky */
        info->underflow = streamComp->hpiDevice->streamIsOutput ?
                          /* Stream seems to start in drained state sometimes, so ignore initial underflow */
                          (frameCounter > 0) && ( (state == HPI_STATE_DRAINED) || (hwDataSize == 0) ) :
                          /* Input streams check the first-level (host) buffer for underflow */
                          (state != HPI_STATE_STOPPED) && (dataSize < threshold);
        /* Check for overflow in second-level (hardware) buffer for both input and output */
        info->overflow = (state != HPI_STATE_STOPPED) && (hwBufferSize - hwDataSize < threshold);

        return paNoError;
    }

error:
    return result;
}


/** Display stream component information for debugging purposes.

 @param streamComp Pointer to stream component (input or output) to query

 @param stream Pointer to stream struct which contains the component above
 */
static void PaAsiHpi_StreamComponentDump( PaAsiHpiStreamComponent *streamComp,
        PaAsiHpiStream *stream )
{
    PaAsiHpiStreamInfo streamInfo;

    assert( streamComp );
    assert( stream );

    /* Name of soundcard/device used by component */
    PA_DEBUG(( "device: %s\n", streamComp->hpiDevice->baseDeviceInfo.name ));
    /* Unfortunately some overlap between input and output here */
    if( streamComp->hpiDevice->streamIsOutput )
    {
        /* Settings on the user side (as experienced by user callback) */
        PA_DEBUG(( "user: %d-bit, %d ",
                   8*stream->bufferProcessor.bytesPerUserOutputSample,
                   stream->bufferProcessor.outputChannelCount));
        if( stream->bufferProcessor.userOutputIsInterleaved )
        {
            PA_DEBUG(( "interleaved channels, " ));
        }
        else
        {
            PA_DEBUG(( "non-interleaved channels, " ));
        }
        PA_DEBUG(( "%d frames/buffer, latency = %5.1f ms\n",
                   stream->bufferProcessor.framesPerUserBuffer,
                   1000*stream->baseStreamRep.streamInfo.outputLatency ));
        /* Settings on the host side (internal to PortAudio host API) */
        PA_DEBUG(( "host: %d-bit, %d interleaved channels, %d frames/buffer ",
                   8*stream->bufferProcessor.bytesPerHostOutputSample,
                   stream->bufferProcessor.outputChannelCount,
                   stream->bufferProcessor.framesPerHostBuffer ));
    }
    else
    {
        /* Settings on the user side (as experienced by user callback) */
        PA_DEBUG(( "user: %d-bit, %d ",
                   8*stream->bufferProcessor.bytesPerUserInputSample,
                   stream->bufferProcessor.inputChannelCount));
        if( stream->bufferProcessor.userInputIsInterleaved )
        {
            PA_DEBUG(( "interleaved channels, " ));
        }
        else
        {
            PA_DEBUG(( "non-interleaved channels, " ));
        }
        PA_DEBUG(( "%d frames/buffer, latency = %5.1f ms\n",
                   stream->bufferProcessor.framesPerUserBuffer,
                   1000*stream->baseStreamRep.streamInfo.inputLatency ));
        /* Settings on the host side (internal to PortAudio host API) */
        PA_DEBUG(( "host: %d-bit, %d interleaved channels, %d frames/buffer ",
                   8*stream->bufferProcessor.bytesPerHostInputSample,
                   stream->bufferProcessor.inputChannelCount,
                   stream->bufferProcessor.framesPerHostBuffer ));
    }
    switch( stream->bufferProcessor.hostBufferSizeMode )
    {
    case paUtilFixedHostBufferSize:
        PA_DEBUG(( "[fixed] " ));
        break;
    case paUtilBoundedHostBufferSize:
        PA_DEBUG(( "[bounded] " ));
        break;
    case paUtilUnknownHostBufferSize:
        PA_DEBUG(( "[unknown] " ));
        break;
    case paUtilVariableHostBufferSizePartialUsageAllowed:
        PA_DEBUG(( "[variable] " ));
        break;
    }
    PA_DEBUG(( "(%d max)\n", streamComp->tempBufferSize / streamComp->bytesPerFrame ));
    /* HPI hardware settings */
    PA_DEBUG(( "HPI: adapter %d stream %d, %d-bit, %d-channel, %d Hz\n",
               streamComp->hpiDevice->adapterIndex, streamComp->hpiDevice->streamIndex,
               8 * streamComp->bytesPerFrame / streamComp->hpiFormat.wChannels,
               streamComp->hpiFormat.wChannels,
               streamComp->hpiFormat.dwSampleRate ));
    /* Stream state and buffer levels */
    PA_DEBUG(( "HPI: " ));
    PaAsiHpi_GetStreamInfo( streamComp, &streamInfo );
    switch( streamInfo.state )
    {
    case HPI_STATE_STOPPED:
        PA_DEBUG(( "[STOPPED] " ));
        break;
    case HPI_STATE_PLAYING:
        PA_DEBUG(( "[PLAYING] " ));
        break;
    case HPI_STATE_RECORDING:
        PA_DEBUG(( "[RECORDING] " ));
        break;
    case HPI_STATE_DRAINED:
        PA_DEBUG(( "[DRAINED] " ));
        break;
    default:
        PA_DEBUG(( "[unknown state] " ));
        break;
    }
    if( streamComp->hostBufferSize )
    {
        PA_DEBUG(( "host = %d/%d B, ", streamInfo.dataSize, streamComp->hostBufferSize ));
        PA_DEBUG(( "hw = %d/%d (%d) B, ", streamInfo.auxDataSize,
                   streamComp->hardwareBufferSize, streamComp->outputBufferCap ));
    }
    else
    {
        PA_DEBUG(( "hw = %d/%d B, ", streamInfo.dataSize, streamComp->hardwareBufferSize ));
    }
    PA_DEBUG(( "count = %d", streamInfo.frameCounter ));
    if( streamInfo.overflow )
    {
        PA_DEBUG(( " [overflow]" ));
    }
    else if( streamInfo.underflow )
    {
        PA_DEBUG(( " [underflow]" ));
    }
    PA_DEBUG(( "\n" ));
}


/** Display stream information for debugging purposes.

 @param stream Pointer to stream to query
 */
static void PaAsiHpi_StreamDump( PaAsiHpiStream *stream )
{
    assert( stream );

    PA_DEBUG(( "\n------------------------- STREAM INFO FOR %p ---------------------------\n", stream ));
    /* General stream info (input+output) */
    if( stream->baseStreamRep.streamCallback )
    {
        PA_DEBUG(( "[callback] " ));
    }
    else
    {
        PA_DEBUG(( "[blocking] " ));
    }
    PA_DEBUG(( "sr=%d Hz, poll=%d ms, max %d frames/buf ",
               (int)stream->baseStreamRep.streamInfo.sampleRate,
               stream->pollingInterval, stream->maxFramesPerHostBuffer ));
    switch( stream->state )
    {
    case paAsiHpiStoppedState:
        PA_DEBUG(( "[stopped]\n" ));
        break;
    case paAsiHpiActiveState:
        PA_DEBUG(( "[active]\n" ));
        break;
    case paAsiHpiCallbackFinishedState:
        PA_DEBUG(( "[cb fin]\n" ));
        break;
    default:
        PA_DEBUG(( "[unknown state]\n" ));
        break;
    }
    if( stream->callbackMode )
    {
        PA_DEBUG(( "cb info: thread=%p, cbAbort=%d, cbFinished=%d\n",
                   stream->thread.thread, stream->callbackAbort, stream->callbackFinished ));
    }

    PA_DEBUG(( "----------------------------------- Input  ------------------------------------\n" ));
    if( stream->input )
    {
        PaAsiHpi_StreamComponentDump( stream->input, stream );
    }
    else
    {
        PA_DEBUG(( "*none*\n" ));
    }

    PA_DEBUG(( "----------------------------------- Output ------------------------------------\n" ));
    if( stream->output )
    {
        PaAsiHpi_StreamComponentDump( stream->output, stream );
    }
    else
    {
        PA_DEBUG(( "*none*\n" ));
    }
    PA_DEBUG(( "-------------------------------------------------------------------------------\n\n" ));

}


/** Determine buffer sizes and allocate appropriate stream buffers.
 This attempts to allocate a BBM (host) buffer for the HPI stream component (either input
 or output, as both have similar buffer needs). Not all AudioScience adapters support BBM,
 in which case the hardware buffer has to suffice. The size of the HPI host buffer is chosen
 as a multiple of framesPerPaHostBuffer, and also influenced by the suggested latency and the
 estimated minimum polling interval. The HPI host and hardware buffer sizes are stored, and an
 appropriate cap for the hardware buffer is also calculated. Finally, the temporary stream
 buffer which serves as the PortAudio host buffer for this implementation is allocated.
 This buffer contains an integer number of user buffers, to simplify buffer adaption in the
 buffer processor. The function returns paBufferTooBig if the HPI interface cannot allocate
 an HPI host buffer of the desired size.

 @param streamComp Pointer to stream component struct

 @param pollingInterval Polling interval for stream, in milliseconds

 @param framesPerPaHostBuffer Size of PortAudio host buffer, in frames

 @param suggestedLatency Suggested latency for stream component, in seconds

 @return PortAudio error code (possibly paBufferTooBig or paInsufficientMemory)
 */
static PaError PaAsiHpi_SetupBuffers( PaAsiHpiStreamComponent *streamComp, HW32 pollingInterval,
                                      unsigned long framesPerPaHostBuffer, PaTime suggestedLatency )
{
    PaError result = paNoError;
    PaAsiHpiStreamInfo streamInfo;
    unsigned long hpiBufferSize = 0, paHostBufferSize = 0;

    assert( streamComp );
    assert( streamComp->hpiDevice );

    /* Obtain size of hardware buffer of HPI stream, since we will be activating BBM shortly
       and afterwards the buffer size will refer to the BBM (host-side) buffer.
       This is necessary to enable reliable detection of xruns. */
    PA_ENSURE_( PaAsiHpi_GetStreamInfo( streamComp, &streamInfo ) );
    streamComp->hardwareBufferSize = streamInfo.bufferSize;
    hpiBufferSize = streamInfo.bufferSize;

    /* Check if BBM (background bus mastering) is to be enabled */
    if( PA_ASIHPI_USE_BBM_ )
    {
        HW32 bbmBufferSize = 0, preLatencyBufferSize = 0;
        HW16 hpiError = 0;
        PaTime pollingOverhead;

        /* Check overhead of Pa_Sleep() call (minimum sleep duration in ms -> OS dependent) */
        pollingOverhead = PaUtil_GetTime();
        Pa_Sleep( 0 );
        pollingOverhead = 1000*(PaUtil_GetTime() - pollingOverhead);
        PA_DEBUG(( "polling overhead = %f ms (length of 0-second sleep)\n", pollingOverhead ));
        /* Obtain minimum recommended size for host buffer (in bytes) */
        PA_ASIHPI_UNLESS_( HPI_StreamEstimateBufferSize( &streamComp->hpiFormat,
                           pollingInterval + (HW32)ceil( pollingOverhead ),
                           &bbmBufferSize ), paUnanticipatedHostError );
        /* BBM places more stringent requirements on buffer size (see description */
        /* of HPI_StreamEstimateBufferSize in HPI API document) */
        bbmBufferSize *= 3;
        /* Make sure the BBM buffer contains multiple PA host buffers */
        if( bbmBufferSize < 3 * streamComp->bytesPerFrame * framesPerPaHostBuffer )
            bbmBufferSize = 3 * streamComp->bytesPerFrame * framesPerPaHostBuffer;
        /* Try to honor latency suggested by user by growing buffer (no decrease possible) */
        if( suggestedLatency > 0.0 )
        {
            PaTime bufferDuration = ((PaTime)bbmBufferSize) / streamComp->bytesPerFrame
                                    / streamComp->hpiFormat.dwSampleRate;
            /* Don't decrease buffer */
            if( bufferDuration < suggestedLatency )
            {
                /* Save old buffer size, to be retried if new size proves too big */
                preLatencyBufferSize = bbmBufferSize;
                bbmBufferSize = (HW32)ceil( suggestedLatency * streamComp->bytesPerFrame
                                            * streamComp->hpiFormat.dwSampleRate );
            }
        }
        /* Choose closest memory block boundary (HPI API document states that
        "a buffer size of Nx4096 - 20 makes the best use of memory"
        (under the entry for HPI_StreamEstimateBufferSize)) */
        bbmBufferSize = ((HW32)ceil((bbmBufferSize + 20)/4096.0))*4096 - 20;
        streamComp->hostBufferSize = bbmBufferSize;
        /* Allocate BBM host buffer (this enables bus mastering transfers in background) */
        if( streamComp->hpiDevice->streamIsOutput )
            hpiError = HPI_OutStreamHostBufferAllocate( streamComp->hpiDevice->subSys,
                       streamComp->hpiStream,
                       bbmBufferSize );
        else
            hpiError = HPI_InStreamHostBufferAllocate( streamComp->hpiDevice->subSys,
                       streamComp->hpiStream,
                       bbmBufferSize );
        if( hpiError )
        {
            PA_ASIHPI_REPORT_ERROR_( hpiError );
            /* Indicate that BBM is disabled */
            streamComp->hostBufferSize = 0;
            /* Retry with smaller buffer size (transfers will still work, but not via BBM) */
            if( hpiError == HPI_ERROR_INVALID_DATASIZE )
            {
                /* Retry BBM allocation with smaller size if requested latency proved too big */
                if( preLatencyBufferSize > 0 )
                {
                    PA_DEBUG(( "Retrying BBM allocation with smaller size (%d vs. %d bytes)\n",
                               preLatencyBufferSize, bbmBufferSize ));
                    bbmBufferSize = preLatencyBufferSize;
                    if( streamComp->hpiDevice->streamIsOutput )
                        hpiError = HPI_OutStreamHostBufferAllocate( streamComp->hpiDevice->subSys,
                                   streamComp->hpiStream,
                                   bbmBufferSize );
                    else
                        hpiError = HPI_InStreamHostBufferAllocate( streamComp->hpiDevice->subSys,
                                   streamComp->hpiStream,
                                   bbmBufferSize );
                    /* Another round of error checking */
                    if( hpiError )
                    {
                        PA_ASIHPI_REPORT_ERROR_( hpiError );
                        /* No escapes this time */
                        if( hpiError == HPI_ERROR_INVALID_DATASIZE )
                        {
                            result = paBufferTooBig;
                            goto error;
                        }
                        else if( hpiError != HPI_ERROR_INVALID_OPERATION )
                        {
                            result = paUnanticipatedHostError;
                            goto error;
                        }
                    }
                    else
                    {
                        streamComp->hostBufferSize = bbmBufferSize;
                        hpiBufferSize = bbmBufferSize;
                    }
                }
                else
                {
                    result = paBufferTooBig;
                    goto error;
                }
            }
            /* If BBM not supported, foreground transfers will be used, but not a show-stopper */
            /* Anything else is an error */
            else if( hpiError != HPI_ERROR_INVALID_OPERATION )
            {
                result = paUnanticipatedHostError;
                goto error;
            }
        }
        else
        {
            hpiBufferSize = bbmBufferSize;
        }
    }

    /* Final check of buffer size */
    paHostBufferSize = streamComp->bytesPerFrame * framesPerPaHostBuffer;
    if( hpiBufferSize < 3*paHostBufferSize )
    {
        result = paBufferTooBig;
        goto error;
    }
    /* Set cap on output buffer size, based on latency suggestions */
    if( streamComp->hpiDevice->streamIsOutput )
    {
        PaTime latency = suggestedLatency > 0.0 ? suggestedLatency :
                         streamComp->hpiDevice->baseDeviceInfo.defaultHighOutputLatency;
        streamComp->outputBufferCap =
            (HW32)ceil( latency * streamComp->bytesPerFrame * streamComp->hpiFormat.dwSampleRate );
        /* The cap should not be too small, to prevent underflow */
        if( streamComp->outputBufferCap < 4*paHostBufferSize )
            streamComp->outputBufferCap = 4*paHostBufferSize;
    }
    else
    {
        streamComp->outputBufferCap = 0;
    }
    /* Temp buffer size should be multiple of PA host buffer size (or 1x, if using fixed blocks) */
    streamComp->tempBufferSize = paHostBufferSize;
    /* Allocate temp buffer */
    PA_UNLESS_( streamComp->tempBuffer = (HW8 *)PaUtil_AllocateMemory( streamComp->tempBufferSize ),
                paInsufficientMemory );
error:
    return result;
}


/** Opens PortAudio stream.
 This determines a suitable value for framesPerBuffer, if the user didn't specify it,
 based on the suggested latency. It then opens each requested stream direction with the
 appropriate stream format, and allocates the required stream buffers. It sets up the
 various PortAudio structures dealing with streams, and estimates the stream latency.

 See pa_hostapi.h for a list of validity guarantees made about OpenStream parameters.

 @param hostApi Pointer to host API struct

 @param s List of open streams, where successfully opened stream will go

 @param inputParameters Pointer to stream parameter struct for input side of stream

 @param outputParameters Pointer to stream parameter struct for output side of stream

 @param sampleRate Desired sample rate

 @param framesPerBuffer Desired number of frames per buffer passed to user callback
                        (or chunk size for blocking stream)

 @param streamFlags Stream flags

 @param streamCallback Pointer to user callback function (zero for blocking interface)

 @param userData Pointer to user data that will be passed to callback function along with data

 @return PortAudio error code
*/
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream **s,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData )
{
    PaError result = paNoError;
    PaAsiHpiHostApiRepresentation *hpiHostApi = (PaAsiHpiHostApiRepresentation*)hostApi;
    PaAsiHpiStream *stream = NULL;
    unsigned long framesPerHostBuffer = framesPerBuffer;
    int inputChannelCount = 0, outputChannelCount = 0;
    PaSampleFormat inputSampleFormat = 0, outputSampleFormat = 0;
    PaSampleFormat hostInputSampleFormat = 0, hostOutputSampleFormat = 0;
    PaTime maxSuggestedLatency = 0.0;

    /* Validate platform-specific flags -> none expected for HPI */
    if( (streamFlags & paPlatformSpecificFlags) != 0 )
        return paInvalidFlag; /* unexpected platform-specific flag */

    /* Create blank stream structure */
    PA_UNLESS_( stream = (PaAsiHpiStream *)PaUtil_AllocateMemory( sizeof(PaAsiHpiStream) ),
                paInsufficientMemory );
    memset( stream, 0, sizeof(PaAsiHpiStream) );

    /* If the number of frames per buffer is unspecified, we have to come up with one. */
    if( framesPerHostBuffer == paFramesPerBufferUnspecified )
    {
        if( inputParameters )
            maxSuggestedLatency = inputParameters->suggestedLatency;
        if( outputParameters && (outputParameters->suggestedLatency > maxSuggestedLatency) )
            maxSuggestedLatency = outputParameters->suggestedLatency;
        /* Use suggested latency if available */
        if( maxSuggestedLatency > 0.0 )
            framesPerHostBuffer = (unsigned long)ceil( maxSuggestedLatency * sampleRate );
        else
            /* AudioScience cards like BIG buffers by default */
            framesPerHostBuffer = 4096;
    }
    /* Lower bounds on host buffer size, due to polling and HPI constraints */
    if( 1000.0*framesPerHostBuffer/sampleRate < PA_ASIHPI_MIN_POLLING_INTERVAL_ )
        framesPerHostBuffer = (unsigned long)ceil( sampleRate * PA_ASIHPI_MIN_POLLING_INTERVAL_ / 1000.0 );
    /*    if( framesPerHostBuffer < PA_ASIHPI_MIN_FRAMES_ )
            framesPerHostBuffer = PA_ASIHPI_MIN_FRAMES_; */
    /* Efficient if host buffer size is integer multiple of user buffer size */
    if( framesPerBuffer > 0 )
        framesPerHostBuffer = (unsigned long)ceil( (double)framesPerHostBuffer / framesPerBuffer ) * framesPerBuffer;
    /* Buffer should always be a multiple of 4 bytes to facilitate 32-bit PCI transfers.
     By keeping the frames a multiple of 4, this is ensured even for 8-bit mono sound. */
    framesPerHostBuffer = (framesPerHostBuffer / 4) * 4;
    /* Polling is based on time length (in milliseconds) of user-requested block size */
    stream->pollingInterval = (HW32)ceil( 1000.0*framesPerHostBuffer/sampleRate );
    assert( framesPerHostBuffer > 0 );

    /* Open underlying streams, check formats and allocate buffers */
    if( inputParameters )
    {
        /* Create blank stream component structure */
        PA_UNLESS_( stream->input = (PaAsiHpiStreamComponent *)PaUtil_AllocateMemory( sizeof(PaAsiHpiStreamComponent) ),
                    paInsufficientMemory );
        memset( stream->input, 0, sizeof(PaAsiHpiStreamComponent) );
        /* Create/validate format */
        PA_ENSURE_( PaAsiHpi_CreateFormat( hostApi, inputParameters, sampleRate,
                                           &stream->input->hpiDevice, &stream->input->hpiFormat ) );
        /* Open stream and set format */
        PA_ENSURE_( PaAsiHpi_OpenInput( hostApi, stream->input->hpiDevice, &stream->input->hpiFormat,
                                        &stream->input->hpiStream ) );
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;
        hostInputSampleFormat = PaAsiHpi_HpiToPaFormat( stream->input->hpiFormat.wFormat );
        stream->input->bytesPerFrame = inputChannelCount * Pa_GetSampleSize( hostInputSampleFormat );
        assert( stream->input->bytesPerFrame > 0 );
        /* Allocate host and temp buffers of appropriate size */
        PA_ENSURE_( PaAsiHpi_SetupBuffers( stream->input, stream->pollingInterval,
                                           framesPerHostBuffer, inputParameters->suggestedLatency ) );
    }
    if( outputParameters )
    {
        /* Create blank stream component structure */
        PA_UNLESS_( stream->output = (PaAsiHpiStreamComponent *)PaUtil_AllocateMemory( sizeof(PaAsiHpiStreamComponent) ),
                    paInsufficientMemory );
        memset( stream->output, 0, sizeof(PaAsiHpiStreamComponent) );
        /* Create/validate format */
        PA_ENSURE_( PaAsiHpi_CreateFormat( hostApi, outputParameters, sampleRate,
                                           &stream->output->hpiDevice, &stream->output->hpiFormat ) );
        /* Open stream and check format */
        PA_ENSURE_( PaAsiHpi_OpenOutput( hostApi, stream->output->hpiDevice,
                                         &stream->output->hpiFormat,
                                         &stream->output->hpiStream ) );
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
        hostOutputSampleFormat = PaAsiHpi_HpiToPaFormat( stream->output->hpiFormat.wFormat );
        stream->output->bytesPerFrame = outputChannelCount * Pa_GetSampleSize( hostOutputSampleFormat );
        /* Allocate host and temp buffers of appropriate size */
        PA_ENSURE_( PaAsiHpi_SetupBuffers( stream->output, stream->pollingInterval,
                                           framesPerHostBuffer, outputParameters->suggestedLatency ) );
    }

    /* Determine maximum frames per host buffer (least common denominator of input/output) */
    if( inputParameters && outputParameters )
    {
        stream->maxFramesPerHostBuffer = PA_MIN( stream->input->tempBufferSize / stream->input->bytesPerFrame,
                                         stream->output->tempBufferSize / stream->output->bytesPerFrame );
    }
    else
    {
        stream->maxFramesPerHostBuffer = inputParameters ? stream->input->tempBufferSize / stream->input->bytesPerFrame
                                         : stream->output->tempBufferSize / stream->output->bytesPerFrame;
    }
    assert( stream->maxFramesPerHostBuffer > 0 );
    /* Initialize various other stream parameters */
    stream->neverDropInput = streamFlags & paNeverDropInput;
    stream->state = paAsiHpiStoppedState;

    /* Initialize either callback or blocking interface */
    if( streamCallback )
    {
        PaUtil_InitializeStreamRepresentation( &stream->baseStreamRep,
                                               &hpiHostApi->callbackStreamInterface,
                                               streamCallback, userData );
        stream->callbackMode = 1;
    }
    else
    {
        PaUtil_InitializeStreamRepresentation( &stream->baseStreamRep,
                                               &hpiHostApi->blockingStreamInterface,
                                               streamCallback, userData );
        /* Pre-allocate non-interleaved user buffer pointers for blocking interface */
        PA_UNLESS_( stream->blockingUserBufferCopy =
                        PaUtil_AllocateMemory( sizeof(void *) * PA_MAX( inputChannelCount, outputChannelCount ) ),
                    paInsufficientMemory );
        stream->callbackMode = 0;
    }
    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, sampleRate );

    /* Following pa_linux_alsa's lead, we operate with fixed host buffer size by default, */
    /* since other modes will invariably lead to block adaption (maybe Bounded better?) */
    PA_ENSURE_( PaUtil_InitializeBufferProcessor( &stream->bufferProcessor,
                inputChannelCount, inputSampleFormat, hostInputSampleFormat,
                outputChannelCount, outputSampleFormat, hostOutputSampleFormat,
                sampleRate, streamFlags,
                framesPerBuffer, framesPerHostBuffer, paUtilFixedHostBufferSize,
                streamCallback, userData ) );

    stream->baseStreamRep.streamInfo.structVersion = 1;
    stream->baseStreamRep.streamInfo.sampleRate = sampleRate;
    /* Determine input latency from buffer processor and buffer sizes */
    if( stream->input )
    {
        PaTime bufferDuration = ( stream->input->hostBufferSize + stream->input->hardwareBufferSize )
                                / sampleRate / stream->input->bytesPerFrame;
        stream->baseStreamRep.streamInfo.inputLatency =
            PaUtil_GetBufferProcessorInputLatency( &stream->bufferProcessor ) +
            bufferDuration - stream->maxFramesPerHostBuffer / sampleRate;
        assert( stream->baseStreamRep.streamInfo.inputLatency > 0.0 );
    }
    /* Determine output latency from buffer processor and buffer sizes */
    if( stream->output )
    {
        PaTime bufferDuration = ( stream->output->hostBufferSize + stream->output->hardwareBufferSize )
                                / sampleRate / stream->output->bytesPerFrame;
        /* Take buffer size cap into account (see PaAsiHpi_WaitForFrames) */
        if( !stream->input && (stream->output->outputBufferCap > 0) )
        {
            bufferDuration = PA_MIN( bufferDuration,
                                     stream->output->outputBufferCap / sampleRate / stream->output->bytesPerFrame );
        }
        stream->baseStreamRep.streamInfo.outputLatency =
            PaUtil_GetBufferProcessorOutputLatency( &stream->bufferProcessor ) +
            bufferDuration - stream->maxFramesPerHostBuffer / sampleRate;
        assert( stream->baseStreamRep.streamInfo.outputLatency > 0.0 );
    }

    /* Report stream info, for debugging purposes */
    PaAsiHpi_StreamDump( stream );

    /* Save initialized stream to PA stream list */
    *s = (PaStream*)stream;
    return result;

error:
    CloseStream( (PaStream*)stream );
    return result;
}


/** Close PortAudio stream.
 When CloseStream() is called, the multi-api layer ensures that the stream has already
 been stopped or aborted. This closes the underlying HPI streams and deallocates stream
 buffers and structs.

 @param s Pointer to PortAudio stream

 @return PortAudio error code
*/
static PaError CloseStream( PaStream *s )
{
    PaError result = paNoError;
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;

    /* If stream is already gone, all is well */
    if( stream == NULL )
        return paNoError;

    /* Generic stream cleanup */
    PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );
    PaUtil_TerminateStreamRepresentation( &stream->baseStreamRep );

    /* Implementation-specific details - close internal streams */
    if( stream->input )
    {
        /* Close HPI stream (freeing BBM host buffer in the process, if used) */
        if( stream->input->hpiStream )
        {
            PA_ASIHPI_UNLESS_( HPI_InStreamClose( stream->input->hpiDevice->subSys,
                                                  stream->input->hpiStream ), paUnanticipatedHostError );
        }
        /* Free temp buffer and stream component */
        PaUtil_FreeMemory( stream->input->tempBuffer );
        PaUtil_FreeMemory( stream->input );
    }
    if( stream->output )
    {
        /* Close HPI stream (freeing BBM host buffer in the process, if used) */
        if( stream->output->hpiStream )
        {
            PA_ASIHPI_UNLESS_( HPI_OutStreamClose( stream->output->hpiDevice->subSys,
                                                   stream->output->hpiStream ), paUnanticipatedHostError );
        }
        /* Free temp buffer and stream component */
        PaUtil_FreeMemory( stream->output->tempBuffer );
        PaUtil_FreeMemory( stream->output );
    }

    PaUtil_FreeMemory( stream->blockingUserBufferCopy );
    PaUtil_FreeMemory( stream );

error:
    return result;
}


/** Prime HPI output stream with silence.
 This resets the output stream and uses PortAudio helper routines to fill the
 temp buffer with silence. It then writes two host buffers to the stream. This is supposed
 to be called before the stream is started. It has no effect on input-only streams.

 @param stream Pointer to stream struct

 @return PortAudio error code
 */
static PaError PaAsiHpi_PrimeOutputWithSilence( PaAsiHpiStream *stream )
{
    PaError result = paNoError;
    PaAsiHpiStreamComponent *out;
    PaUtilZeroer *zeroer;
    PaSampleFormat outputFormat;
    HPI_DATA data;

    assert( stream );
    out = stream->output;
    /* Only continue if stream has output channels */
    if( !out )
        return result;
    assert( out->tempBuffer );

    /* Clear all existing data in hardware playback buffer */
    PA_ASIHPI_UNLESS_( HPI_OutStreamReset( out->hpiDevice->subSys,
                                           out->hpiStream ), paUnanticipatedHostError );
    /* Fill temp buffer with silence */
    outputFormat = PaAsiHpi_HpiToPaFormat( out->hpiFormat.wFormat );
    zeroer = PaUtil_SelectZeroer( outputFormat );
    zeroer(out->tempBuffer, 1, out->tempBufferSize / Pa_GetSampleSize(outputFormat) );
    /* Write temp buffer to hardware fifo twice, to get started */
    PA_ASIHPI_UNLESS_( HPI_DataCreate( &data, &out->hpiFormat, out->tempBuffer, out->tempBufferSize ),
                       paUnanticipatedHostError );
    PA_ASIHPI_UNLESS_( HPI_OutStreamWrite( out->hpiDevice->subSys,
                                           out->hpiStream, &data ), paUnanticipatedHostError );
    PA_ASIHPI_UNLESS_( HPI_OutStreamWrite( out->hpiDevice->subSys,
                                           out->hpiStream, &data ), paUnanticipatedHostError );

error:
    return result;
}


/** Start HPI streams (both input + output).
 This starts all HPI streams in the PortAudio stream. Output streams are first primed with
 silence, if required. After this call the PA stream is in the Active state.

 @todo Implement priming via the user callback

 @param stream Pointer to stream struct

 @param outputPrimed True if output is already primed (if false, silence will be loaded before starting)

 @return PortAudio error code
 */
static PaError PaAsiHpi_StartStream( PaAsiHpiStream *stream, int outputPrimed )
{
    PaError result = paNoError;

    if( stream->input )
    {
        PA_ASIHPI_UNLESS_( HPI_InStreamStart( stream->input->hpiDevice->subSys,
                                              stream->input->hpiStream ), paUnanticipatedHostError );
    }
    if( stream->output )
    {
        if( !outputPrimed )
        {
            /* Buffer isn't primed, so load stream with silence */
            PA_ENSURE_( PaAsiHpi_PrimeOutputWithSilence( stream ) );
        }
        PA_ASIHPI_UNLESS_( HPI_OutStreamStart( stream->output->hpiDevice->subSys,
                                               stream->output->hpiStream ), paUnanticipatedHostError );
    }
    stream->state = paAsiHpiActiveState;
    stream->callbackFinished = 0;

    /* Report stream info for debugging purposes */
    /*    PaAsiHpi_StreamDump( stream );   */

error:
    return result;
}


/** Start PortAudio stream.
 If the stream has a callback interface, this starts a helper thread to feed the user callback.
 The thread will then take care of starting the HPI streams, and this function will block
 until the streams actually start. In the case of a blocking interface, the HPI streams
 are simply started.

 @param s Pointer to PortAudio stream

 @return PortAudio error code
*/
static PaError StartStream( PaStream *s )
{
    PaError result = paNoError;
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;

    assert( stream );

    /* Ready the processor */
    PaUtil_ResetBufferProcessor( &stream->bufferProcessor );

    if( stream->callbackMode )
    {
        /* Create and start callback engine thread */
        /* Also waits 1 second for stream to be started by engine thread (otherwise aborts) */
        PA_ENSURE_( PaUnixThread_New( &stream->thread, &CallbackThreadFunc, stream, 1., 0 ) );
    }
    else
    {
        PA_ENSURE_( PaAsiHpi_StartStream( stream, 0 ) );
    }

error:
    return result;
}


/** Stop HPI streams (input + output), either softly or abruptly.
 If abort is false, the function blocks until the output stream is drained, otherwise it
 stops immediately and discards data in the stream hardware buffers.

 This function is safe to call from the callback engine thread as well as the main thread.

 @param stream Pointer to stream struct

 @param abort True if samples in output buffer should be discarded (otherwise blocks until stream is done)

 @return PortAudio error code

 */
static PaError PaAsiHpi_StopStream( PaAsiHpiStream *stream, int abort )
{
    PaError result = paNoError;

    assert( stream );

    /* Input channels */
    if( stream->input )
    {
        PA_ASIHPI_UNLESS_( HPI_InStreamReset( stream->input->hpiDevice->subSys,
                                              stream->input->hpiStream ), paUnanticipatedHostError );
    }
    /* Output channels */
    if( stream->output )
    {
        if( !abort )
        {
            /* Wait until HPI output stream is drained */
            while( 1 )
            {
                PaAsiHpiStreamInfo streamInfo;
                PaTime timeLeft;

                /* Obtain number of samples waiting to be played */
                PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->output, &streamInfo ) );
                /* Check if stream is drained */
                if( (streamInfo.state != HPI_STATE_PLAYING) &&
                        (streamInfo.dataSize < stream->output->bytesPerFrame * PA_ASIHPI_MIN_FRAMES_) )
                    break;
                /* Sleep amount of time represented by remaining samples */
                timeLeft = 1000.0 * streamInfo.dataSize / stream->output->bytesPerFrame
                           / stream->baseStreamRep.streamInfo.sampleRate;
                Pa_Sleep( (long)ceil( timeLeft ) );
            }
        }
        PA_ASIHPI_UNLESS_( HPI_OutStreamReset( stream->output->hpiDevice->subSys,
                                               stream->output->hpiStream ), paUnanticipatedHostError );
    }

    /* Report stream info for debugging purposes */
    /*    PaAsiHpi_StreamDump( stream ); */

error:
    return result;
}


/** Stop or abort PortAudio stream.

 This function is used to explicitly stop the PortAudio stream (via StopStream/AbortStream),
 as opposed to the situation when the callback finishes with a result other than paContinue.
 If a stream is in callback mode we will have to inspect whether the background thread has
 finished, or we will have to take it out. In either case we join the thread before returning.
 In blocking mode, we simply tell HPI to stop abruptly (abort) or finish buffers (drain).
 The PortAudio stream will be in the Stopped state after a call to this function.

 Don't call this from the callback engine thread!

 @param stream Pointer to stream struct

 @param abort True if samples in output buffer should be discarded (otherwise blocks until stream is done)

 @return PortAudio error code
*/
static PaError PaAsiHpi_ExplicitStop( PaAsiHpiStream *stream, int abort )
{
    PaError result = paNoError;

    /* First deal with the callback thread, cancelling and/or joining it if necessary */
    if( stream->callbackMode )
    {
        PaError threadRes;
        stream->callbackAbort = abort;
        if( abort )
        {
            PA_DEBUG(( "Aborting callback\n" ));
        }
        else
        {
            PA_DEBUG(( "Stopping callback\n" ));
        }
        PA_ENSURE_( PaUnixThread_Terminate( &stream->thread, !abort, &threadRes ) );
        if( threadRes != paNoError )
        {
            PA_DEBUG(( "Callback thread returned: %d\n", threadRes ));
        }
    }
    else
    {
        PA_ENSURE_( PaAsiHpi_StopStream( stream, abort ) );
    }

    stream->state = paAsiHpiStoppedState;

error:
    return result;
}


/** Stop PortAudio stream.
 This blocks until the output buffers are drained.

 @param s Pointer to PortAudio stream

 @return PortAudio error code
*/
static PaError StopStream( PaStream *s )
{
    return PaAsiHpi_ExplicitStop( (PaAsiHpiStream *) s, 0 );
}


/** Abort PortAudio stream.
 This discards any existing data in output buffers and stops the stream immediately.

 @param s Pointer to PortAudio stream

 @return PortAudio error code
*/
static PaError AbortStream( PaStream *s )
{
    return PaAsiHpi_ExplicitStop( (PaAsiHpiStream * ) s, 1 );
}


/** Determine whether the stream is stopped.
 A stream is considered to be stopped prior to a successful call to StartStream and after
 a successful call to StopStream or AbortStream. If a stream callback returns a value other
 than paContinue the stream is NOT considered to be stopped (it is in CallbackFinished state).

 @param s Pointer to PortAudio stream

 @return Returns one (1) when the stream is stopped, zero (0) when the stream is running, or
         a PaErrorCode (which are always negative) if PortAudio is not initialized or an
         error is encountered.
*/
static PaError IsStreamStopped( PaStream *s )
{
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;

    assert( stream );
    return stream->state == paAsiHpiStoppedState ? 1 : 0;
}


/** Determine whether the stream is active.
 A stream is active after a successful call to StartStream(), until it becomes inactive either
 as a result of a call to StopStream() or AbortStream(), or as a result of a return value
 other than paContinue from the stream callback. In the latter case, the stream is considered
 inactive after the last buffer has finished playing.

 @param s Pointer to PortAudio stream

 @return Returns one (1) when the stream is active (i.e. playing or recording audio),
         zero (0) when not playing, or a PaErrorCode (which are always negative)
         if PortAudio is not initialized or an error is encountered.
*/
static PaError IsStreamActive( PaStream *s )
{
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;

    assert( stream );
    return stream->state == paAsiHpiActiveState ? 1 : 0;
}


/** Returns current stream time.
 This corresponds to the system clock. The clock should run continuously while the stream
 is open, i.e. between calls to OpenStream() and CloseStream(), therefore a frame counter
 is not good enough.

 @param s Pointer to PortAudio stream

 @return Stream time, in seconds
 */
static PaTime GetStreamTime( PaStream *s )
{
    return PaUtil_GetTime();
}


/** Returns CPU load.

 @param s Pointer to PortAudio stream

 @return CPU load (0.0 if blocking interface is used)
 */
static double GetStreamCpuLoad( PaStream *s )
{
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;

    return stream->callbackMode ? PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer ) : 0.0;
}

/* --------------------------- Callback Interface --------------------------- */

/** Exit routine which is called when callback thread quits.
 This takes care of stopping the HPI streams (either waiting for output to finish, or
 abruptly). It also calls the user-supplied StreamFinished callback, and sets the
 stream state to CallbackFinished if it was reached via a non-paContinue return from
 the user callback function.

 @param userData A pointer to an open stream previously created with Pa_OpenStream
 */
static void PaAsiHpi_OnThreadExit( void *userData )
{
    PaAsiHpiStream *stream = (PaAsiHpiStream *) userData;

    assert( stream );

    PaUtil_ResetCpuLoadMeasurer( &stream->cpuLoadMeasurer );

    PA_DEBUG(( "%s: Stopping HPI streams\n", __FUNCTION__ ));
    PaAsiHpi_StopStream( stream, stream->callbackAbort );
    PA_DEBUG(( "%s: Stoppage\n", __FUNCTION__ ));

    /* Eventually notify user all buffers have played */
    if( stream->baseStreamRep.streamFinishedCallback )
    {
        stream->baseStreamRep.streamFinishedCallback( stream->baseStreamRep.userData );
    }

    /* Unfortunately both explicit calls to Stop/AbortStream (leading to Stopped state)
     and implicit stops via paComplete/paAbort (leading to CallbackFinished state)
     end up here - need another flag to remind us which is the case */
    if( stream->callbackFinished )
        stream->state = paAsiHpiCallbackFinishedState;
}


/** Wait until there is enough frames to fill a host buffer.
 The routine attempts to sleep until at least a full host buffer can be retrieved from the
 input HPI stream and passed to the output HPI stream. It will first sleep until enough
 output space is available, as this is usually easily achievable. If it is an output-only
 stream, it will also sleep if the hardware buffer is too full, thereby throttling the
 filling of the output buffer and reducing output latency. The routine then blocks until
 enough input samples are available, unless this will cause an output underflow. In the
 process, input overflows and output underflows are indicated.

 @param stream Pointer to stream struct

 @param framesAvail Returns the number of available frames

 @param cbFlags Overflows and underflows indicated in here

 @return PortAudio error code (only paUnanticipatedHostError expected)
 */
static PaError PaAsiHpi_WaitForFrames( PaAsiHpiStream *stream, unsigned long *framesAvail,
                                       PaStreamCallbackFlags *cbFlags )
{
    PaError result = paNoError;
    double sampleRate;
    unsigned long framesTarget;
    HW32 outputData = 0, outputSpace = 0, inputData = 0, framesLeft = 0;

    assert( stream );
    assert( stream->input || stream->output );

    sampleRate = stream->baseStreamRep.streamInfo.sampleRate;
    /* We have to come up with this much frames on both input and output */
    framesTarget = stream->bufferProcessor.framesPerHostBuffer;
    assert( framesTarget > 0 );

    while( 1 )
    {
        PaAsiHpiStreamInfo info;
        /* Check output first, as this takes priority in the default full-duplex mode */
        if( stream->output )
        {
            PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->output, &info ) );
            /* Wait until enough space is available in output buffer to receive a full block */
            if( info.availableFrames < framesTarget )
            {
                framesLeft = framesTarget - info.availableFrames;
                Pa_Sleep( (long)ceil( 1000 * framesLeft / sampleRate ) );
                continue;
            }
            /* Wait until the data in hardware buffer has dropped to a sensible level.
             Without this, the hardware buffer quickly fills up in the absence of an input
             stream to regulate its data rate (if data generation is fast). This leads to
             large latencies, as the AudioScience hardware buffers are humongous.
             This is similar to the default "Hardware Buffering=off" option in the
             AudioScience WAV driver. */
            if( !stream->input && (stream->output->outputBufferCap > 0) &&
                    ( info.totalBufferedData > stream->output->outputBufferCap / stream->output->bytesPerFrame ) )
            {
                framesLeft = info.totalBufferedData - stream->output->outputBufferCap / stream->output->bytesPerFrame;
                Pa_Sleep( (long)ceil( 1000 * framesLeft / sampleRate ) );
                continue;
            }
            outputData = info.totalBufferedData;
            outputSpace = info.availableFrames;
            /* Report output underflow to callback */
            if( info.underflow )
            {
                *cbFlags |= paOutputUnderflow;
            }
        }

        /* Now check input side */
        if( stream->input )
        {
            PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->input, &info ) );
            /* If a full block of samples hasn't been recorded yet, wait for it if possible */
            if( info.availableFrames < framesTarget )
            {
                framesLeft = framesTarget - info.availableFrames;
                /* As long as output is not disrupted in the process, wait for a full
                block of input samples */
                if( !stream->output || (outputData > framesLeft) )
                {
                    Pa_Sleep( (long)ceil( 1000 * framesLeft / sampleRate ) );
                    continue;
                }
            }
            inputData = info.availableFrames;
            /** @todo The paInputOverflow flag should be set in the callback containing the
             first input sample following the overflow. That means the block currently sitting
             at the fore-front of recording, i.e. typically the one containing the newest (last)
             sample in the HPI buffer system. This is most likely not the same as the current
             block of data being passed to the callback. The current overflow should ideally
             be noted in an overflow list of sorts, with an indication of when it should be
             reported. The trouble starts if there are several separate overflow incidents,
             given a big input buffer. Oh well, something to try out later... */
            if( info.overflow )
            {
                *cbFlags |= paInputOverflow;
            }
        }
        break;
    }
    /* Full-duplex stream */
    if( stream->input && stream->output )
    {
        if( outputSpace >= framesTarget )
            *framesAvail = outputSpace;
        /* If input didn't make the target, keep the output count instead (input underflow) */
        if( (inputData >= framesTarget) && (inputData < outputSpace) )
            *framesAvail = inputData;
    }
    else
    {
        *framesAvail = stream->input ? inputData : outputSpace;
    }

error:
    return result;
}


/** Obtain recording, current and playback timestamps of stream.
 The current time is determined by the system clock. This "now" timestamp occurs at the
 forefront of recording (and playback in the full-duplex case), which happens later than the
 input timestamp by an amount equal to the total number of recorded frames in the input buffer.
 The output timestamp indicates when the next generated sample will actually be played. This
 happens after all the samples currently in the output buffer are played. The output timestamp
 therefore follows the current timestamp by an amount equal to the number of frames yet to be
 played back in the output buffer.

 If the current timestamp is the present, the input timestamp is in the past and the output
 timestamp is in the future.

 @param stream Pointer to stream struct

 @param timeInfo Pointer to timeInfo struct that will contain timestamps
 */
static void PaAsiHpi_CalculateTimeInfo( PaAsiHpiStream *stream, PaStreamCallbackTimeInfo *timeInfo )
{
    PaAsiHpiStreamInfo streamInfo;
    double sampleRate;

    assert( stream );
    assert( timeInfo );
    sampleRate = stream->baseStreamRep.streamInfo.sampleRate;

    /* The current time ("now") is at the forefront of both recording and playback */
    timeInfo->currentTime = GetStreamTime( (PaStream *)stream );
    /* The last sample in the input buffer was recorded just now, so the first sample
     happened (number of recorded samples)/sampleRate ago */
    timeInfo->inputBufferAdcTime = timeInfo->currentTime;
    if( stream->input )
    {
        PaAsiHpi_GetStreamInfo( stream->input, &streamInfo );
        timeInfo->inputBufferAdcTime -= streamInfo.totalBufferedData / sampleRate;
    }
    /* The first of the outgoing samples will be played after all the samples in the output
     buffer is done */
    timeInfo->outputBufferDacTime = timeInfo->currentTime;
    if( stream->output )
    {
        PaAsiHpi_GetStreamInfo( stream->output, &streamInfo );
        timeInfo->outputBufferDacTime += streamInfo.totalBufferedData / sampleRate;
    }
}


/** Read from HPI input stream and register buffers.
 This reads data from the HPI input stream (if it exists) and registers the temp stream
 buffers of both input and output streams with the buffer processor. In the process it also
 handles input underflows in the full-duplex case.

 @param stream Pointer to stream struct

 @param numFrames On entrance the number of available frames, on exit the number of
                  received frames

 @param cbFlags Indicates overflows and underflows

 @return PortAudio error code
 */
static PaError PaAsiHpi_BeginProcessing( PaAsiHpiStream *stream, unsigned long *numFrames,
        PaStreamCallbackFlags *cbFlags )
{
    PaError result = paNoError;

    assert( stream );
    if( *numFrames > stream->maxFramesPerHostBuffer )
        *numFrames = stream->maxFramesPerHostBuffer;

    if( stream->input )
    {
        PaAsiHpiStreamInfo info;
        HPI_DATA data;
        HW32 framesToGet = *numFrames;

        /* Check for overflows and underflows yet again */
        PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->input, &info ) );
        if( info.overflow )
        {
            *cbFlags |= paInputOverflow;
        }
        /* Input underflow if less than expected number of samples pitch up */
        if( framesToGet > info.availableFrames )
        {
            PaUtilZeroer *zeroer;
            PaSampleFormat inputFormat;

            /* Never call an input-only stream with InputUnderflow set */
            if( stream->output )
                *cbFlags |= paInputUnderflow;
            framesToGet = info.availableFrames;
            /* Fill temp buffer with silence (to make up for missing input samples) */
            inputFormat = PaAsiHpi_HpiToPaFormat( stream->input->hpiFormat.wFormat );
            zeroer = PaUtil_SelectZeroer( inputFormat );
            zeroer(stream->input->tempBuffer, 1,
                   stream->input->tempBufferSize / Pa_GetSampleSize(inputFormat) );
        }

        /* Setup HPI data structure around temp buffer */
        HPI_DataCreate( &data, &stream->input->hpiFormat, stream->input->tempBuffer,
                        framesToGet * stream->input->bytesPerFrame );
        /* Read block of data into temp buffer */
        PA_ASIHPI_UNLESS_( HPI_InStreamRead( stream->input->hpiDevice->subSys,
                                             stream->input->hpiStream, &data ),
                           paUnanticipatedHostError );
        /* Register temp buffer with buffer processor (always FULL buffer) */
        PaUtil_SetInputFrameCount( &stream->bufferProcessor, *numFrames );
        /* HPI interface only allows interleaved channels */
        PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor,
                                            0, stream->input->tempBuffer,
                                            stream->input->hpiFormat.wChannels );
    }
    if( stream->output )
    {
        /* Register temp buffer with buffer processor */
        PaUtil_SetOutputFrameCount( &stream->bufferProcessor, *numFrames );
        /* HPI interface only allows interleaved channels */
        PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor,
                                             0, stream->output->tempBuffer,
                                             stream->output->hpiFormat.wChannels );
    }

error:
    return result;
}


/** Flush output buffers to HPI output stream.
 This completes the processing cycle by writing the temp buffer to the HPI interface.
 Additional output underflows are caught before data is written to the stream, as this
 action typically remedies the underflow and hides it in the process.

 @param stream Pointer to stream struct

 @param numFrames The number of frames to write to the output stream

 @param cbFlags Indicates overflows and underflows
 */
static PaError PaAsiHpi_EndProcessing( PaAsiHpiStream *stream, unsigned long numFrames,
                                       PaStreamCallbackFlags *cbFlags )
{
    PaError result = paNoError;

    assert( stream );

    if( stream->output )
    {
        PaAsiHpiStreamInfo info;
        HPI_DATA data;

        /* Check for underflows after the (potentially time-consuming) callback */
        PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->output, &info ) );
        if( info.underflow )
        {
            *cbFlags |= paOutputUnderflow;
        }

        /* Setup HPI data structure around temp buffer */
        HPI_DataCreate( &data, &stream->output->hpiFormat, stream->output->tempBuffer,
                        numFrames * stream->output->bytesPerFrame );
        /* Write temp buffer to HPI stream */
        PA_ASIHPI_UNLESS_( HPI_OutStreamWrite( stream->output->hpiDevice->subSys,
                                               stream->output->hpiStream, &data ),
                           paUnanticipatedHostError );
    }

error:
    return result;
}


/** Main callback engine.
 This function runs in a separate thread and does all the work of fetching audio data from
 the AudioScience card via the HPI interface, feeding it to the user callback via the buffer
 processor, and delivering the resulting output data back to the card via HPI calls.
 It is started and terminated when the PortAudio stream is started and stopped, and starts
 the HPI streams on startup.

 @param userData A pointer to an open stream previously created with Pa_OpenStream.
*/
static void *CallbackThreadFunc( void *userData )
{
    PaError result = paNoError;
    PaAsiHpiStream *stream = (PaAsiHpiStream *) userData;
    int callbackResult = paContinue;

    assert( stream );

    /* Cleanup routine stops streams on thread exit */
    pthread_cleanup_push( &PaAsiHpi_OnThreadExit, stream );

    /* Start HPI streams and notify parent when we're done */
    PA_ENSURE_( PaUnixThread_PrepareNotify( &stream->thread ) );
    /* Buffer will be primed with silence */
    PA_ENSURE_( PaAsiHpi_StartStream( stream, 0 ) );
    PA_ENSURE_( PaUnixThread_NotifyParent( &stream->thread ) );

    /* MAIN LOOP */
    while( 1 )
    {
        PaStreamCallbackFlags cbFlags = 0;
        unsigned long framesAvail, framesGot;

        pthread_testcancel();

        /** @concern StreamStop if the main thread has requested a stop and the stream has not
        * been effectively stopped we signal this condition by modifying callbackResult
        * (we'll want to flush buffered output). */
        if( PaUnixThread_StopRequested( &stream->thread ) && (callbackResult == paContinue) )
        {
            PA_DEBUG(( "Setting callbackResult to paComplete\n" ));
            callbackResult = paComplete;
        }

        /* Start winding down thread if requested */
        if( callbackResult != paContinue )
        {
            stream->callbackAbort = (callbackResult == paAbort);
            if( stream->callbackAbort ||
                    /** @concern BlockAdaption: Go on if adaption buffers are empty */
                    PaUtil_IsBufferProcessorOutputEmpty( &stream->bufferProcessor ) )
            {
                goto end;
            }
            PA_DEBUG(( "%s: Flushing buffer processor\n", __FUNCTION__ ));
            /* There is still buffered output that needs to be processed */
        }

        /* SLEEP */
        /* Wait for data (or buffer space) to become available. This basically sleeps and
        polls the HPI interface until a full block of frames can be moved. */
        PA_ENSURE_( PaAsiHpi_WaitForFrames( stream, &framesAvail, &cbFlags ) );

        /* Consume buffer space. Once we have a number of frames available for consumption we
        must retrieve the data from the HPI interface and pass it to the PA buffer processor.
        We should be prepared to process several chunks successively. */
        while( framesAvail > 0 )
        {
            PaStreamCallbackTimeInfo timeInfo = {0, 0, 0};

            pthread_testcancel();

            framesGot = framesAvail;
            if( stream->bufferProcessor.hostBufferSizeMode == paUtilFixedHostBufferSize )
            {
                /* We've committed to a fixed host buffer size, stick to that */
                framesGot = framesGot >= stream->maxFramesPerHostBuffer ? stream->maxFramesPerHostBuffer : 0;
            }
            else
            {
                /* We've committed to an upper bound on the size of host buffers */
                assert( stream->bufferProcessor.hostBufferSizeMode == paUtilBoundedHostBufferSize );
                framesGot = PA_MIN( framesGot, stream->maxFramesPerHostBuffer );
            }

            /* Obtain buffer timestamps */
            PaAsiHpi_CalculateTimeInfo( stream, &timeInfo );
            PaUtil_BeginBufferProcessing( &stream->bufferProcessor, &timeInfo, cbFlags );
            /* CPU load measurement should include processing activivity external to the stream callback */
            PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );
            if( framesGot > 0 )
            {
                /* READ FROM HPI INPUT STREAM */
                PA_ENSURE_( PaAsiHpi_BeginProcessing( stream, &framesGot, &cbFlags ) );
                /* Input overflow in a full-duplex stream makes for interesting times */
                if( stream->input && stream->output && (cbFlags & paInputOverflow) )
                {
                    /* Special full-duplex paNeverDropInput mode */
                    if( stream->neverDropInput )
                    {
                        PaUtil_SetNoOutput( &stream->bufferProcessor );
                        cbFlags |= paOutputOverflow;
                    }
                }
                /* CALL USER CALLBACK WITH INPUT DATA, AND OBTAIN OUTPUT DATA */
                PaUtil_EndBufferProcessing( &stream->bufferProcessor, &callbackResult );
                /* Clear overflow and underflow information (but PaAsiHpi_EndProcessing might
                still show up output underflow that will carry over to next round) */
                cbFlags = 0;
                /*  WRITE TO HPI OUTPUT STREAM */
                PA_ENSURE_( PaAsiHpi_EndProcessing( stream, framesGot, &cbFlags ) );
                /* Advance frame counter */
                framesAvail -= framesGot;
            }
            PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, framesGot );

            if( framesGot == 0 )
            {
                /* Go back to polling for more frames */
                break;

            }
            if( callbackResult != paContinue )
                break;
        }
    }

    /* This code is unreachable, but important to include regardless because it
     * is possibly a macro with a closing brace to match the opening brace in
     * pthread_cleanup_push() above.  The documentation states that they must
     * always occur in pairs. */
    pthread_cleanup_pop( 1 );

end:
    /* Indicates normal exit of callback, as opposed to the thread getting killed explicitly */
    stream->callbackFinished = 1;
    PA_DEBUG(( "%s: Thread %d exiting (callbackResult = %d)\n ",
               __FUNCTION__, pthread_self(), callbackResult ));
    /* Exit from thread and report any PortAudio error in the process */
    PaUnixThreading_EXIT( result );
error:
    goto end;
}

/* --------------------------- Blocking Interface --------------------------- */

/* As separate stream interfaces are used for blocking and callback streams, the following
 functions can be guaranteed to only be called for blocking streams. */

/** Read data from input stream.
 This reads the indicated number of frames into the supplied buffer from an input stream,
 and blocks until this is done.

 @param s Pointer to PortAudio stream

 @param buffer Pointer to buffer that will receive interleaved data (or an array of pointers
               to a buffer for each non-interleaved channel)

 @param frames Number of frames to read from stream

 @return PortAudio error code (also indicates overflow via paInputOverflowed)
 */
static PaError ReadStream( PaStream *s,
                           void *buffer,
                           unsigned long frames )
{
    PaError result = paNoError;
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;
    PaAsiHpiStreamInfo info;
    void *userBuffer;

    assert( stream );
    PA_UNLESS_( stream->input, paCanNotReadFromAnOutputOnlyStream );

    /* Check for input overflow since previous call to ReadStream */
    PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->input, &info ) );
    if( info.overflow )
    {
        result = paInputOverflowed;
    }

    /* NB Make copy of user buffer pointers, since they are advanced by buffer processor */
    if( stream->bufferProcessor.userInputIsInterleaved )
    {
        userBuffer = buffer;
    }
    else
    {
        /* Copy channels into local array */
        userBuffer = stream->blockingUserBufferCopy;
        memcpy( userBuffer, buffer, sizeof (void *) * stream->input->hpiFormat.wChannels );
    }

    while( frames > 0 )
    {
        unsigned long framesGot, framesAvail;
        PaStreamCallbackFlags cbFlags = 0;

        PA_ENSURE_( PaAsiHpi_WaitForFrames( stream, &framesAvail, &cbFlags ) );
        framesGot = PA_MIN( framesAvail, frames );
        PA_ENSURE_( PaAsiHpi_BeginProcessing( stream, &framesGot, &cbFlags ) );

        if( framesGot > 0 )
        {
            framesGot = PaUtil_CopyInput( &stream->bufferProcessor, &userBuffer, framesGot );
            PA_ENSURE_( PaAsiHpi_EndProcessing( stream, framesGot, &cbFlags ) );
            /* Advance frame counter */
            frames -= framesGot;
        }
    }

error:
    return result;
}


/** Write data to output stream.
 This writes the indicated number of frames from the supplied buffer to an output stream,
 and blocks until this is done.

 @param s Pointer to PortAudio stream

 @param buffer Pointer to buffer that provides interleaved data (or an array of pointers
               to a buffer for each non-interleaved channel)

 @param frames Number of frames to write to stream

 @return PortAudio error code (also indicates underflow via paOutputUnderflowed)
 */
static PaError WriteStream( PaStream *s,
                            const void *buffer,
                            unsigned long frames )
{
    PaError result = paNoError;
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;
    PaAsiHpiStreamInfo info;
    const void *userBuffer;

    assert( stream );
    PA_UNLESS_( stream->output, paCanNotWriteToAnInputOnlyStream );

    /* Check for output underflow since previous call to WriteStream */
    PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->output, &info ) );
    if( info.underflow )
    {
        result = paOutputUnderflowed;
    }

    /* NB Make copy of user buffer pointers, since they are advanced by buffer processor */
    if( stream->bufferProcessor.userOutputIsInterleaved )
    {
        userBuffer = buffer;
    }
    else
    {
        /* Copy channels into local array */
        userBuffer = stream->blockingUserBufferCopy;
        memcpy( (void *)userBuffer, buffer, sizeof (void *) * stream->output->hpiFormat.wChannels );
    }

    while( frames > 0 )
    {
        unsigned long framesGot, framesAvail;
        PaStreamCallbackFlags cbFlags = 0;

        PA_ENSURE_( PaAsiHpi_WaitForFrames( stream, &framesAvail, &cbFlags ) );
        framesGot = PA_MIN( framesAvail, frames );
        PA_ENSURE_( PaAsiHpi_BeginProcessing( stream, &framesGot, &cbFlags ) );

        if( framesGot > 0 )
        {
            framesGot = PaUtil_CopyOutput( &stream->bufferProcessor, &userBuffer, framesGot );
            PA_ENSURE_( PaAsiHpi_EndProcessing( stream, framesGot, &cbFlags ) );
            /* Advance frame counter */
            frames -= framesGot;
        }
    }

error:
    return result;
}


/** Number of frames that can be read from input stream without blocking.

 @param s Pointer to PortAudio stream

 @return Number of frames, or PortAudio error code
 */
static signed long GetStreamReadAvailable( PaStream *s )
{
    PaError result = paNoError;
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;
    PaAsiHpiStreamInfo info;

    assert( stream );
    PA_UNLESS_( stream->input, paCanNotReadFromAnOutputOnlyStream );

    PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->input, &info ) );
    /* Round down to the nearest host buffer multiple */
    result = (info.availableFrames / stream->maxFramesPerHostBuffer) * stream->maxFramesPerHostBuffer;
    if( info.overflow )
    {
        result = paInputOverflowed;
    }

error:
    return result;
}


/** Number of frames that can be written to output stream without blocking.

 @param s Pointer to PortAudio stream

 @return Number of frames, or PortAudio error code
 */
static signed long GetStreamWriteAvailable( PaStream *s )
{
    PaError result = paNoError;
    PaAsiHpiStream *stream = (PaAsiHpiStream*)s;
    PaAsiHpiStreamInfo info;

    assert( stream );
    PA_UNLESS_( stream->output, paCanNotWriteToAnInputOnlyStream );

    PA_ENSURE_( PaAsiHpi_GetStreamInfo( stream->output, &info ) );
    /* Round down to the nearest host buffer multiple */
    result = (info.availableFrames / stream->maxFramesPerHostBuffer) * stream->maxFramesPerHostBuffer;
    if( info.underflow )
    {
        result = paOutputUnderflowed;
    }

error:
    return result;
}
