/*
 * $Id: pa_mac_core_old.c 1083 2006-08-23 07:30:49Z rossb $
 * pa_mac_core.c
 * Implementation of PortAudio for Mac OS X CoreAudio       
 *                                                                                         
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 *
 * Authors: Ross Bencina and Phil Burk
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

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "portaudio.h"
#include "pa_trace.h"
#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"

// =====  constants  =====

// =====  structs  =====
#pragma mark structs

// PaMacCoreHostApiRepresentation - host api datastructure specific to this implementation
typedef struct PaMacCore_HAR
{
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;
    
    PaUtilAllocationGroup *allocations;
    AudioDeviceID *macCoreDeviceIds;
}
PaMacCoreHostApiRepresentation;

typedef struct PaMacCore_DI
{
    PaDeviceInfo inheritedDeviceInfo;
}
PaMacCoreDeviceInfo;

// PaMacCoreStream - a stream data structure specifically for this implementation
typedef struct PaMacCore_S
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;
    
    int primeStreamUsingCallback;
    
    AudioDeviceID inputDevice;
    AudioDeviceID outputDevice;
    
    // Processing thread management --------------
//    HANDLE abortEvent;
//    HANDLE processingThread;
//    DWORD processingThreadId;
    
    char throttleProcessingThreadOnOverload; // 0 -> don't throtte, non-0 -> throttle
    int processingThreadPriority;
    int highThreadPriority;
    int throttledThreadPriority;
    unsigned long throttledSleepMsecs;
    
    int isStopped;
    volatile int isActive;
    volatile int stopProcessing; // stop thread once existing buffers have been returned
    volatile int abortProcessing; // stop thread immediately
    
//    DWORD allBuffersDurationMs; // used to calculate timeouts
}
PaMacCoreStream;

// Data needed by the CoreAudio callback functions
typedef struct PaMacCore_CD
{
    PaMacCoreStream *stream;
    PaStreamCallback *callback;
    void *userData;
    PaUtilConverter *inputConverter;
    PaUtilConverter *outputConverter;
    void *inputBuffer;
    void *outputBuffer;
    int inputChannelCount;
    int outputChannelCount;
    PaSampleFormat inputSampleFormat;
    PaSampleFormat outputSampleFormat;
    PaUtilTriangularDitherGenerator *ditherGenerator;
}
PaMacClientData;

// =====  CoreAudio-PortAudio bridge functions =====
#pragma mark CoreAudio-PortAudio bridge functions

// Maps CoreAudio OSStatus codes to PortAudio PaError codes
static PaError conv_err(OSStatus error)
{
    PaError result;
    
    switch (error) {
        case kAudioHardwareNoError:
            result = paNoError; break;
        case kAudioHardwareNotRunningError:
            result = paInternalError; break;
        case kAudioHardwareUnspecifiedError:
            result = paInternalError; break;
        case kAudioHardwareUnknownPropertyError:
            result = paInternalError; break;
        case kAudioHardwareBadPropertySizeError:
            result = paInternalError; break;
        case kAudioHardwareIllegalOperationError:
            result = paInternalError; break;
        case kAudioHardwareBadDeviceError:
            result = paInvalidDevice; break;
        case kAudioHardwareBadStreamError:
            result = paBadStreamPtr; break;
        case kAudioHardwareUnsupportedOperationError:
            result = paInternalError; break;
        case kAudioDeviceUnsupportedFormatError:
            result = paSampleFormatNotSupported; break;
        case kAudioDevicePermissionsError:
            result = paDeviceUnavailable; break;
        default:
            result = paInternalError;
    }
    
    return result;
}

/* This function is unused
static AudioStreamBasicDescription *InitializeStreamDescription(const PaStreamParameters *parameters, double sampleRate)
{
    struct AudioStreamBasicDescription *streamDescription = PaUtil_AllocateMemory(sizeof(AudioStreamBasicDescription));
    streamDescription->mSampleRate = sampleRate;
    streamDescription->mFormatID = kAudioFormatLinearPCM;
    streamDescription->mFormatFlags = 0;
    streamDescription->mFramesPerPacket = 1;
    
    if (parameters->sampleFormat & paNonInterleaved) {
        streamDescription->mFormatFlags |= kLinearPCMFormatFlagIsNonInterleaved;
        streamDescription->mChannelsPerFrame = 1;
        streamDescription->mBytesPerFrame = Pa_GetSampleSize(parameters->sampleFormat);
        streamDescription->mBytesPerPacket = Pa_GetSampleSize(parameters->sampleFormat);
    }
    else {
        streamDescription->mChannelsPerFrame = parameters->channelCount;
    }
    
    streamDescription->mBytesPerFrame = Pa_GetSampleSize(parameters->sampleFormat) * streamDescription->mChannelsPerFrame;
    streamDescription->mBytesPerPacket = streamDescription->mBytesPerFrame * streamDescription->mFramesPerPacket;
    
    if (parameters->sampleFormat & paFloat32) {
        streamDescription->mFormatFlags |= kLinearPCMFormatFlagIsFloat;
        streamDescription->mBitsPerChannel = 32;
    }
    else if (parameters->sampleFormat & paInt32) {
        streamDescription->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
        streamDescription->mBitsPerChannel = 32;
    }
    else if (parameters->sampleFormat & paInt24) {
        streamDescription->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
        streamDescription->mBitsPerChannel = 24;
    }
    else if (parameters->sampleFormat & paInt16) {
        streamDescription->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
        streamDescription->mBitsPerChannel = 16;
    }
    else if (parameters->sampleFormat & paInt8) {
        streamDescription->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
        streamDescription->mBitsPerChannel = 8;
    }    
    else if (parameters->sampleFormat & paInt32) {
        streamDescription->mBitsPerChannel = 8;
    }
    
    return streamDescription;
}
*/

