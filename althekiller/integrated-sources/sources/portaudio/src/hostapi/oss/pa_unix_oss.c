/*
 * $Id: pa_unix_oss.c 1296 2007-10-28 22:43:50Z aknudsen $
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 * OSS implementation by:
 *   Douglas Repetto
 *   Phil Burk
 *   Dominic Mazzoni
 *   Arve Knudsen
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <limits.h>
#include <semaphore.h>

#ifdef HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
# define DEVICE_NAME_BASE            "/dev/dsp"
#elif defined(HAVE_LINUX_SOUNDCARD_H)
# include <linux/soundcard.h>
# define DEVICE_NAME_BASE            "/dev/dsp"
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
# include <machine/soundcard.h> /* JH20010905 */
# define DEVICE_NAME_BASE            "/dev/audio"
#else
# error No sound card header file
#endif

#include "portaudio.h"
#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_unix_util.h"
#include "pa_debugprint.h"

static int sysErr_;
static pthread_t mainThread_;

/* Check return value of system call, and map it to PaError */
#define ENSURE_(expr, code) \
    do { \
        if( UNLIKELY( (sysErr_ = (expr)) < 0 ) ) \
        { \
            /* PaUtil_SetLastHostErrorInfo should only be used in the main thread */ \
            if( (code) == paUnanticipatedHostError && pthread_self() == mainThread_ ) \
            { \
                PaUtil_SetLastHostErrorInfo( paALSA, sysErr_, strerror( errno ) ); \
            } \
            \
            PaUtil_DebugPrint(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            result = (code); \
            goto error; \
        } \
    } while( 0 );

#ifndef AFMT_S16_NE
#define AFMT_S16_NE  Get_AFMT_S16_NE()
/*********************************************************************
 * Some versions of OSS do not define AFMT_S16_NE. So check CPU.
 * PowerPC is Big Endian. X86 is Little Endian.
 */
static int Get_AFMT_S16_NE( void )
{
    long testData = 1;
    char *ptr = (char *) &testData;
    int isLittle = ( *ptr == 1 ); /* Does address point to least significant byte? */
    return isLittle ? AFMT_S16_LE : AFMT_S16_BE;
}
#endif

/* PaOSSHostApiRepresentation - host api datastructure specific to this implementation */

typedef struct
{
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;

    PaUtilAllocationGroup *allocations;

    PaHostApiIndex hostApiIndex;
}
PaOSSHostApiRepresentation;

/** Per-direction structure for PaOssStream.
 *
 * Aspect StreamChannels: In case the user requests to open the same device for both capture and playback,
 * but with different number of channels we will have to adapt between the number of user and host
 * channels for at least one direction, since the configuration space is the same for both directions
 * of an OSS device.
 */
typedef struct
{
    int fd;
    const char *devName;
    int userChannelCount, hostChannelCount;
    int userInterleaved;
    void *buffer;
    PaSampleFormat userFormat, hostFormat;
    double latency;
    unsigned long hostFrames, numBufs;
    void **userBuffers; /* For non-interleaved blocking */
} PaOssStreamComponent;

/** Implementation specific representation of a PaStream.
 *
 */
typedef struct PaOssStream
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    PaUtilThreading threading;

    int sharedDevice;
    unsigned long framesPerHostBuffer;
    int triggered;  /* Have the devices been triggered yet (first start) */

    int isActive;
    int isStopped;

    int lastPosPtr;
    double lastStreamBytes;

    int framesProcessed;

    double sampleRate;

    int callbackMode;
    int callbackStop, callbackAbort;

    PaOssStreamComponent *capture, *playback;
    unsigned long pollTimeout;
    sem_t semaphore;
}
PaOssStream;

typedef enum {
    StreamMode_In,
    StreamMode_Out
} StreamMode;

/* prototypes for functions declared in this file */

static void Terminate( struct PaUtilHostApiRepresentation *hostApi );
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate );
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** s,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData );
static PaError CloseStream( PaStream* stream );
static PaError StartStream( PaStream *stream );
static PaError StopStream( PaStream *stream );
static PaError AbortStream( PaStream *stream );
static PaError IsStreamStopped( PaStream *s );
static PaError IsStreamActive( PaStream *stream );
static PaTime GetStreamTime( PaStream *stream );
static double GetStreamCpuLoad( PaStream* stream );
static PaError ReadStream( PaStream* stream, void *buffer, unsigned long frames );
static PaError WriteStream( PaStream* stream, const void *buffer, unsigned long frames );
static signed long GetStreamReadAvailable( PaStream* stream );
static signed long GetStreamWriteAvailable( PaStream* stream );
static PaError BuildDeviceList( PaOSSHostApiRepresentation *hostApi );


/** Initialize the OSS API implementation.
 *
 * This function will initialize host API datastructures and query host devices for information.
 *
 * Aspect DeviceCapabilities: Enumeration of host API devices is initiated from here
 *
 * Aspect FreeResources: If an error is encountered under way we have to free each resource allocated in this function,
 * this happens with the usual "error" label.
 */
PaError PaOSS_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    PaOSSHostApiRepresentation *ossHostApi = NULL;

    PA_UNLESS( ossHostApi = (PaOSSHostApiRepresentation*)PaUtil_AllocateMemory( sizeof(PaOSSHostApiRepresentation) ),
            paInsufficientMemory );
    PA_UNLESS( ossHostApi->allocations = PaUtil_CreateAllocationGroup(), paInsufficientMemory );
    ossHostApi->hostApiIndex = hostApiIndex;

    /* Initialize host API structure */
    *hostApi = &ossHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;
    (*hostApi)->info.type = paOSS;
    (*hostApi)->info.name = "OSS";
    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PA_ENSURE( BuildDeviceList( ossHostApi ) );

    PaUtil_InitializeStreamInterface( &ossHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable,
                                      PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &ossHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );

    mainThread_ = pthread_self();

    return result;

error:
    if( ossHostApi )
    {
        if( ossHostApi->allocations )
        {
            PaUtil_FreeAllAllocations( ossHostApi->allocations );
            PaUtil_DestroyAllocationGroup( ossHostApi->allocations );
        }

        PaUtil_FreeMemory( ossHostApi );
    }
    return result;
}

PaError PaUtil_InitializeDeviceInfo( PaDeviceInfo *deviceInfo, const char *name, PaHostApiIndex hostApiIndex, int maxInputChannels,
        int maxOutputChannels, PaTime defaultLowInputLatency, PaTime defaultLowOutputLatency, PaTime defaultHighInputLatency,
        PaTime defaultHighOutputLatency, double defaultSampleRate, PaUtilAllocationGroup *allocations  )
{
    PaError result = paNoError;

    deviceInfo->structVersion = 2;
    if( allocations )
    {
        size_t len = strlen( name ) + 1;
        PA_UNLESS( deviceInfo->name = PaUtil_GroupAllocateMemory( allocations, len ), paInsufficientMemory );
        strncpy( (char *)deviceInfo->name, name, len );
    }
    else
        deviceInfo->name = name;

    deviceInfo->hostApi = hostApiIndex;
    deviceInfo->maxInputChannels = maxInputChannels;
    deviceInfo->maxOutputChannels = maxOutputChannels;
    deviceInfo->defaultLowInputLatency = defaultLowInputLatency;
    deviceInfo->defaultLowOutputLatency = defaultLowOutputLatency;
    deviceInfo->defaultHighInputLatency = defaultHighInputLatency;
    deviceInfo->defaultHighOutputLatency = defaultHighOutputLatency;
    deviceInfo->defaultSampleRate = defaultSampleRate;

error:
    return result;
}

static PaError QueryDirection( const char *deviceName, StreamMode mode, double *defaultSampleRate, int *maxChannelCount,
        double *defaultLowLatency, double *defaultHighLatency )
{
    PaError result = paNoError;
    int numChannels, maxNumChannels;
    int busy = 0;
    int devHandle = -1;
    int sr;
    *maxChannelCount = 0;  /* Default value in case this fails */

    if ( (devHandle = open( deviceName, (mode == StreamMode_In ? O_RDONLY : O_WRONLY) | O_NONBLOCK ))  < 0 )
    {
        if( errno == EBUSY || errno == EAGAIN )
        {
            PA_DEBUG(( "%s: Device %s busy\n", __FUNCTION__, deviceName ));
        }
        else
        {
            PA_DEBUG(( "%s: Can't access device: %s\n", __FUNCTION__, strerror( errno ) ));
        }

        return paDeviceUnavailable;
    }

    /* Negotiate for the maximum number of channels for this device. PLB20010927
     * Consider up to 16 as the upper number of channels.
     * Variable maxNumChannels should contain the actual upper limit after the call.
     * Thanks to John Lazzaro and Heiko Purnhagen for suggestions.
     */
    maxNumChannels = 0;
    for( numChannels = 1; numChannels <= 16; numChannels++ )
    {
        int temp = numChannels;
        if( ioctl( devHandle, SNDCTL_DSP_CHANNELS, &temp ) < 0 )
        {
            busy = EAGAIN == errno || EBUSY == errno;
            /* ioctl() failed so bail out if we already have stereo */
            if( maxNumChannels >= 2 )
                break;
        }
        else
        {
            /* ioctl() worked but bail out if it does not support numChannels.
             * We don't want to leave gaps in the numChannels supported.
             */
            if( (numChannels > 2) && (temp != numChannels) )
                break;
            if( temp > maxNumChannels )
                maxNumChannels = temp; /* Save maximum. */
        }
    }
    /* A: We're able to open a device for capture if it's busy playing back and vice versa,
     * but we can't configure anything */
    if( 0 == maxNumChannels && busy )
    {
        result = paDeviceUnavailable;
        goto error;
    }

    /* The above negotiation may fail for an old driver so try this older technique. */
    if( maxNumChannels < 1 )
    {
        int stereo = 1;
        if( ioctl( devHandle, SNDCTL_DSP_STEREO, &stereo ) < 0 )
        {
            maxNumChannels = 1;
        }
        else
        {
            maxNumChannels = (stereo) ? 2 : 1;
        }
        PA_DEBUG(( "%s: use SNDCTL_DSP_STEREO, maxNumChannels = %d\n", __FUNCTION__, maxNumChannels ));
    }

    /* During channel negotiation, the last ioctl() may have failed. This can
     * also cause sample rate negotiation to fail. Hence the following, to return
     * to a supported number of channels. SG20011005 */
    {
        /* use most reasonable default value */
        int temp = PA_MIN( maxNumChannels, 2 );
        ENSURE_( ioctl( devHandle, SNDCTL_DSP_CHANNELS, &temp ), paUnanticipatedHostError );
    }

    /* Get supported sample rate closest to 44100 Hz */
    if( *defaultSampleRate < 0 )
    {
        sr = 44100;
        if( ioctl( devHandle, SNDCTL_DSP_SPEED, &sr ) < 0 )
        {
            result = paUnanticipatedHostError;
            goto error;
        }

        *defaultSampleRate = sr;
    }

    *maxChannelCount = maxNumChannels;
    /* TODO */
    *defaultLowLatency = 512. / *defaultSampleRate;
    *defaultHighLatency = 2048. / *defaultSampleRate;

error:
    if( devHandle >= 0 )
        close( devHandle );

    return result;
}

/** Query OSS device.
 *
 * This is where PaDeviceInfo objects are constructed and filled in with relevant information.
 *
 * Aspect DeviceCapabilities: The inferred device capabilities are recorded in a PaDeviceInfo object that is constructed
 * in place.
 */
static PaError QueryDevice( char *deviceName, PaOSSHostApiRepresentation *ossApi, PaDeviceInfo **deviceInfo )
{
    PaError result = paNoError;
    double sampleRate = -1.;
    int maxInputChannels, maxOutputChannels;
    PaTime defaultLowInputLatency, defaultLowOutputLatency, defaultHighInputLatency, defaultHighOutputLatency;
    PaError tmpRes = paNoError;
    int busy = 0;
    *deviceInfo = NULL;

    /* douglas:
       we have to do this querying in a slightly different order. apparently
       some sound cards will give you different info based on their settins.
       e.g. a card might give you stereo at 22kHz but only mono at 44kHz.
       the correct order for OSS is: format, channels, sample rate
    */

    /* Aspect StreamChannels: The number of channels supported for a device may depend on the mode it is
     * opened in, it may have more channels available for capture than playback and vice versa. Therefore
     * we will open the device in both read- and write-only mode to determine the supported number.
     */
    if( (tmpRes = QueryDirection( deviceName, StreamMode_In, &sampleRate, &maxInputChannels, &defaultLowInputLatency,
                &defaultHighInputLatency )) != paNoError )
    {
        if( tmpRes != paDeviceUnavailable )
        {
            PA_DEBUG(( "%s: Querying device %s for capture failed!\n", __FUNCTION__, deviceName ));
            /* PA_ENSURE( tmpRes ); */
        }
        ++busy;
    }
    if( (tmpRes = QueryDirection( deviceName, StreamMode_Out, &sampleRate, &maxOutputChannels, &defaultLowOutputLatency,
                &defaultHighOutputLatency )) != paNoError )
    {
        if( tmpRes != paDeviceUnavailable )
        {
            PA_DEBUG(( "%s: Querying device %s for playback failed!\n", __FUNCTION__, deviceName ));
            /* PA_ENSURE( tmpRes ); */
        }
        ++busy;
    }
    assert( 0 <= busy && busy <= 2 );
    if( 2 == busy )     /* Both directions are unavailable to us */
    {
        result = paDeviceUnavailable;
        goto error;
    }

    PA_UNLESS( *deviceInfo = PaUtil_GroupAllocateMemory( ossApi->allocations, sizeof (PaDeviceInfo) ), paInsufficientMemory );
    PA_ENSURE( PaUtil_InitializeDeviceInfo( *deviceInfo, deviceName, ossApi->hostApiIndex, maxInputChannels, maxOutputChannels,
                defaultLowInputLatency, defaultLowOutputLatency, defaultHighInputLatency, defaultHighOutputLatency, sampleRate,
                ossApi->allocations ) );

error:
    return result;
}

/** Query host devices.
 *
 * Loop over host devices and query their capabilitiesu
 *
 * Aspect DeviceCapabilities: This function calls QueryDevice on each device entry and receives a filled in PaDeviceInfo object
 * per device, these are placed in the host api representation's deviceInfos array.
 */