static PaStreamCallbackTimeInfo *InitializeTimeInfo(const AudioTimeStamp* now, const AudioTimeStamp* inputTime, const AudioTimeStamp* outputTime)
{
    PaStreamCallbackTimeInfo *timeInfo = PaUtil_AllocateMemory(sizeof(PaStreamCallbackTimeInfo));
    
    timeInfo->inputBufferAdcTime = inputTime->mSampleTime;
    timeInfo->currentTime = now->mSampleTime;
    timeInfo->outputBufferDacTime = outputTime->mSampleTime;
    
    return timeInfo;
}

// =====  support functions  =====
#pragma mark support functions

static void CleanUp(PaMacCoreHostApiRepresentation *macCoreHostApi)
{
    if( macCoreHostApi->allocations )
    {
        PaUtil_FreeAllAllocations( macCoreHostApi->allocations );
        PaUtil_DestroyAllocationGroup( macCoreHostApi->allocations );
    }
    
    PaUtil_FreeMemory( macCoreHostApi );    
}

static PaError GetChannelInfo(PaDeviceInfo *deviceInfo, AudioDeviceID macCoreDeviceId, int isInput)
{
    UInt32 propSize;
    PaError err = paNoError;
    UInt32 i;
    int numChannels = 0;
    AudioBufferList *buflist;

    err = conv_err(AudioDeviceGetPropertyInfo(macCoreDeviceId, 0, isInput, kAudioDevicePropertyStreamConfiguration, &propSize, NULL));
    buflist = PaUtil_AllocateMemory(propSize);
    err = conv_err(AudioDeviceGetProperty(macCoreDeviceId, 0, isInput, kAudioDevicePropertyStreamConfiguration, &propSize, buflist));
    if (!err) {
        for (i = 0; i < buflist->mNumberBuffers; ++i) {
            numChannels += buflist->mBuffers[i].mNumberChannels;
        }
		
		if (isInput)
			deviceInfo->maxInputChannels = numChannels;
		else
			deviceInfo->maxOutputChannels = numChannels;
		
        int frameLatency;
        propSize = sizeof(UInt32);
        err = conv_err(AudioDeviceGetProperty(macCoreDeviceId, 0, isInput, kAudioDevicePropertyLatency, &propSize, &frameLatency));
        if (!err) {
            double secondLatency = frameLatency / deviceInfo->defaultSampleRate;
            if (isInput) {
                deviceInfo->defaultLowInputLatency = secondLatency;
                deviceInfo->defaultHighInputLatency = secondLatency;
            }
            else {
                deviceInfo->defaultLowOutputLatency = secondLatency;
                deviceInfo->defaultHighOutputLatency = secondLatency;
            }
        }
    }
    PaUtil_FreeMemory(buflist);
    
    return err;
}