static PaError BuildDeviceList( PaOSSHostApiRepresentation *ossApi )
{
    PaError result = paNoError;
    PaUtilHostApiRepresentation *commonApi = &ossApi->inheritedHostApiRep;
    int i;
    int numDevices = 0, maxDeviceInfos = 1;
    PaDeviceInfo **deviceInfos = NULL;

    /* These two will be set to the first working input and output device, respectively */
    commonApi->info.defaultInputDevice = paNoDevice;
    commonApi->info.defaultOutputDevice = paNoDevice;

    /* Find devices by calling QueryDevice on each
     * potential device names.  When we find a valid one,
     * add it to a linked list.
     * A: Set an arbitrary of 100 devices, should probably be a smarter way. */

    for( i = 0; i < 100; i++ )
    {
       char deviceName[32];
       PaDeviceInfo *deviceInfo;
       int testResult;
       struct stat stbuf;

       if( i == 0 )
          snprintf(deviceName, sizeof (deviceName), "%s", DEVICE_NAME_BASE);
       else
          snprintf(deviceName, sizeof (deviceName), "%s%d", DEVICE_NAME_BASE, i);

       /* PA_DEBUG(("PaOSS BuildDeviceList: trying device %s\n", deviceName )); */
       if( stat( deviceName, &stbuf ) < 0 )
       {
           if( ENOENT != errno )
               PA_DEBUG(( "%s: Error stat'ing %s: %s\n", __FUNCTION__, deviceName, strerror( errno ) ));
           continue;
       }
       if( (testResult = QueryDevice( deviceName, ossApi, &deviceInfo )) != paNoError )
       {
           if( testResult != paDeviceUnavailable )
               PA_ENSURE( testResult );

           continue;
       }

       ++numDevices;
       if( !deviceInfos || numDevices > maxDeviceInfos )
       {
           maxDeviceInfos *= 2;
           PA_UNLESS( deviceInfos = (PaDeviceInfo **) realloc( deviceInfos, maxDeviceInfos * sizeof (PaDeviceInfo *) ),
                   paInsufficientMemory );
       }
       {
           int devIdx = numDevices - 1;
           deviceInfos[devIdx] = deviceInfo;

           if( commonApi->info.defaultInputDevice == paNoDevice && deviceInfo->maxInputChannels > 0 )
               commonApi->info.defaultInputDevice = devIdx;
           if( commonApi->info.defaultOutputDevice == paNoDevice && deviceInfo->maxOutputChannels > 0 )
               commonApi->info.defaultOutputDevice = devIdx;
       }
    }

    /* Make an array of PaDeviceInfo pointers out of the linked list */

    PA_DEBUG(("PaOSS %s: Total number of devices found: %d\n", __FUNCTION__, numDevices));

    commonApi->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory(
        ossApi->allocations, sizeof(PaDeviceInfo*) * numDevices );
    memcpy( commonApi->deviceInfos, deviceInfos, numDevices * sizeof (PaDeviceInfo *) );

    commonApi->info.deviceCount = numDevices;

error:
    free( deviceInfos );

    return result;
}

static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaOSSHostApiRepresentation *ossHostApi = (PaOSSHostApiRepresentation*)hostApi;

    if( ossHostApi->allocations )
    {
        PaUtil_FreeAllAllocations( ossHostApi->allocations );
        PaUtil_DestroyAllocationGroup( ossHostApi->allocations );
    }

    PaUtil_FreeMemory( ossHostApi );
}

static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    PaError result = paNoError;
    PaDeviceIndex device;
    PaDeviceInfo *deviceInfo;
    char *deviceName;
    int inputChannelCount, outputChannelCount;
    int tempDevHandle = -1;
    int flags;
    PaSampleFormat inputSampleFormat, outputSampleFormat;

    if( inputParameters )
    {
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if( inputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that input device can support inputChannelCount */
        if( inputChannelCount > hostApi->deviceInfos[ inputParameters->device ]->maxInputChannels )
            return paInvalidChannelCount;

        /* validate inputStreamInfo */
        if( inputParameters->hostApiSpecificStreamInfo )
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */
    }
    else
    {
        inputChannelCount = 0;
    }

    if( outputParameters )
    {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if( outputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that output device can support inputChannelCount */
        if( outputChannelCount > hostApi->deviceInfos[ outputParameters->device ]->maxOutputChannels )
            return paInvalidChannelCount;

        /* validate outputStreamInfo */
        if( outputParameters->hostApiSpecificStreamInfo )
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */
    }
    else
    {
        outputChannelCount = 0;
    }

    if (inputChannelCount == 0 && outputChannelCount == 0)
        return paInvalidChannelCount;

    /* if full duplex, make sure that they're the same device */

    if (inputChannelCount > 0 && outputChannelCount > 0 &&
        inputParameters->device != outputParameters->device)
        return paInvalidDevice;

    /* if full duplex, also make sure that they're the same number of channels */

    if (inputChannelCount > 0 && outputChannelCount > 0 &&
        inputChannelCount != outputChannelCount)
       return paInvalidChannelCount;

    /* open the device so we can do more tests */

    if( inputChannelCount > 0 )
    {
        result = PaUtil_DeviceIndexToHostApiDeviceIndex(&device, inputParameters->device, hostApi);
        if (result != paNoError)
            return result;
    }
    else
    {
        result = PaUtil_DeviceIndexToHostApiDeviceIndex(&device, outputParameters->device, hostApi);
        if (result != paNoError)
            return result;
    }

    deviceInfo = hostApi->deviceInfos[device];
    deviceName = (char *)deviceInfo->name;

    flags = O_NONBLOCK;
    if (inputChannelCount > 0 && outputChannelCount > 0)
       flags |= O_RDWR;
    else if (inputChannelCount > 0)
       flags |= O_RDONLY;
    else
       flags |= O_WRONLY;

    ENSURE_( tempDevHandle = open( deviceInfo->name, flags ), paDeviceUnavailable );

    /* PaOssStream_Configure will do the rest of the checking for us */
    /* PA_ENSURE( PaOssStream_Configure( tempDevHandle, deviceName, outputChannelCount, &sampleRate ) ); */

    /* everything succeeded! */

 error:
    if( tempDevHandle >= 0 )
        close( tempDevHandle );

    return result;
}

/** Validate stream parameters.
 *
 * Aspect StreamChannels: We verify that the number of channels is within the allowed range for the device
 */
static PaError ValidateParameters( const PaStreamParameters *parameters, const PaDeviceInfo *deviceInfo, StreamMode mode )
{
    int maxChans;

    assert( parameters );

    if( parameters->device == paUseHostApiSpecificDeviceSpecification )
    {
        return paInvalidDevice;
    }

    maxChans = (mode == StreamMode_In ? deviceInfo->maxInputChannels :
        deviceInfo->maxOutputChannels);
    if( parameters->channelCount > maxChans )
    {
        return paInvalidChannelCount;
    }

    return paNoError;
}

static PaError PaOssStreamComponent_Initialize( PaOssStreamComponent *component, const PaStreamParameters *parameters,
        int callbackMode, int fd, const char *deviceName )
{
    PaError result = paNoError;
    assert( component );

    memset( component, 0, sizeof (PaOssStreamComponent) );

    component->fd = fd;
    component->devName = deviceName;
    component->userChannelCount = parameters->channelCount;
    component->userFormat = parameters->sampleFormat;
    component->latency = parameters->suggestedLatency;
    component->userInterleaved = !(parameters->sampleFormat & paNonInterleaved);

    if( !callbackMode && !component->userInterleaved )
    {
        /* Pre-allocate non-interleaved user provided buffers */
        PA_UNLESS( component->userBuffers = PaUtil_AllocateMemory( sizeof (void *) * component->userChannelCount ),
                paInsufficientMemory );
    }

error:
    return result;
}

static void PaOssStreamComponent_Terminate( PaOssStreamComponent *component )
{
    assert( component );

    if( component->fd >= 0 )
        close( component->fd );
    if( component->buffer )
        PaUtil_FreeMemory( component->buffer );

    if( component->userBuffers )
        PaUtil_FreeMemory( component->userBuffers );

    PaUtil_FreeMemory( component );
}

static PaError ModifyBlocking( int fd, int blocking )
{
    PaError result = paNoError;
    int fflags;

    ENSURE_( fflags = fcntl( fd, F_GETFL ), paUnanticipatedHostError );

    if( blocking )
        fflags &= ~O_NONBLOCK;
    else
        fflags |= O_NONBLOCK;

    ENSURE_( fcntl( fd, F_SETFL, fflags ), paUnanticipatedHostError );

error:
    return result;
}

static PaError OpenDevices( const char *idevName, const char *odevName, int *idev, int *odev )
{
    PaError result = paNoError;
    int flags = O_NONBLOCK, duplex = 0;
    int enableBits = 0;
    *idev = *odev = -1;

    if( idevName && odevName )
    {
        duplex = 1;
        flags |= O_RDWR;
    }
    else if( idevName )
        flags |= O_RDONLY;
    else
        flags |= O_WRONLY;

    /* open first in nonblocking mode, in case it's busy...
     * A: then unset the non-blocking attribute */
    assert( flags & O_NONBLOCK );
    if( idevName )
    {
        ENSURE_( *idev = open( idevName, flags ), paDeviceUnavailable );
        PA_ENSURE( ModifyBlocking( *idev, 1 ) ); /* Blocking */

        /* Initially disable */
        enableBits = ~PCM_ENABLE_INPUT;
        ENSURE_( ioctl( *idev, SNDCTL_DSP_SETTRIGGER, &enableBits ), paUnanticipatedHostError );
    }
    if( odevName )
    {
        if( !idevName )
        {
            ENSURE_( *odev = open( odevName, flags ), paDeviceUnavailable );
            PA_ENSURE( ModifyBlocking( *odev, 1 ) ); /* Blocking */

            /* Initially disable */
            enableBits = ~PCM_ENABLE_OUTPUT;
            ENSURE_( ioctl( *odev, SNDCTL_DSP_SETTRIGGER, &enableBits ), paUnanticipatedHostError );
        }
        else
        {
            ENSURE_( *odev = dup( *idev ), paUnanticipatedHostError );
        }
    }

error:
    return result;
}

static PaError PaOssStream_Initialize( PaOssStream *stream, const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters,
        PaStreamCallback callback, void *userData, PaStreamFlags streamFlags,
        PaOSSHostApiRepresentation *ossApi )
{
    PaError result = paNoError;
    int idev, odev;
    PaUtilHostApiRepresentation *hostApi = &ossApi->inheritedHostApiRep;
    const char *idevName = NULL, *odevName = NULL;

    assert( stream );

    memset( stream, 0, sizeof (PaOssStream) );
    stream->isStopped = 1;

    PA_ENSURE( PaUtil_InitializeThreading( &stream->threading ) );

    if( inputParameters && outputParameters )
    {
        if( inputParameters->device == outputParameters->device )
            stream->sharedDevice = 1;
    }

    if( inputParameters )
        idevName = hostApi->deviceInfos[inputParameters->device]->name;
    if( outputParameters )
        odevName = hostApi->deviceInfos[outputParameters->device]->name;
    PA_ENSURE( OpenDevices( idevName, odevName, &idev, &odev ) );
    if( inputParameters )
    {
        PA_UNLESS( stream->capture = PaUtil_AllocateMemory( sizeof (PaOssStreamComponent) ), paInsufficientMemory );
        PA_ENSURE( PaOssStreamComponent_Initialize( stream->capture, inputParameters, callback != NULL, idev, idevName ) );
    }
    if( outputParameters )
    {
        PA_UNLESS( stream->playback = PaUtil_AllocateMemory( sizeof (PaOssStreamComponent) ), paInsufficientMemory );
        PA_ENSURE( PaOssStreamComponent_Initialize( stream->playback, outputParameters, callback != NULL, odev, odevName ) );
    }

    if( callback != NULL )
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &ossApi->callbackStreamInterface, callback, userData );
        stream->callbackMode = 1;
    }
    else
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &ossApi->blockingStreamInterface, callback, userData );
    }

    ENSURE_( sem_init( &stream->semaphore, 0, 0 ), paInternalError );

error:
    return result;
}

static void PaOssStream_Terminate( PaOssStream *stream )
{
    assert( stream );

    PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );
    PaUtil_TerminateThreading( &stream->threading );

    if( stream->capture )
        PaOssStreamComponent_Terminate( stream->capture );
    if( stream->playback )
        PaOssStreamComponent_Terminate( stream->playback );

    sem_destroy( &stream->semaphore );

    PaUtil_FreeMemory( stream );
}

/** Translate from PA format to OSS native.
 *
 */
static PaError Pa2OssFormat( PaSampleFormat paFormat, int *ossFormat )
{
    switch( paFormat )
    {
        case paUInt8:
            *ossFormat = AFMT_U8;
            break;
        case paInt8:
            *ossFormat = AFMT_S8;
            break;
        case paInt16:
            *ossFormat = AFMT_S16_NE;
            break;
        default:
            return paInternalError;     /* This shouldn't happen */
    }

    return paNoError;
}

/** Return the PA-compatible formats that this device can support.
 *
 */
static PaError GetAvailableFormats( PaOssStreamComponent *component, PaSampleFormat *availableFormats )
{
    PaError result = paNoError;
    int mask = 0;
    PaSampleFormat frmts = 0;

    ENSURE_( ioctl( component->fd, SNDCTL_DSP_GETFMTS, &mask ), paUnanticipatedHostError );
    if( mask & AFMT_U8 )
        frmts |= paUInt8;
    if( mask & AFMT_S8 )
        frmts |= paInt8;
    if( mask & AFMT_S16_NE )
        frmts |= paInt16;
    else
        result = paSampleFormatNotSupported;

    *availableFormats = frmts;

error:
    return result;
}

static unsigned int PaOssStreamComponent_FrameSize( PaOssStreamComponent *component )
{
    return Pa_GetSampleSize( component->hostFormat ) * component->hostChannelCount;
}

/** Buffer size in bytes.
 *
 */
static unsigned long PaOssStreamComponent_BufferSize( PaOssStreamComponent *component )
{
    return PaOssStreamComponent_FrameSize( component ) * component->hostFrames * component->numBufs;
}

static int CalcHigherLogTwo( int n )
{
    int log2 = 0;
    while( (1<<log2) < n ) log2++;
    return log2;
}

static PaError PaOssStreamComponent_Configure( PaOssStreamComponent *component, double sampleRate, unsigned long
        framesPerBuffer, StreamMode streamMode, PaOssStreamComponent *master )
{
    PaError result = paNoError;
    int temp, nativeFormat;
    int sr = (int)sampleRate;
    PaSampleFormat availableFormats = 0, hostFormat = 0;
    int chans = component->userChannelCount;
    int frgmt;
    int numBufs;
    int bytesPerBuf;
    unsigned long bufSz;
    unsigned long fragSz;
    audio_buf_info bufInfo;

    /* We may have a situation where only one component (the master) is configured, if both point to the same device.
     * In that case, the second component will copy settings from the other */
    if( !master )
    {
        /* Aspect BufferSettings: If framesPerBuffer is unspecified we have to infer a suitable fragment size.
         * The hardware need not respect the requested fragment size, so we may have to adapt.
         */
        if( framesPerBuffer == paFramesPerBufferUnspecified )
        {
            bufSz = (unsigned long)(component->latency * sampleRate);
            fragSz = bufSz / 4;
        }
        else
        {
            fragSz = framesPerBuffer;
            bufSz = (unsigned long)(component->latency * sampleRate) + fragSz; /* Latency + 1 buffer */
        }

        PA_ENSURE( GetAvailableFormats( component, &availableFormats ) );
        hostFormat = PaUtil_SelectClosestAvailableFormat( availableFormats, component->userFormat );

        /* OSS demands at least 2 buffers, and 16 bytes per buffer */
        numBufs = (int)PA_MAX( bufSz / fragSz, 2 );
        bytesPerBuf = PA_MAX( fragSz * Pa_GetSampleSize( hostFormat ) * chans, 16 );

        /* The fragment parameters are encoded like this:
         * Most significant byte: number of fragments
         * Least significant byte: exponent of fragment size (i.e., for 256, 8)
         */
        frgmt = (numBufs << 16) + (CalcHigherLogTwo( bytesPerBuf ) & 0xffff);
        ENSURE_( ioctl( component->fd, SNDCTL_DSP_SETFRAGMENT, &frgmt ), paUnanticipatedHostError );

        /* A: according to the OSS programmer's guide parameters should be set in this order:
         * format, channels, rate */

        /* This format should be deemed good before we get this far */
        PA_ENSURE( Pa2OssFormat( hostFormat, &temp ) );
        nativeFormat = temp;
        ENSURE_( ioctl( component->fd, SNDCTL_DSP_SETFMT, &temp ), paUnanticipatedHostError );
        PA_UNLESS( temp == nativeFormat, paInternalError );

        /* try to set the number of channels */
        ENSURE_( ioctl( component->fd, SNDCTL_DSP_CHANNELS, &chans ), paSampleFormatNotSupported );   /* XXX: Should be paInvalidChannelCount? */
        /* It's possible that the minimum number of host channels is greater than what the user requested */
        PA_UNLESS( chans >= component->userChannelCount, paInvalidChannelCount );

        /* try to set the sample rate */
        ENSURE_( ioctl( component->fd, SNDCTL_DSP_SPEED, &sr ), paInvalidSampleRate );

        /* reject if there's no sample rate within 1% of the one requested */
        if( (fabs( sampleRate - sr ) / sampleRate) > 0.01 )
        {
            PA_DEBUG(("%s: Wanted %f, closest sample rate was %d\n", __FUNCTION__, sampleRate, sr ));
            PA_ENSURE( paInvalidSampleRate );
        }

        ENSURE_( ioctl( component->fd, streamMode == StreamMode_In ? SNDCTL_DSP_GETISPACE : SNDCTL_DSP_GETOSPACE, &bufInfo ),
                paUnanticipatedHostError );
        component->numBufs = bufInfo.fragstotal;

        /* This needs to be the last ioctl call before the first read/write, according to the OSS programmer's guide */
        ENSURE_( ioctl( component->fd, SNDCTL_DSP_GETBLKSIZE, &bytesPerBuf ), paUnanticipatedHostError );

        component->hostFrames = bytesPerBuf / Pa_GetSampleSize( hostFormat ) / chans;
        component->hostChannelCount = chans;
        component->hostFormat = hostFormat;
    }
    else
    {
        component->hostFormat = master->hostFormat;
        component->hostFrames = master->hostFrames;
        component->hostChannelCount = master->hostChannelCount;
        component->numBufs = master->numBufs;
    }

    PA_UNLESS( component->buffer = PaUtil_AllocateMemory( PaOssStreamComponent_BufferSize( component ) ),
            paInsufficientMemory );

error:
    return result;
}