static PaError InitializeDeviceInfo(PaMacCoreDeviceInfo *macCoreDeviceInfo,  AudioDeviceID macCoreDeviceId, PaHostApiIndex hostApiIndex )
{
    PaDeviceInfo *deviceInfo = &macCoreDeviceInfo->inheritedDeviceInfo;
    deviceInfo->structVersion = 2;
    deviceInfo->hostApi = hostApiIndex;
    
    PaError err = paNoError;
    UInt32 propSize;

    err = conv_err(AudioDeviceGetPropertyInfo(macCoreDeviceId, 0, 0, kAudioDevicePropertyDeviceName, &propSize, NULL));
    // FIXME: this allocation should be part of the allocations group
    char *name = PaUtil_AllocateMemory(propSize);
    err = conv_err(AudioDeviceGetProperty(macCoreDeviceId, 0, 0, kAudioDevicePropertyDeviceName, &propSize, name));
    if (!err) {
        deviceInfo->name = name;
    }
    
    Float64 sampleRate;
    propSize = sizeof(Float64);
    err = conv_err(AudioDeviceGetProperty(macCoreDeviceId, 0, 0, kAudioDevicePropertyNominalSampleRate, &propSize, &sampleRate));
    if (!err) {
        deviceInfo->defaultSampleRate = sampleRate;
    }


    // Get channel info
    err = GetChannelInfo(deviceInfo, macCoreDeviceId, 1);
    err = GetChannelInfo(deviceInfo, macCoreDeviceId, 0);

    return err;
}

static PaError InitializeDeviceInfos( PaMacCoreHostApiRepresentation *macCoreHostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    PaUtilHostApiRepresentation *hostApi;
    PaMacCoreDeviceInfo *deviceInfoArray;

    // initialise device counts and default devices under the assumption that there are no devices. These values are incremented below if and when devices are successfully initialized.
    hostApi = &macCoreHostApi->inheritedHostApiRep;
    hostApi->info.deviceCount = 0;
    hostApi->info.defaultInputDevice = paNoDevice;
    hostApi->info.defaultOutputDevice = paNoDevice;
    
    UInt32 propsize;
    AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &propsize, NULL);
    int numDevices = propsize / sizeof(AudioDeviceID);
    hostApi->info.deviceCount = numDevices;
    if (numDevices > 0) {
        hostApi->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory(
                                            macCoreHostApi->allocations, sizeof(PaDeviceInfo*) * numDevices );
        if( !hostApi->deviceInfos )
        {
            return paInsufficientMemory;
        }

        // allocate all device info structs in a contiguous block
        deviceInfoArray = (PaMacCoreDeviceInfo*)PaUtil_GroupAllocateMemory(
                                macCoreHostApi->allocations, sizeof(PaMacCoreDeviceInfo) * numDevices );
        if( !deviceInfoArray )
        {
            return paInsufficientMemory;
        }
        
        macCoreHostApi->macCoreDeviceIds = PaUtil_GroupAllocateMemory(macCoreHostApi->allocations, propsize);
        AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &propsize, macCoreHostApi->macCoreDeviceIds);

        AudioDeviceID defaultInputDevice, defaultOutputDevice;
        propsize = sizeof(AudioDeviceID);
        AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &propsize, &defaultInputDevice);
        AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &propsize, &defaultOutputDevice);
        
        UInt32 i;
        for (i = 0; i < numDevices; ++i) {
            if (macCoreHostApi->macCoreDeviceIds[i] == defaultInputDevice) {
                hostApi->info.defaultInputDevice = i;
            }
            if (macCoreHostApi->macCoreDeviceIds[i] == defaultOutputDevice) {
                hostApi->info.defaultOutputDevice = i;
            }
            InitializeDeviceInfo(&deviceInfoArray[i], macCoreHostApi->macCoreDeviceIds[i], hostApiIndex);
            hostApi->deviceInfos[i] = &(deviceInfoArray[i].inheritedDeviceInfo);      
        }
    }

    return result;
}

static OSStatus CheckFormat(AudioDeviceID macCoreDeviceId, const PaStreamParameters *parameters, double sampleRate, int isInput)
{
    UInt32 propSize = sizeof(AudioStreamBasicDescription);
    AudioStreamBasicDescription *streamDescription = PaUtil_AllocateMemory(propSize);

    streamDescription->mSampleRate = sampleRate;
    streamDescription->mFormatID = 0;
    streamDescription->mFormatFlags = 0;
    streamDescription->mBytesPerPacket = 0;
    streamDescription->mFramesPerPacket = 0;
    streamDescription->mBytesPerFrame = 0;
    streamDescription->mChannelsPerFrame = 0;
    streamDescription->mBitsPerChannel = 0;
    streamDescription->mReserved = 0;

    OSStatus result = AudioDeviceGetProperty(macCoreDeviceId, 0, isInput, kAudioDevicePropertyStreamFormatSupported, &propSize, streamDescription);
    PaUtil_FreeMemory(streamDescription);
    return result;
}

static OSStatus CopyInputData(PaMacClientData* destination, const AudioBufferList *source, unsigned long frameCount)
{
    int frameSpacing, channelSpacing;
    if (destination->inputSampleFormat & paNonInterleaved) {
        frameSpacing = 1;
        channelSpacing = destination->inputChannelCount;
    }
    else {
        frameSpacing = destination->inputChannelCount;
        channelSpacing = 1;
    }
    
    AudioBuffer const *inputBuffer = &source->mBuffers[0];
    void *coreAudioBuffer = inputBuffer->mData;
    void *portAudioBuffer = destination->inputBuffer;
    UInt32 i, streamNumber, streamChannel;
    for (i = streamNumber = streamChannel = 0; i < destination->inputChannelCount; ++i, ++streamChannel) {
        if (streamChannel >= inputBuffer->mNumberChannels) {
            ++streamNumber;
            inputBuffer = &source->mBuffers[streamNumber];
            coreAudioBuffer = inputBuffer->mData;
            streamChannel = 0;
        }
        destination->inputConverter(portAudioBuffer, frameSpacing, coreAudioBuffer, inputBuffer->mNumberChannels, frameCount, destination->ditherGenerator);
        coreAudioBuffer += sizeof(Float32);
        portAudioBuffer += Pa_GetSampleSize(destination->inputSampleFormat) * channelSpacing;
    }
    return noErr;
}

static OSStatus CopyOutputData(AudioBufferList* destination, PaMacClientData *source, unsigned long frameCount)
{
    int frameSpacing, channelSpacing;
    if (source->outputSampleFormat & paNonInterleaved) {
        frameSpacing = 1;
        channelSpacing = source->outputChannelCount;
    }
    else {
        frameSpacing = source->outputChannelCount;
        channelSpacing = 1;
    }
    
    AudioBuffer *outputBuffer = &destination->mBuffers[0];
    void *coreAudioBuffer = outputBuffer->mData;
    void *portAudioBuffer = source->outputBuffer;
    UInt32 i, streamNumber, streamChannel;
    for (i = streamNumber = streamChannel = 0; i < source->outputChannelCount; ++i, ++streamChannel) {
        if (streamChannel >= outputBuffer->mNumberChannels) {
            ++streamNumber;
            outputBuffer = &destination->mBuffers[streamNumber];
            coreAudioBuffer = outputBuffer->mData;
            streamChannel = 0;
        }
        source->outputConverter(coreAudioBuffer, outputBuffer->mNumberChannels, portAudioBuffer, frameSpacing, frameCount, NULL);
        coreAudioBuffer += sizeof(Float32);
        portAudioBuffer += Pa_GetSampleSize(source->outputSampleFormat) * channelSpacing;
    }
    return noErr;
}