static PaError PaOssStreamComponent_Read( PaOssStreamComponent *component, unsigned long *frames )
{
    PaError result = paNoError;
    size_t len = *frames * PaOssStreamComponent_FrameSize( component );
    ssize_t bytesRead;

    ENSURE_( bytesRead = read( component->fd, component->buffer, len ), paUnanticipatedHostError );
    *frames = bytesRead / PaOssStreamComponent_FrameSize( component );
    /* TODO: Handle condition where number of frames read doesn't equal number of frames requested */

error:
    return result;
}

static PaError PaOssStreamComponent_Write( PaOssStreamComponent *component, unsigned long *frames )
{
    PaError result = paNoError;
    size_t len = *frames * PaOssStreamComponent_FrameSize( component );
    ssize_t bytesWritten;

    ENSURE_( bytesWritten = write( component->fd, component->buffer, len ), paUnanticipatedHostError );
    *frames = bytesWritten / PaOssStreamComponent_FrameSize( component );
    /* TODO: Handle condition where number of frames written doesn't equal number of frames requested */

error:
    return result;
}

/** Configure the stream according to input/output parameters.
 *
 * Aspect StreamChannels: The minimum number of channels supported by the device may exceed that requested by
 * the user, if so we'll record the actual number of host channels and adapt later.
 */
static PaError PaOssStream_Configure( PaOssStream *stream, double sampleRate, unsigned long framesPerBuffer,
        double *inputLatency, double *outputLatency )
{
    PaError result = paNoError;
    int duplex = stream->capture && stream->playback;
    unsigned long framesPerHostBuffer = 0;

    /* We should request full duplex first thing after opening the device */
    if( duplex && stream->sharedDevice )
        ENSURE_( ioctl( stream->capture->fd, SNDCTL_DSP_SETDUPLEX, 0 ), paUnanticipatedHostError );

    if( stream->capture )
    {
        PaOssStreamComponent *component = stream->capture;
        PA_ENSURE( PaOssStreamComponent_Configure( component, sampleRate, framesPerBuffer, StreamMode_In,
                    NULL ) );

        assert( component->hostChannelCount > 0 );
        assert( component->hostFrames > 0 );

        *inputLatency = component->hostFrames * (component->numBufs - 1) / sampleRate;
    }
    if( stream->playback )
    {
        PaOssStreamComponent *component = stream->playback, *master = stream->sharedDevice ? stream->capture : NULL;
        PA_ENSURE( PaOssStreamComponent_Configure( component, sampleRate, framesPerBuffer, StreamMode_Out,
                    master ) );

        assert( component->hostChannelCount > 0 );
        assert( component->hostFrames > 0 );

        *outputLatency = component->hostFrames * (component->numBufs - 1) / sampleRate;
    }

    if( duplex )
        framesPerHostBuffer = PA_MIN( stream->capture->hostFrames, stream->playback->hostFrames );
    else if( stream->capture )
        framesPerHostBuffer = stream->capture->hostFrames;
    else if( stream->playback )
        framesPerHostBuffer = stream->playback->hostFrames;

    stream->framesPerHostBuffer = framesPerHostBuffer;
    stream->pollTimeout = (int) ceil( 1e6 * framesPerHostBuffer / sampleRate );    /* Period in usecs, rounded up */

    stream->sampleRate = stream->streamRepresentation.streamInfo.sampleRate = sampleRate;

error:
    return result;
}

/* see pa_hostapi.h for a list of validity guarantees made about OpenStream parameters */

/** Open a PA OSS stream.
 *
 * Aspect StreamChannels: The number of channels is specified per direction (in/out), and can differ between the
 * two. However, OSS doesn't support separate configuration spaces for capture and playback so if both
 * directions are the same device we will demand the same number of channels. The number of channels can range
 * from 1 to the maximum supported by the device.
 *
 * Aspect BufferSettings: If framesPerBuffer != paFramesPerBufferUnspecified the number of frames per callback
 * must reflect this, in addition the host latency per device should approximate the corresponding
 * suggestedLatency. Based on these constraints we need to determine a number of frames per host buffer that
 * both capture and playback can agree on (they can be different devices), the buffer processor can adapt
 * between host and user buffer size, but the ratio should preferably be integral.
 */
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** s,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData )
{
    PaError result = paNoError;
    PaOSSHostApiRepresentation *ossHostApi = (PaOSSHostApiRepresentation*)hostApi;
    PaOssStream *stream = NULL;
    int inputChannelCount = 0, outputChannelCount = 0;
    PaSampleFormat inputSampleFormat = 0, outputSampleFormat = 0, inputHostFormat = 0, outputHostFormat = 0;
    const PaDeviceInfo *inputDeviceInfo = 0, *outputDeviceInfo = 0;
    int bpInitialized = 0;
    double inLatency = 0., outLatency = 0.;
    int i = 0;

    /* validate platform specific flags */
    if( (streamFlags & paPlatformSpecificFlags) != 0 )
        return paInvalidFlag; /* unexpected platform specific flag */

    if( inputParameters )
    {
        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */
        inputDeviceInfo = hostApi->deviceInfos[inputParameters->device];
        PA_ENSURE( ValidateParameters( inputParameters, inputDeviceInfo, StreamMode_In ) );

        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;
    }
    if( outputParameters )
    {
        outputDeviceInfo = hostApi->deviceInfos[outputParameters->device];
        PA_ENSURE( ValidateParameters( outputParameters, outputDeviceInfo, StreamMode_Out ) );

        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
    }

    /* Aspect StreamChannels: We currently demand that number of input and output channels are the same, if the same
     * device is opened for both directions
     */
    if( inputChannelCount > 0 && outputChannelCount > 0 )
    {
        if( inputParameters->device == outputParameters->device )
        {
            if( inputParameters->channelCount != outputParameters->channelCount )
                return paInvalidChannelCount;
        }
    }

    /* Round framesPerBuffer to the next power-of-two to make OSS happy. */
    if( framesPerBuffer != paFramesPerBufferUnspecified )
    {
        framesPerBuffer &= INT_MAX;
        for (i = 1; framesPerBuffer > i; i <<= 1) ;
        framesPerBuffer = i;
    }

    /* allocate and do basic initialization of the stream structure */
    PA_UNLESS( stream = (PaOssStream*)PaUtil_AllocateMemory( sizeof(PaOssStream) ), paInsufficientMemory );
    PA_ENSURE( PaOssStream_Initialize( stream, inputParameters, outputParameters, streamCallback, userData, streamFlags, ossHostApi ) );

    PA_ENSURE( PaOssStream_Configure( stream, sampleRate, framesPerBuffer, &inLatency, &outLatency ) );

    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, sampleRate );

    if( inputParameters )
    {
        inputHostFormat = stream->capture->hostFormat;
        stream->streamRepresentation.streamInfo.inputLatency = inLatency +
            PaUtil_GetBufferProcessorInputLatency( &stream->bufferProcessor ) / sampleRate;
    }
    if( outputParameters )
    {
        outputHostFormat = stream->playback->hostFormat;
        stream->streamRepresentation.streamInfo.outputLatency = outLatency +
            PaUtil_GetBufferProcessorOutputLatency( &stream->bufferProcessor ) / sampleRate;
    }

    /* Initialize buffer processor with fixed host buffer size.
     * Aspect StreamSampleFormat: Here we commit the user and host sample formats, PA infrastructure will
     * convert between the two.
     */
    PA_ENSURE( PaUtil_InitializeBufferProcessor( &stream->bufferProcessor,
              inputChannelCount, inputSampleFormat, inputHostFormat, outputChannelCount, outputSampleFormat,
              outputHostFormat, sampleRate, streamFlags, framesPerBuffer, stream->framesPerHostBuffer,
              paUtilFixedHostBufferSize, streamCallback, userData ) );
    bpInitialized = 1;

    *s = (PaStream*)stream;

    return result;