static OSStatus AudioIOProc( AudioDeviceID inDevice,
                      const AudioTimeStamp* inNow,
                      const AudioBufferList* inInputData,
                      const AudioTimeStamp* inInputTime,
                      AudioBufferList* outOutputData, 
                      const AudioTimeStamp* inOutputTime,
                      void* inClientData)
{
    PaMacClientData *clientData = (PaMacClientData *)inClientData;
    PaStreamCallbackTimeInfo *timeInfo = InitializeTimeInfo(inNow, inInputTime, inOutputTime);
    
    PaUtil_BeginCpuLoadMeasurement( &clientData->stream->cpuLoadMeasurer );
    
    AudioBuffer *outputBuffer = &outOutputData->mBuffers[0];
    unsigned long frameCount = outputBuffer->mDataByteSize / (outputBuffer->mNumberChannels * sizeof(Float32));

    if (clientData->inputBuffer) {
        CopyInputData(clientData, inInputData, frameCount);
    }
    PaStreamCallbackResult result = clientData->callback(clientData->inputBuffer, clientData->outputBuffer, frameCount, timeInfo, paNoFlag, clientData->userData);
    if (clientData->outputBuffer) {
        CopyOutputData(outOutputData, clientData, frameCount);
    }

    PaUtil_EndCpuLoadMeasurement( &clientData->stream->cpuLoadMeasurer, frameCount );
    
    if (result == paComplete || result == paAbort) {
        Pa_StopStream(clientData->stream);
    }

    PaUtil_FreeMemory( timeInfo );
    return noErr;
}

// This is not for input-only streams, this is for streams where the input device is different from the output device
static OSStatus AudioInputProc( AudioDeviceID inDevice,
                         const AudioTimeStamp* inNow,
                         const AudioBufferList* inInputData,
                         const AudioTimeStamp* inInputTime,
                         AudioBufferList* outOutputData, 
                         const AudioTimeStamp* inOutputTime,
                         void* inClientData)
{
    PaMacClientData *clientData = (PaMacClientData *)inClientData;
    PaStreamCallbackTimeInfo *timeInfo = InitializeTimeInfo(inNow, inInputTime, inOutputTime);

    PaUtil_BeginCpuLoadMeasurement( &clientData->stream->cpuLoadMeasurer );

    AudioBuffer const *inputBuffer = &inInputData->mBuffers[0];
    unsigned long frameCount = inputBuffer->mDataByteSize / (inputBuffer->mNumberChannels * sizeof(Float32));

    CopyInputData(clientData, inInputData, frameCount);
    PaStreamCallbackResult result = clientData->callback(clientData->inputBuffer, clientData->outputBuffer, frameCount, timeInfo, paNoFlag, clientData->userData);
    
    PaUtil_EndCpuLoadMeasurement( &clientData->stream->cpuLoadMeasurer, frameCount );
    if( result == paComplete || result == paAbort )
       Pa_StopStream(clientData->stream);
    PaUtil_FreeMemory( timeInfo );
    return noErr;
}

// This is not for output-only streams, this is for streams where the input device is different from the output device
static OSStatus AudioOutputProc( AudioDeviceID inDevice,
                          const AudioTimeStamp* inNow,
                          const AudioBufferList* inInputData,
                          const AudioTimeStamp* inInputTime,
                          AudioBufferList* outOutputData, 
                          const AudioTimeStamp* inOutputTime,
                          void* inClientData)
{
    PaMacClientData *clientData = (PaMacClientData *)inClientData;
    //PaStreamCallbackTimeInfo *timeInfo = InitializeTimeInfo(inNow, inInputTime, inOutputTime);

    PaUtil_BeginCpuLoadMeasurement( &clientData->stream->cpuLoadMeasurer );

    AudioBuffer *outputBuffer = &outOutputData->mBuffers[0];
    unsigned long frameCount = outputBuffer->mDataByteSize / (outputBuffer->mNumberChannels * sizeof(Float32));

    //clientData->callback(NULL, clientData->outputBuffer, frameCount, timeInfo, paNoFlag, clientData->userData);

    CopyOutputData(outOutputData, clientData, frameCount);

    PaUtil_EndCpuLoadMeasurement( &clientData->stream->cpuLoadMeasurer, frameCount );
    return noErr;
}