error:
    if( bpInitialized )
        PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );
    if( stream )
        PaOssStream_Terminate( stream );

    return result;
}

/*! Poll on I/O filedescriptors.

  Poll till we've determined there's data for read or write. In the full-duplex case,
  we don't want to hang around forever waiting for either input or output frames, so
  whenever we have a timed out filedescriptor we check if we're nearing under/overrun
  for the other direction (critical limit set at one buffer). If so, we exit the waiting
  state, and go on with what we got. We align the number of frames on a host buffer
  boundary because it is possible that the buffer size differs for the two directions and
  the host buffer size is a compromise between the two.
  */
static PaError PaOssStream_WaitForFrames( PaOssStream *stream, unsigned long *frames )
{
    PaError result = paNoError;
    int pollPlayback = 0, pollCapture = 0;
    int captureAvail = INT_MAX, playbackAvail = INT_MAX, commonAvail;
    audio_buf_info bufInfo;
    /* int ofs = 0, nfds = stream->nfds; */
    fd_set readFds, writeFds;
    int nfds = 0;
    struct timeval selectTimeval = {0, 0};
    unsigned long timeout = stream->pollTimeout;    /* In usecs */
    int captureFd = -1, playbackFd = -1;

    assert( stream );
    assert( frames );

    if( stream->capture )
    {
        pollCapture = 1;
        captureFd = stream->capture->fd;
        /* stream->capture->pfd->events = POLLIN; */
    }
    if( stream->playback )
    {
        pollPlayback = 1;
        playbackFd = stream->playback->fd;
        /* stream->playback->pfd->events = POLLOUT; */
    }

    FD_ZERO( &readFds );
    FD_ZERO( &writeFds );

    while( pollPlayback || pollCapture )
    {
        pthread_testcancel();

        /* select may modify the timeout parameter */
        selectTimeval.tv_usec = timeout;
        nfds = 0;

        if( pollCapture )
        {
            FD_SET( captureFd, &readFds );
            nfds = captureFd + 1;
        }
        if( pollPlayback )
        {
            FD_SET( playbackFd, &writeFds );
            nfds = PA_MAX( nfds, playbackFd + 1 );
        }
        ENSURE_( select( nfds, &readFds, &writeFds, NULL, &selectTimeval ), paUnanticipatedHostError );
        /*
        if( poll( stream->pfds + ofs, nfds, stream->pollTimeout ) < 0 )
        {

            ENSURE_( -1, paUnanticipatedHostError );
        }
        */
        pthread_testcancel();

        if( pollCapture )
        {
            if( FD_ISSET( captureFd, &readFds ) )
            {
                FD_CLR( captureFd, &readFds );
                pollCapture = 0;
            }
            /*
            if( stream->capture->pfd->revents & POLLIN )
            {
                --nfds;
                ++ofs;
                pollCapture = 0;
            }
            */
            else if( stream->playback ) /* Timed out, go on with playback? */
            {
                /*PA_DEBUG(( "%s: Trying to poll again for capture frames, pollTimeout: %d\n",
                            __FUNCTION__, stream->pollTimeout ));*/
            }
        }
        if( pollPlayback )
        {
            if( FD_ISSET( playbackFd, &writeFds ) )
            {
                FD_CLR( playbackFd, &writeFds );
                pollPlayback = 0;
            }
            /*
            if( stream->playback->pfd->revents & POLLOUT )
            {
                --nfds;
                pollPlayback = 0;
            }
            */
            else if( stream->capture )  /* Timed out, go on with capture? */
            {
                /*PA_DEBUG(( "%s: Trying to poll again for playback frames, pollTimeout: %d\n\n",
                            __FUNCTION__, stream->pollTimeout ));*/
            }
        }
    }

    if( stream->capture )
    {
        ENSURE_( ioctl( captureFd, SNDCTL_DSP_GETISPACE, &bufInfo ), paUnanticipatedHostError );
        captureAvail = bufInfo.fragments * stream->capture->hostFrames;
        if( !captureAvail )
            PA_DEBUG(( "%s: captureAvail: 0\n", __FUNCTION__ ));

        captureAvail = captureAvail == 0 ? INT_MAX : captureAvail;      /* Disregard if zero */
    }
    if( stream->playback )
    {
        ENSURE_( ioctl( playbackFd, SNDCTL_DSP_GETOSPACE, &bufInfo ), paUnanticipatedHostError );
        playbackAvail = bufInfo.fragments * stream->playback->hostFrames;
        if( !playbackAvail )
        {
            PA_DEBUG(( "%s: playbackAvail: 0\n", __FUNCTION__ ));
        }

        playbackAvail = playbackAvail == 0 ? INT_MAX : playbackAvail;      /* Disregard if zero */
    }

    commonAvail = PA_MIN( captureAvail, playbackAvail );
    if( commonAvail == INT_MAX )
        commonAvail = 0;
    commonAvail -= commonAvail % stream->framesPerHostBuffer;

    assert( commonAvail != INT_MAX );
    assert( commonAvail >= 0 );
    *frames = commonAvail;

error:
    return result;
}

/** Prepare stream for capture/playback.
 *
 * In order to synchronize capture and playback properly we use the SETTRIGGER command.
 */
static PaError PaOssStream_Prepare( PaOssStream *stream )
{
    PaError result = paNoError;
    int enableBits = 0;

    if( stream->triggered )
        return result;

    if( stream->playback )
    {
        size_t bufSz = PaOssStreamComponent_BufferSize( stream->playback );
        memset( stream->playback->buffer, 0, bufSz );

        /* Looks like we have to turn off blocking before we try this, but if we don't fill the buffer
         * OSS will complain. */
        PA_ENSURE( ModifyBlocking( stream->playback->fd, 0 ) );
        while (1)
        {
            if( write( stream->playback->fd, stream->playback->buffer, bufSz ) < 0 )
                break;
        }
        PA_ENSURE( ModifyBlocking( stream->playback->fd, 1 ) );
    }

    if( stream->sharedDevice )
    {
        enableBits = PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT;
        ENSURE_( ioctl( stream->capture->fd, SNDCTL_DSP_SETTRIGGER, &enableBits ), paUnanticipatedHostError );
    }
    else
    {
        if( stream->capture )
        {
            enableBits = PCM_ENABLE_INPUT;
            ENSURE_( ioctl( stream->capture->fd, SNDCTL_DSP_SETTRIGGER, &enableBits ), paUnanticipatedHostError );
        }
        if( stream->playback )
        {
            enableBits = PCM_ENABLE_OUTPUT;
            ENSURE_( ioctl( stream->playback->fd, SNDCTL_DSP_SETTRIGGER, &enableBits ), paUnanticipatedHostError );
        }
    }

    /* Ok, we have triggered the stream */
    stream->triggered = 1;

error:
    return result;
}