static PaError SetSampleRate(AudioDeviceID device, double sampleRate, int isInput)
{
    PaError result = paNoError;
    
    double actualSampleRate;
    UInt32 propSize = sizeof(double);
    result = conv_err(AudioDeviceSetProperty(device, NULL, 0, isInput, kAudioDevicePropertyNominalSampleRate, propSize, &sampleRate));
    
    result = conv_err(AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyNominalSampleRate, &propSize, &actualSampleRate));
    
    if (result == paNoError && actualSampleRate != sampleRate) {
        result = paInvalidSampleRate;
    }
    
    return result;    
}

static PaError SetFramesPerBuffer(AudioDeviceID device, unsigned long framesPerBuffer, int isInput)
{
    PaError result = paNoError;
    UInt32 preferredFramesPerBuffer = framesPerBuffer;
    //    while (preferredFramesPerBuffer > UINT32_MAX) {
    //        preferredFramesPerBuffer /= 2;
    //    }
    
    UInt32 actualFramesPerBuffer;
    UInt32 propSize = sizeof(UInt32);
    result = conv_err(AudioDeviceSetProperty(device, NULL, 0, isInput, kAudioDevicePropertyBufferFrameSize, propSize, &preferredFramesPerBuffer));
    
    result = conv_err(AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyBufferFrameSize, &propSize, &actualFramesPerBuffer));
    
    if (result != paNoError) {
        // do nothing
    }
    else if (actualFramesPerBuffer > framesPerBuffer) {
        result = paBufferTooSmall;
    }
    else if (actualFramesPerBuffer < framesPerBuffer) {
        result = paBufferTooBig;
    }
    
    return result;    
}
    
static PaError SetUpUnidirectionalStream(AudioDeviceID device, double sampleRate, unsigned long framesPerBuffer, int isInput)
{
    PaError err = paNoError;
    err = SetSampleRate(device, sampleRate, isInput);
    if( err == paNoError )
        err = SetFramesPerBuffer(device, framesPerBuffer, isInput);
    return err;
}

// =====  PortAudio functions  =====
#pragma mark PortAudio functions

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
    
    PaError PaMacCore_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
    
#ifdef __cplusplus
}
#endif // __cplusplus

static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaMacCoreHostApiRepresentation *macCoreHostApi = (PaMacCoreHostApiRepresentation*)hostApi;
    
    CleanUp(macCoreHostApi);
}