/** Stop audio processing
 *
 */
static PaError PaOssStream_Stop( PaOssStream *stream, int abort )
{
    PaError result = paNoError;

    /* Looks like the only safe way to stop audio without reopening the device is SNDCTL_DSP_POST.
     * Also disable capture/playback till the stream is started again */
    if( stream->capture )
    {
        ENSURE_( ioctl( stream->capture->fd, SNDCTL_DSP_POST, 0 ), paUnanticipatedHostError );
    }
    if( stream->playback && !stream->sharedDevice )
    {
        ENSURE_( ioctl( stream->playback->fd, SNDCTL_DSP_POST, 0 ), paUnanticipatedHostError );
    }

error:
    return result;
}

/** Clean up after thread exit.
 *
 * Aspect StreamState: If the user has registered a streamFinishedCallback it will be called here
 */
static void OnExit( void *data )
{
    PaOssStream *stream = (PaOssStream *) data;
    assert( data );

    PaUtil_ResetCpuLoadMeasurer( &stream->cpuLoadMeasurer );

    PaOssStream_Stop( stream, stream->callbackAbort );

    PA_DEBUG(( "OnExit: Stoppage\n" ));

    /* Eventually notify user all buffers have played */
    if( stream->streamRepresentation.streamFinishedCallback )
        stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );

    stream->callbackAbort = 0;      /* Clear state */
    stream->isActive = 0;
}

static PaError SetUpBuffers( PaOssStream *stream, unsigned long framesAvail )
{
    PaError result = paNoError;

    if( stream->capture )
    {
        PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor, 0, stream->capture->buffer,
                stream->capture->hostChannelCount );
        PaUtil_SetInputFrameCount( &stream->bufferProcessor, framesAvail );
    }
    if( stream->playback )
    {
        PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor, 0, stream->playback->buffer,
                stream->playback->hostChannelCount );
        PaUtil_SetOutputFrameCount( &stream->bufferProcessor, framesAvail );
    }

    return result;
}

/** Thread procedure for callback processing.
 *
 * Aspect StreamState: StartStream will wait on this to initiate audio processing, useful in case the
 * callback should be used for buffer priming. When the stream is cancelled a separate function will
 * take care of the transition to the Callback Finished state (the stream isn't considered Stopped
 * before StopStream() or AbortStream() are called).
 */
static void *PaOSS_AudioThreadProc( void *userData )
{
    PaError result = paNoError;
    PaOssStream *stream = (PaOssStream*)userData;
    unsigned long framesAvail = 0, framesProcessed = 0;
    int callbackResult = paContinue;
    int triggered = stream->triggered;  /* See if SNDCTL_DSP_TRIGGER has been issued already */
    int initiateProcessing = triggered;    /* Already triggered? */
    PaStreamCallbackFlags cbFlags = 0;  /* We might want to keep state across iterations */
    PaStreamCallbackTimeInfo timeInfo = {0,0,0}; /* TODO: IMPLEMENT ME */

    /*
#if ( SOUND_VERSION > 0x030904 )
        audio_errinfo errinfo;
#endif
*/

    assert( stream );

    pthread_cleanup_push( &OnExit, stream );	/* Execute OnExit when exiting */

    /* The first time the stream is started we use SNDCTL_DSP_TRIGGER to accurately start capture and
     * playback in sync, when the stream is restarted after being stopped we simply start by reading/
     * writing.
     */
    PA_ENSURE( PaOssStream_Prepare( stream ) );

    /* If we are to initiate processing implicitly by reading/writing data, we start off in blocking mode */
    if( initiateProcessing )
    {
        /* Make sure devices are in blocking mode */
        if( stream->capture )
            ModifyBlocking( stream->capture->fd, 1 );
        if( stream->playback )
            ModifyBlocking( stream->playback->fd, 1 );
    }

    while( 1 )
    {
        pthread_testcancel();

        if( stream->callbackStop && callbackResult == paContinue )
        {
            PA_DEBUG(( "Setting callbackResult to paComplete\n" ));
            callbackResult = paComplete;
        }

        /* Aspect StreamState: Because of the messy OSS scheme we can't explicitly trigger device start unless
         * the stream has been recently started, we will have to go right ahead and read/write in blocking
         * fashion to trigger operation. Therefore we begin with processing one host buffer before we switch
         * to non-blocking mode.
         */
        if( !initiateProcessing )
        {
            /* Wait on available frames */
            PA_ENSURE( PaOssStream_WaitForFrames( stream, &framesAvail ) );
            assert( framesAvail % stream->framesPerHostBuffer == 0 );
        }
        else
        {
            framesAvail = stream->framesPerHostBuffer;
        }

        while( framesAvail > 0 )
        {
            unsigned long frames = framesAvail;

            pthread_testcancel();

            PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );

            /* Read data */
            if ( stream->capture )
            {
                PA_ENSURE( PaOssStreamComponent_Read( stream->capture, &frames ) );
                if( frames < framesAvail )
                {
                    PA_DEBUG(( "Read %lu less frames than requested\n", framesAvail - frames ));
                    framesAvail = frames;
                }
            }

#if ( SOUND_VERSION >= 0x030904 )
            /*
               Check with OSS to see if there have been any under/overruns
               since last time we checked.
               */
            /*
            if( ioctl( stream->deviceHandle, SNDCTL_DSP_GETERROR, &errinfo ) >= 0 )
            {
                if( errinfo.play_underruns )
                    cbFlags |= paOutputUnderflow ;
                if( errinfo.record_underruns )
                    cbFlags |= paInputUnderflow ;
            }
            else
                PA_DEBUG(( "SNDCTL_DSP_GETERROR command failed: %s\n", strerror( errno ) ));
                */
#endif

            PaUtil_BeginBufferProcessing( &stream->bufferProcessor, &timeInfo,
                    cbFlags );
            cbFlags = 0;
            PA_ENSURE( SetUpBuffers( stream, framesAvail ) );

            framesProcessed = PaUtil_EndBufferProcessing( &stream->bufferProcessor,
                    &callbackResult );
            assert( framesProcessed == framesAvail );
            PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, framesProcessed );

            if ( stream->playback )
            {
                frames = framesAvail;

                PA_ENSURE( PaOssStreamComponent_Write( stream->playback, &frames ) );
                if( frames < framesAvail )
                {
                    /* TODO: handle bytesWritten != bytesRequested (slippage?) */
                    PA_DEBUG(( "Wrote %lu less frames than requested\n", framesAvail - frames ));
                }
            }

            framesAvail -= framesProcessed;
            stream->framesProcessed += framesProcessed;

            if( callbackResult != paContinue )
                break;
        }

        if( initiateProcessing || !triggered )
        {
            /* Non-blocking */
            if( stream->capture )
                PA_ENSURE( ModifyBlocking( stream->capture->fd, 0 ) );
            if( stream->playback && !stream->sharedDevice )
                PA_ENSURE( ModifyBlocking( stream->playback->fd, 0 ) );

            initiateProcessing = 0;
            sem_post( &stream->semaphore );
        }

        if( callbackResult != paContinue )
        {
            stream->callbackAbort = callbackResult == paAbort;
            if( stream->callbackAbort || PaUtil_IsBufferProcessorOutputEmpty( &stream->bufferProcessor ) )
                break;
        }
    }

    pthread_cleanup_pop( 1 );