static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    PaMacCoreHostApiRepresentation *macCoreHostApi = (PaMacCoreHostApiRepresentation*)hostApi;
    PaDeviceInfo *deviceInfo;
    
    PaError result = paNoError;
    if (inputParameters) {
        deviceInfo = macCoreHostApi->inheritedHostApiRep.deviceInfos[inputParameters->device];
        if (inputParameters->channelCount > deviceInfo->maxInputChannels)
            result = paInvalidChannelCount;
        else if (CheckFormat(macCoreHostApi->macCoreDeviceIds[inputParameters->device], inputParameters, sampleRate, 1) != kAudioHardwareNoError) {
            result = paInvalidSampleRate;
        }
    }
    if (outputParameters && result == paNoError) {
        deviceInfo = macCoreHostApi->inheritedHostApiRep.deviceInfos[outputParameters->device];
        if (outputParameters->channelCount > deviceInfo->maxOutputChannels)
            result = paInvalidChannelCount;
        else if (CheckFormat(macCoreHostApi->macCoreDeviceIds[outputParameters->device], outputParameters, sampleRate, 0) != kAudioHardwareNoError) {
            result = paInvalidSampleRate;
        }
    }

    return result;
}

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
    PaError err = paNoError;
    PaMacCoreHostApiRepresentation *macCoreHostApi = (PaMacCoreHostApiRepresentation *)hostApi;
    PaMacCoreStream *stream = PaUtil_AllocateMemory(sizeof(PaMacCoreStream));
    stream->isActive = 0;
    stream->isStopped = 1;
    stream->inputDevice = kAudioDeviceUnknown;
    stream->outputDevice = kAudioDeviceUnknown;
    
    PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                           ( (streamCallback)
                                             ? &macCoreHostApi->callbackStreamInterface
                                             : &macCoreHostApi->blockingStreamInterface ),
                                           streamCallback, userData );
    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, sampleRate );
    
    *s = (PaStream*)stream;
    PaMacClientData *clientData = PaUtil_AllocateMemory(sizeof(PaMacClientData));
    clientData->stream = stream;
    clientData->callback = streamCallback;
    clientData->userData = userData;
    clientData->inputBuffer = 0;
    clientData->outputBuffer = 0;
    clientData->ditherGenerator = PaUtil_AllocateMemory(sizeof(PaUtilTriangularDitherGenerator));
    PaUtil_InitializeTriangularDitherState(clientData->ditherGenerator);
    
    if (inputParameters != NULL) {
        stream->inputDevice = macCoreHostApi->macCoreDeviceIds[inputParameters->device];
        clientData->inputConverter = PaUtil_SelectConverter(paFloat32, inputParameters->sampleFormat, streamFlags);
        clientData->inputBuffer = PaUtil_AllocateMemory(Pa_GetSampleSize(inputParameters->sampleFormat) * framesPerBuffer * inputParameters->channelCount);
        clientData->inputChannelCount = inputParameters->channelCount;
        clientData->inputSampleFormat = inputParameters->sampleFormat;
        err = SetUpUnidirectionalStream(stream->inputDevice, sampleRate, framesPerBuffer, 1);
    }
    
    if (err == paNoError && outputParameters != NULL) {
        stream->outputDevice = macCoreHostApi->macCoreDeviceIds[outputParameters->device];
        clientData->outputConverter = PaUtil_SelectConverter(outputParameters->sampleFormat, paFloat32, streamFlags);
        clientData->outputBuffer = PaUtil_AllocateMemory(Pa_GetSampleSize(outputParameters->sampleFormat) * framesPerBuffer * outputParameters->channelCount);
        clientData->outputChannelCount = outputParameters->channelCount;
        clientData->outputSampleFormat = outputParameters->sampleFormat;
        err = SetUpUnidirectionalStream(stream->outputDevice, sampleRate, framesPerBuffer, 0);
    }

    if (inputParameters == NULL || outputParameters == NULL || stream->inputDevice == stream->outputDevice) {
        AudioDeviceID device = (inputParameters == NULL) ? stream->outputDevice : stream->inputDevice;

        AudioDeviceAddIOProc(device, AudioIOProc, clientData);
    }
    else {
        // using different devices for input and output
        AudioDeviceAddIOProc(stream->inputDevice, AudioInputProc, clientData);
        AudioDeviceAddIOProc(stream->outputDevice, AudioOutputProc, clientData);
    }
    
    return err;
}

// Note: When CloseStream() is called, the multi-api layer ensures that the stream has already been stopped or aborted.
static PaError CloseStream( PaStream* s )
{
    PaError err = paNoError;
    PaMacCoreStream *stream = (PaMacCoreStream*)s;

    PaUtil_ResetCpuLoadMeasurer( &stream->cpuLoadMeasurer );

    if (stream->inputDevice != kAudioDeviceUnknown) {
        if (stream->outputDevice == kAudioDeviceUnknown || stream->outputDevice == stream->inputDevice) {
            err = conv_err(AudioDeviceRemoveIOProc(stream->inputDevice, AudioIOProc));
        }
        else {
            err = conv_err(AudioDeviceRemoveIOProc(stream->inputDevice, AudioInputProc));
            err = conv_err(AudioDeviceRemoveIOProc(stream->outputDevice, AudioOutputProc));
        }
    }
    else {
        err = conv_err(AudioDeviceRemoveIOProc(stream->outputDevice, AudioIOProc));
    }
    
    return err;
}


static PaError StartStream( PaStream *s )
{
    PaError err = paNoError;
    PaMacCoreStream *stream = (PaMacCoreStream*)s;

    if (stream->inputDevice != kAudioDeviceUnknown) {
        if (stream->outputDevice == kAudioDeviceUnknown || stream->outputDevice == stream->inputDevice) {
            err = conv_err(AudioDeviceStart(stream->inputDevice, AudioIOProc));
        }
        else {
            err = conv_err(AudioDeviceStart(stream->inputDevice, AudioInputProc));
            err = conv_err(AudioDeviceStart(stream->outputDevice, AudioOutputProc));
        }
    }
    else {
        err = conv_err(AudioDeviceStart(stream->outputDevice, AudioIOProc));
    }
    
    stream->isActive = 1;
    stream->isStopped = 0;
    return err;
}