error:
    pthread_exit( NULL );
}

/** Close the stream.
 *
 */
static PaError CloseStream( PaStream* s )
{
    PaError result = paNoError;
    PaOssStream *stream = (PaOssStream*)s;

    assert( stream );

    PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );
    PaOssStream_Terminate( stream );

    return result;
}

/** Start the stream.
 *
 * Aspect StreamState: After returning, the stream shall be in the Active state, implying that an eventual
 * callback will be repeatedly called in a separate thread. If a separate thread is started this function
 * will block untill it has started processing audio, otherwise audio processing is started directly.
 */
static PaError StartStream( PaStream *s )
{
    PaError result = paNoError;
    PaOssStream *stream = (PaOssStream*)s;

    stream->isActive = 1;
    stream->isStopped = 0;
    stream->lastPosPtr = 0;
    stream->lastStreamBytes = 0;
    stream->framesProcessed = 0;

    /* only use the thread for callback streams */
    if( stream->bufferProcessor.streamCallback )
    {
        PA_ENSURE( PaUtil_StartThreading( &stream->threading, &PaOSS_AudioThreadProc, stream ) );
        sem_wait( &stream->semaphore );
    }
    else
        PA_ENSURE( PaOssStream_Prepare( stream ) );

error:
    return result;
}

static PaError RealStop( PaOssStream *stream, int abort )
{
    PaError result = paNoError;

    if( stream->callbackMode )
    {
        if( abort )
            stream->callbackAbort = 1;
        else
            stream->callbackStop = 1;

        PA_ENSURE( PaUtil_CancelThreading( &stream->threading, !abort, NULL ) );

        stream->callbackStop = stream->callbackAbort = 0;
    }
    else
        PA_ENSURE( PaOssStream_Stop( stream, abort ) );

    stream->isStopped = 1;

error:
    return result;
}

/** Stop the stream.
 *
 * Aspect StreamState: This will cause the stream to transition to the Stopped state, playing all enqueued
 * buffers.
 */
static PaError StopStream( PaStream *s )
{
    return RealStop( (PaOssStream *)s, 0 );
}

/** Abort the stream.
 *
 * Aspect StreamState: This will cause the stream to transition to the Stopped state, discarding all enqueued
 * buffers. Note that the buffers are not currently correctly discarded, this is difficult without closing
 * the OSS device.
 */
static PaError AbortStream( PaStream *s )
{
    return RealStop( (PaOssStream *)s, 1 );
}

/** Is the stream in the Stopped state.
 *
 */
static PaError IsStreamStopped( PaStream *s )
{
    PaOssStream *stream = (PaOssStream*)s;

    return (stream->isStopped);
}

/** Is the stream in the Active state.
 *
 */
static PaError IsStreamActive( PaStream *s )
{
    PaOssStream *stream = (PaOssStream*)s;

    return (stream->isActive);
}

static PaTime GetStreamTime( PaStream *s )
{
    PaOssStream *stream = (PaOssStream*)s;
    count_info info;
    int delta;

    if( stream->playback ) {
        if( ioctl( stream->playback->fd, SNDCTL_DSP_GETOPTR, &info) == 0 ) {
            delta = ( info.bytes - stream->lastPosPtr ) /* & 0x000FFFFF*/;
            return (float)(stream->lastStreamBytes + delta) / PaOssStreamComponent_FrameSize( stream->playback ) / stream->sampleRate;
        }
    }
    else {
        if (ioctl( stream->capture->fd, SNDCTL_DSP_GETIPTR, &info) == 0) {
            delta = (info.bytes - stream->lastPosPtr) /*& 0x000FFFFF*/;
            return (float)(stream->lastStreamBytes + delta) / PaOssStreamComponent_FrameSize( stream->capture ) / stream->sampleRate;
        }
    }

    /* the ioctl failed, but we can still give a coarse estimate */

    return stream->framesProcessed / stream->sampleRate;
}


static double GetStreamCpuLoad( PaStream* s )
{
    PaOssStream *stream = (PaOssStream*)s;

    return PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer );
}


/*
    As separate stream interfaces are used for blocking and callback
    streams, the following functions can be guaranteed to only be called
    for blocking streams.
*/


static PaError ReadStream( PaStream* s,
                           void *buffer,
                           unsigned long frames )
{
    PaOssStream *stream = (PaOssStream*)s;
    int bytesRequested, bytesRead;
    unsigned long framesRequested;
    void *userBuffer;

    /* If user input is non-interleaved, PaUtil_CopyInput will manipulate the channel pointers,
     * so we copy the user provided pointers */
    if( stream->bufferProcessor.userInputIsInterleaved )
        userBuffer = buffer;
    else /* Copy channels into local array */
    {
        userBuffer = stream->capture->userBuffers;
        memcpy( (void *)userBuffer, buffer, sizeof (void *) * stream->capture->userChannelCount );
    }

    while( frames )
    {
        framesRequested = PA_MIN( frames, stream->capture->hostFrames );

	bytesRequested = framesRequested * PaOssStreamComponent_FrameSize( stream->capture );
	bytesRead = read( stream->capture->fd, stream->capture->buffer, bytesRequested );
	if ( bytesRequested != bytesRead )
	    return paUnanticipatedHostError;

	PaUtil_SetInputFrameCount( &stream->bufferProcessor, stream->capture->hostFrames );
	PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor, 0, stream->capture->buffer, stream->capture->hostChannelCount );
        PaUtil_CopyInput( &stream->bufferProcessor, &userBuffer, framesRequested );
	frames -= framesRequested;
    }
    return paNoError;
}


static PaError WriteStream( PaStream *s, const void *buffer, unsigned long frames )
{
    PaOssStream *stream = (PaOssStream*)s;
    int bytesRequested, bytesWritten;
    unsigned long framesConverted;
    const void *userBuffer;

    /* If user output is non-interleaved, PaUtil_CopyOutput will manipulate the channel pointers,
     * so we copy the user provided pointers */
    if( stream->bufferProcessor.userOutputIsInterleaved )
        userBuffer = buffer;
    else
    {
        /* Copy channels into local array */
        userBuffer = stream->playback->userBuffers;
        memcpy( (void *)userBuffer, buffer, sizeof (void *) * stream->playback->userChannelCount );
    }

    while( frames )
    {
	PaUtil_SetOutputFrameCount( &stream->bufferProcessor, stream->playback->hostFrames );
	PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor, 0, stream->playback->buffer, stream->playback->hostChannelCount );

	framesConverted = PaUtil_CopyOutput( &stream->bufferProcessor, &userBuffer, frames );
	frames -= framesConverted;

	bytesRequested = framesConverted * PaOssStreamComponent_FrameSize( stream->playback );
	bytesWritten = write( stream->playback->fd, stream->playback->buffer, bytesRequested );

	if ( bytesRequested != bytesWritten )
	    return paUnanticipatedHostError;
    }
    return paNoError;
}


static signed long GetStreamReadAvailable( PaStream* s )
{
    PaOssStream *stream = (PaOssStream*)s;
    audio_buf_info info;

    if( ioctl( stream->capture->fd, SNDCTL_DSP_GETISPACE, &info ) < 0 )
        return paUnanticipatedHostError;
    return info.fragments * stream->capture->hostFrames;
}


/* TODO: Compute number of allocated bytes somewhere else, can we use ODELAY with capture */
static signed long GetStreamWriteAvailable( PaStream* s )
{
    PaOssStream *stream = (PaOssStream*)s;
    int delay = 0;

    if( ioctl( stream->playback->fd, SNDCTL_DSP_GETODELAY, &delay ) < 0 )
        return paUnanticipatedHostError;

    return (PaOssStreamComponent_BufferSize( stream->playback ) - delay) / PaOssStreamComponent_FrameSize( stream->playback );
}