static PaError AbortStream( PaStream *s )
{
    PaError err = paNoError;
    PaMacCoreStream *stream = (PaMacCoreStream*)s;
    
    if (stream->inputDevice != kAudioDeviceUnknown) {
        if (stream->outputDevice == kAudioDeviceUnknown || stream->outputDevice == stream->inputDevice) {
            err = conv_err(AudioDeviceStop(stream->inputDevice, AudioIOProc));
        }
        else {
            err = conv_err(AudioDeviceStop(stream->inputDevice, AudioInputProc));
            err = conv_err(AudioDeviceStop(stream->outputDevice, AudioOutputProc));
        }
    }
    else {
        err = conv_err(AudioDeviceStop(stream->outputDevice, AudioIOProc));
    }
    
    stream->isActive = 0;
    stream->isStopped = 1;
    return err;
}    

static PaError StopStream( PaStream *s )
{
    // TODO: this should be nicer than abort
    return AbortStream(s);
}

static PaError IsStreamStopped( PaStream *s )
{
    PaMacCoreStream *stream = (PaMacCoreStream*)s;
    
    return stream->isStopped;
}


static PaError IsStreamActive( PaStream *s )
{
    PaMacCoreStream *stream = (PaMacCoreStream*)s;

    return stream->isActive;
}


static PaTime GetStreamTime( PaStream *s )
{
    OSStatus err;
    PaTime result;
    PaMacCoreStream *stream = (PaMacCoreStream*)s;

    AudioTimeStamp *timeStamp = PaUtil_AllocateMemory(sizeof(AudioTimeStamp));
    if (stream->inputDevice != kAudioDeviceUnknown) {
        err = AudioDeviceGetCurrentTime(stream->inputDevice, timeStamp);
    }
    else {
        err = AudioDeviceGetCurrentTime(stream->outputDevice, timeStamp);
    }
    
    result = err ? 0 : timeStamp->mSampleTime;
    PaUtil_FreeMemory(timeStamp);

    return result;
}


static double GetStreamCpuLoad( PaStream* s )
{
    PaMacCoreStream *stream = (PaMacCoreStream*)s;
    
    return PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer );
}


// As separate stream interfaces are used for blocking and callback streams, the following functions can be guaranteed to only be called for blocking streams.

static PaError ReadStream( PaStream* s,
                           void *buffer,
                           unsigned long frames )
{
    return paInternalError;
}


static PaError WriteStream( PaStream* s,
                            const void *buffer,
                            unsigned long frames )
{
    return paInternalError;
}


static signed long GetStreamReadAvailable( PaStream* s )
{
    return paInternalError;
}


static signed long GetStreamWriteAvailable( PaStream* s )
{
    return paInternalError;
}

// HostAPI-specific initialization function
PaError PaMacCore_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    PaMacCoreHostApiRepresentation *macCoreHostApi = (PaMacCoreHostApiRepresentation *)PaUtil_AllocateMemory( sizeof(PaMacCoreHostApiRepresentation) );
    if( !macCoreHostApi )
    {
        result = paInsufficientMemory;
        goto error;
    }
    
    macCoreHostApi->allocations = PaUtil_CreateAllocationGroup();
    if( !macCoreHostApi->allocations )
    {
        result = paInsufficientMemory;
        goto error;
    }
    
    *hostApi = &macCoreHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;
    (*hostApi)->info.type = paCoreAudio;
    (*hostApi)->info.name = "CoreAudio";

    result = InitializeDeviceInfos(macCoreHostApi, hostApiIndex);
    if (result != paNoError) {
        goto error;
    }
    
    // Set up the proper callbacks to this HostApi's functions
    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;
    
    PaUtil_InitializeStreamInterface( &macCoreHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );
    
    PaUtil_InitializeStreamInterface( &macCoreHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );
    
    return result;
    
error:
        if( macCoreHostApi ) {
            CleanUp(macCoreHostApi);
        }
    
    return result;
}
