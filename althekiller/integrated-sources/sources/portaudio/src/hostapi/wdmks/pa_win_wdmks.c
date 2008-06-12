/*
 * $Id: pa_win_wdmks.c 1263 2007-08-27 22:59:09Z rossb $
 * PortAudio Windows WDM-KS interface
 *
 * Author: Andrew Baldwin
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2004 Andrew Baldwin, Ross Bencina, Phil Burk
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
 @ingroup hostaip_src
 @brief Portaudio WDM-KS host API.

 @note This is the implementation of the Portaudio host API using the
 Windows WDM/Kernel Streaming API in order to enable very low latency
 playback and recording on all modern Windows platforms (e.g. 2K, XP)
 Note: This API accesses the device drivers below the usual KMIXER
 component which is normally used to enable multi-client mixing and
 format conversion. That means that it will lock out all other users
 of a device for the duration of active stream using those devices
*/

#include <stdio.h>

/* Debugging/tracing support */

#define PA_LOGE_
#define PA_LOGL_

#ifdef __GNUC__
    #include <initguid.h>
    #define _WIN32_WINNT 0x0501
    #define WINVER 0x0501
#endif

#include <string.h> /* strlen() */
#include <assert.h>

#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "portaudio.h"
#include "pa_debugprint.h"

#include <windows.h>
#include <winioctl.h>


#ifdef __GNUC__
    #undef PA_LOGE_
    #define PA_LOGE_ PA_DEBUG(("%s {\n",__FUNCTION__))
    #undef PA_LOGL_
    #define PA_LOGL_ PA_DEBUG(("} %s\n",__FUNCTION__))
    /* These defines are set in order to allow the WIndows DirectX
     * headers to compile with a GCC compiler such as MinGW
     * NOTE: The headers may generate a few warning in GCC, but
     * they should compile */
    #define _INC_MMSYSTEM
    #define _INC_MMREG
    #define _NTRTL_ /* Turn off default definition of DEFINE_GUIDEX */
    #define DEFINE_GUID_THUNK(name,guid) DEFINE_GUID(name,guid)
    #define DEFINE_GUIDEX(n) DEFINE_GUID_THUNK( n, STATIC_##n )
    #if !defined( DEFINE_WAVEFORMATEX_GUID )
        #define DEFINE_WAVEFORMATEX_GUID(x) (USHORT)(x), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
    #endif
    #define  WAVE_FORMAT_ADPCM      0x0002
    #define  WAVE_FORMAT_IEEE_FLOAT 0x0003
    #define  WAVE_FORMAT_ALAW       0x0006
    #define  WAVE_FORMAT_MULAW      0x0007
    #define  WAVE_FORMAT_MPEG       0x0050
    #define  WAVE_FORMAT_DRM        0x0009
    #define DYNAMIC_GUID_THUNK(l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
    #define DYNAMIC_GUID(data) DYNAMIC_GUID_THUNK(data)
#endif

#ifdef _MSC_VER
    #define DYNAMIC_GUID(data) {data}
    #define _INC_MMREG
    #define _NTRTL_ /* Turn off default definition of DEFINE_GUIDEX */
    #undef DEFINE_GUID
    #define DEFINE_GUID(n,data) EXTERN_C const GUID n = {data}
    #define DEFINE_GUID_THUNK(n,data) DEFINE_GUID(n,data)
    #define DEFINE_GUIDEX(n) DEFINE_GUID_THUNK(n, STATIC_##n)
    #if !defined( DEFINE_WAVEFORMATEX_GUID )
        #define DEFINE_WAVEFORMATEX_GUID(x) (USHORT)(x), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
    #endif
    #define  WAVE_FORMAT_ADPCM      0x0002
    #define  WAVE_FORMAT_IEEE_FLOAT 0x0003
    #define  WAVE_FORMAT_ALAW       0x0006
    #define  WAVE_FORMAT_MULAW      0x0007
    #define  WAVE_FORMAT_MPEG       0x0050
    #define  WAVE_FORMAT_DRM        0x0009
#endif

#include <ks.h>
#include <ksmedia.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>

/* These next definitions allow the use of the KSUSER DLL */
typedef KSDDKAPI DWORD WINAPI KSCREATEPIN(HANDLE, PKSPIN_CONNECT, ACCESS_MASK, PHANDLE);
extern HMODULE      DllKsUser;
extern KSCREATEPIN* FunctionKsCreatePin;

/* Forward definition to break circular type reference between pin and filter */
struct __PaWinWdmFilter;
typedef struct __PaWinWdmFilter PaWinWdmFilter;

/* The Pin structure
 * A pin is an input or output node, e.g. for audio flow */
typedef struct __PaWinWdmPin
{
    HANDLE                      handle;
    PaWinWdmFilter*             parentFilter;
    unsigned long               pinId;
    KSPIN_CONNECT*              pinConnect;
    unsigned long               pinConnectSize;
    KSDATAFORMAT_WAVEFORMATEX*  ksDataFormatWfx;
    KSPIN_COMMUNICATION         communication;
    KSDATARANGE*                dataRanges;
    KSMULTIPLE_ITEM*            dataRangesItem;
    KSPIN_DATAFLOW              dataFlow;
    KSPIN_CINSTANCES            instances;
    unsigned long               frameSize;
    int                         maxChannels;
    unsigned long               formats;
    int                         bestSampleRate;
}
PaWinWdmPin;

/* The Filter structure
 * A filter has a number of pins and a "friendly name" */
struct __PaWinWdmFilter
{
    HANDLE         handle;
    int            pinCount;
    PaWinWdmPin**  pins;
    TCHAR          filterName[MAX_PATH];
    TCHAR          friendlyName[MAX_PATH];
    int            maxInputChannels;
    int            maxOutputChannels;
    unsigned long  formats;
    int            usageCount;
    int            bestSampleRate;
};

/* PaWinWdmHostApiRepresentation - host api datastructure specific to this implementation */
typedef struct __PaWinWdmHostApiRepresentation
{
    PaUtilHostApiRepresentation  inheritedHostApiRep;
    PaUtilStreamInterface        callbackStreamInterface;
    PaUtilStreamInterface        blockingStreamInterface;

    PaUtilAllocationGroup*       allocations;
    PaWinWdmFilter**             filters;
    int                          filterCount;
}
PaWinWdmHostApiRepresentation;

typedef struct __PaWinWdmDeviceInfo
{
    PaDeviceInfo     inheritedDeviceInfo;
    PaWinWdmFilter*  filter;
}
PaWinWdmDeviceInfo;

typedef struct __DATAPACKET
{
    KSSTREAM_HEADER  Header;
    OVERLAPPED       Signal;
} DATAPACKET;

/* PaWinWdmStream - a stream data structure specifically for this implementation */
typedef struct __PaWinWdmStream
{
    PaUtilStreamRepresentation  streamRepresentation;
    PaUtilCpuLoadMeasurer       cpuLoadMeasurer;
    PaUtilBufferProcessor       bufferProcessor;

    PaWinWdmPin*                recordingPin;
    PaWinWdmPin*                playbackPin;
    char*                       hostBuffer;
    unsigned long               framesPerHostIBuffer;
    unsigned long               framesPerHostOBuffer;
    int                         bytesPerInputFrame;
    int                         bytesPerOutputFrame;
    int                         streamStarted;
    int                         streamActive;
    int                         streamStop;
    int                         streamAbort;
    int                         oldProcessPriority;
    HANDLE                      streamThread;
    HANDLE                      events[5];  /* 2 play + 2 record packets + abort events */
    DATAPACKET                  packets[4]; /* 2 play + 2 record */
    PaStreamFlags               streamFlags;
    /* These values handle the case where the user wants to use fewer
     * channels than the device has */
    int                         userInputChannels;
    int                         deviceInputChannels;
    int                         userOutputChannels;
    int                         deviceOutputChannels;
    int                         inputSampleSize;
    int                         outputSampleSize;
}
PaWinWdmStream;

#include <setupapi.h>

HMODULE      DllKsUser = NULL;
KSCREATEPIN* FunctionKsCreatePin = NULL;

/* prototypes for functions declared in this file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

PaError PaWinWdm_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Low level I/O functions */
static PaError WdmSyncIoctl(HANDLE handle,
    unsigned long ioctlNumber,
    void* inBuffer,
    unsigned long inBufferCount,
    void* outBuffer,
    unsigned long outBufferCount,
    unsigned long* bytesReturned);
static PaError WdmGetPropertySimple(HANDLE handle,
    const GUID* const guidPropertySet,
    unsigned long property,
    void* value,
    unsigned long valueCount,
    void* instance,
    unsigned long instanceCount);
static PaError WdmSetPropertySimple(HANDLE handle,
    const GUID* const guidPropertySet,
    unsigned long property,
    void* value,
    unsigned long valueCount,
    void* instance,
    unsigned long instanceCount);
static PaError WdmGetPinPropertySimple(HANDLE  handle,
    unsigned long pinId,
    const GUID* const guidPropertySet,
    unsigned long property,
    void* value,
    unsigned long valueCount);
static PaError WdmGetPinPropertyMulti(HANDLE  handle,
    unsigned long pinId,
    const GUID* const guidPropertySet,
    unsigned long property,
    KSMULTIPLE_ITEM** ksMultipleItem);

/** Pin management functions */
static PaWinWdmPin* PinNew(PaWinWdmFilter* parentFilter, unsigned long pinId, PaError* error);
static void PinFree(PaWinWdmPin* pin);
static void PinClose(PaWinWdmPin* pin);
static PaError PinInstantiate(PaWinWdmPin* pin);
/*static PaError PinGetState(PaWinWdmPin* pin, KSSTATE* state); NOT USED */
static PaError PinSetState(PaWinWdmPin* pin, KSSTATE state);
static PaError PinSetFormat(PaWinWdmPin* pin, const WAVEFORMATEX* format);
static PaError PinIsFormatSupported(PaWinWdmPin* pin, const WAVEFORMATEX* format);

/* Filter management functions */
static PaWinWdmFilter* FilterNew(
    TCHAR* filterName,
    TCHAR* friendlyName,
    PaError* error);
static void FilterFree(PaWinWdmFilter* filter);
static PaWinWdmPin* FilterCreateRenderPin(
    PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error);
static PaWinWdmPin* FilterFindViableRenderPin(
    PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error);
static PaError FilterCanCreateRenderPin(
    PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex);
static PaWinWdmPin* FilterCreateCapturePin(
    PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error);
static PaWinWdmPin* FilterFindViableCapturePin(
    PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error);
static PaError FilterCanCreateCapturePin(
    PaWinWdmFilter* filter,
    const WAVEFORMATEX* pwfx);
static PaError FilterUse(
    PaWinWdmFilter* filter);
static void FilterRelease(
    PaWinWdmFilter* filter);

/* Interface functions */
static void Terminate( struct PaUtilHostApiRepresentation *hostApi );
static PaError IsFormatSupported(
    struct PaUtilHostApiRepresentation *hostApi,
    const PaStreamParameters *inputParameters,
    const PaStreamParameters *outputParameters,
    double sampleRate );
static PaError OpenStream(
    struct PaUtilHostApiRepresentation *hostApi,
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
static PaError ReadStream(
    PaStream* stream,
    void *buffer,
    unsigned long frames );
static PaError WriteStream(
    PaStream* stream,
    const void *buffer,
    unsigned long frames );
static signed long GetStreamReadAvailable( PaStream* stream );
static signed long GetStreamWriteAvailable( PaStream* stream );

/* Utility functions */
static unsigned long GetWfexSize(const WAVEFORMATEX* wfex);
static PaError BuildFilterList(PaWinWdmHostApiRepresentation* wdmHostApi);
static BOOL PinWrite(HANDLE h, DATAPACKET* p);
static BOOL PinRead(HANDLE h, DATAPACKET* p);
static void DuplicateFirstChannelInt16(void* buffer, int channels, int samples);
static void DuplicateFirstChannelInt24(void* buffer, int channels, int samples);
static DWORD WINAPI ProcessingThread(LPVOID pParam);

/* Function bodies */

static unsigned long GetWfexSize(const WAVEFORMATEX* wfex)
{
    if( wfex->wFormatTag == WAVE_FORMAT_PCM )
    {
        return sizeof( WAVEFORMATEX );
    }
    else
    {
        return (sizeof( WAVEFORMATEX ) + wfex->cbSize);
    }
}

/*
Low level pin/filter access functions
*/
static PaError WdmSyncIoctl(
    HANDLE handle,
    unsigned long ioctlNumber,
    void* inBuffer,
    unsigned long inBufferCount,
    void* outBuffer,
    unsigned long outBufferCount,
    unsigned long* bytesReturned)
{
    PaError result = paNoError;
    OVERLAPPED overlapped;
    int boolResult;
    unsigned long dummyBytesReturned;
    unsigned long error;

    if( !bytesReturned )
    {
        /* User a dummy as the caller hasn't supplied one */
        bytesReturned = &dummyBytesReturned;
    }

    FillMemory((void *)&overlapped,sizeof(overlapped),0);
    overlapped.hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if( !overlapped.hEvent )
    {
          result = paInsufficientMemory;
        goto error;
    }
    overlapped.hEvent = (HANDLE)((DWORD_PTR)overlapped.hEvent | 0x1);

    boolResult = DeviceIoControl(handle, ioctlNumber, inBuffer, inBufferCount,
        outBuffer, outBufferCount, bytesReturned, &overlapped);
    if( !boolResult )
    {
        error = GetLastError();
        if( error == ERROR_IO_PENDING )
        {
            error = WaitForSingleObject(overlapped.hEvent,INFINITE);
            if( error != WAIT_OBJECT_0 )
            {
                result = paUnanticipatedHostError;
                goto error;
            }
        }
        else if((( error == ERROR_INSUFFICIENT_BUFFER ) ||
                  ( error == ERROR_MORE_DATA )) &&
                  ( ioctlNumber == IOCTL_KS_PROPERTY ) &&
                  ( outBufferCount == 0 ))
        {
            boolResult = TRUE;
        }
        else
        {
            result = paUnanticipatedHostError;
        }
    }
    if( !boolResult )
        *bytesReturned = 0;

error:
    if( overlapped.hEvent )
    {
            CloseHandle( overlapped.hEvent );
    }
    return result;
}

static PaError WdmGetPropertySimple(HANDLE handle,
    const GUID* const guidPropertySet,
    unsigned long property,
    void* value,
    unsigned long valueCount,
    void* instance,
    unsigned long instanceCount)
{
    PaError result;
    KSPROPERTY* ksProperty;
    unsigned long propertyCount;

    propertyCount = sizeof(KSPROPERTY) + instanceCount;
    ksProperty = (KSPROPERTY*)PaUtil_AllocateMemory( propertyCount );
    if( !ksProperty )
    {
        return paInsufficientMemory;
    }

    FillMemory((void*)ksProperty,sizeof(ksProperty),0);
    ksProperty->Set = *guidPropertySet;
    ksProperty->Id = property;
    ksProperty->Flags = KSPROPERTY_TYPE_GET;

    if( instance )
    {
        memcpy( (void*)(((char*)ksProperty)+sizeof(KSPROPERTY)), instance, instanceCount );
    }

    result = WdmSyncIoctl(
                handle,
                IOCTL_KS_PROPERTY,
                ksProperty,
                propertyCount,
                value,
                valueCount,
                NULL);

    PaUtil_FreeMemory( ksProperty );
    return result;
}

static PaError WdmSetPropertySimple(
    HANDLE handle,
    const GUID* const guidPropertySet,
    unsigned long property,
    void* value,
    unsigned long valueCount,
    void* instance,
    unsigned long instanceCount)
{
    PaError result;
    KSPROPERTY* ksProperty;
    unsigned long propertyCount  = 0;

    propertyCount = sizeof(KSPROPERTY) + instanceCount;
    ksProperty = (KSPROPERTY*)PaUtil_AllocateMemory( propertyCount );
    if( !ksProperty )
    {
        return paInsufficientMemory;
    }

    ksProperty->Set = *guidPropertySet;
    ksProperty->Id = property;
    ksProperty->Flags = KSPROPERTY_TYPE_SET;

    if( instance )
    {
        memcpy((void*)((char*)ksProperty + sizeof(KSPROPERTY)), instance, instanceCount);
    }

    result = WdmSyncIoctl(
                handle,
                IOCTL_KS_PROPERTY,
                ksProperty,
                propertyCount,
                value,
                valueCount,
                NULL);

    PaUtil_FreeMemory( ksProperty );
    return result;
}

static PaError WdmGetPinPropertySimple(
    HANDLE  handle,
    unsigned long pinId,
    const GUID* const guidPropertySet,
    unsigned long property,
    void* value,
    unsigned long valueCount)
{
    PaError result;

    KSP_PIN ksPProp;
    ksPProp.Property.Set = *guidPropertySet;
    ksPProp.Property.Id = property;
    ksPProp.Property.Flags = KSPROPERTY_TYPE_GET;
    ksPProp.PinId = pinId;
    ksPProp.Reserved = 0;

    result = WdmSyncIoctl(
                handle,
                IOCTL_KS_PROPERTY,
                &ksPProp,
                sizeof(KSP_PIN),
                value,
                valueCount,
                NULL);

    return result;
}

static PaError WdmGetPinPropertyMulti(
    HANDLE handle,
    unsigned long pinId,
    const GUID* const guidPropertySet,
    unsigned long property,
    KSMULTIPLE_ITEM** ksMultipleItem)
{
    PaError result;
    unsigned long multipleItemSize = 0;
    KSP_PIN ksPProp;

    ksPProp.Property.Set = *guidPropertySet;
    ksPProp.Property.Id = property;
    ksPProp.Property.Flags = KSPROPERTY_TYPE_GET;
    ksPProp.PinId = pinId;
    ksPProp.Reserved = 0;

    result = WdmSyncIoctl(
                handle,
                IOCTL_KS_PROPERTY,
                &ksPProp.Property,
                sizeof(KSP_PIN),
                NULL,
                0,
                &multipleItemSize);
    if( result != paNoError )
    {
        return result;
    }

    *ksMultipleItem = (KSMULTIPLE_ITEM*)PaUtil_AllocateMemory( multipleItemSize );
    if( !*ksMultipleItem )
    {
        return paInsufficientMemory;
    }

    result = WdmSyncIoctl(
                handle,
                IOCTL_KS_PROPERTY,
                &ksPProp,
                sizeof(KSP_PIN),
                (void*)*ksMultipleItem,
                multipleItemSize,
                NULL);

    if( result != paNoError )
    {
        PaUtil_FreeMemory( ksMultipleItem );
    }

    return result;
}


/*
Create a new pin object belonging to a filter
The pin object holds all the configuration information about the pin
before it is opened, and then the handle of the pin after is opened
*/
static PaWinWdmPin* PinNew(PaWinWdmFilter* parentFilter, unsigned long pinId, PaError* error)
{
    PaWinWdmPin* pin;
    PaError result;
    unsigned long i;
    KSMULTIPLE_ITEM* item = NULL;
    KSIDENTIFIER* identifier;
    KSDATARANGE* dataRange;

    PA_LOGE_;
    PA_DEBUG(("Creating pin %d:\n",pinId));

    /* Allocate the new PIN object */
    pin = (PaWinWdmPin*)PaUtil_AllocateMemory( sizeof(PaWinWdmPin) );
    if( !pin )
    {
        result = paInsufficientMemory;
        goto error;
    }

    /* Zero the pin object */
    /* memset( (void*)pin, 0, sizeof(PaWinWdmPin) ); */

    pin->parentFilter = parentFilter;
    pin->pinId = pinId;

    /* Allocate a connect structure */
    pin->pinConnectSize = sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX);
    pin->pinConnect = (KSPIN_CONNECT*)PaUtil_AllocateMemory( pin->pinConnectSize );
    if( !pin->pinConnect )
    {
        result = paInsufficientMemory;
        goto error;
    }

    /* Configure the connect structure with default values */
    pin->pinConnect->Interface.Set               = KSINTERFACESETID_Standard;
    pin->pinConnect->Interface.Id                = KSINTERFACE_STANDARD_STREAMING;
    pin->pinConnect->Interface.Flags             = 0;
    pin->pinConnect->Medium.Set                  = KSMEDIUMSETID_Standard;
    pin->pinConnect->Medium.Id                   = KSMEDIUM_TYPE_ANYINSTANCE;
    pin->pinConnect->Medium.Flags                = 0;
    pin->pinConnect->PinId                       = pinId;
    pin->pinConnect->PinToHandle                 = NULL;
    pin->pinConnect->Priority.PriorityClass      = KSPRIORITY_NORMAL;
    pin->pinConnect->Priority.PrioritySubClass   = 1;
    pin->ksDataFormatWfx = (KSDATAFORMAT_WAVEFORMATEX*)(pin->pinConnect + 1);
    pin->ksDataFormatWfx->DataFormat.FormatSize  = sizeof(KSDATAFORMAT_WAVEFORMATEX);
    pin->ksDataFormatWfx->DataFormat.Flags       = 0;
    pin->ksDataFormatWfx->DataFormat.Reserved    = 0;
    pin->ksDataFormatWfx->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    pin->ksDataFormatWfx->DataFormat.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
    pin->ksDataFormatWfx->DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

    pin->frameSize = 0; /* Unknown until we instantiate pin */

    /* Get the COMMUNICATION property */
    result = WdmGetPinPropertySimple(
        parentFilter->handle,
        pinId,
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_COMMUNICATION,
        &pin->communication,
        sizeof(KSPIN_COMMUNICATION));
    if( result != paNoError )
        goto error;

    if( /*(pin->communication != KSPIN_COMMUNICATION_SOURCE) &&*/
         (pin->communication != KSPIN_COMMUNICATION_SINK) &&
         (pin->communication != KSPIN_COMMUNICATION_BOTH) )
    {
        PA_DEBUG(("Not source/sink\n"));
        result = paInvalidDevice;
        goto error;
    }

    /* Get dataflow information */
    result = WdmGetPinPropertySimple(
        parentFilter->handle,
        pinId,
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_DATAFLOW,
        &pin->dataFlow,
        sizeof(KSPIN_DATAFLOW));

    if( result != paNoError )
        goto error;

    /* Get the INTERFACE property list */
    result = WdmGetPinPropertyMulti(
        parentFilter->handle,
        pinId,
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_INTERFACES,
        &item);

    if( result != paNoError )
        goto error;

    identifier = (KSIDENTIFIER*)(item+1);

    /* Check that at least one interface is STANDARD_STREAMING */
    result = paUnanticipatedHostError;
    for( i = 0; i < item->Count; i++ )
    {
        if( !memcmp( (void*)&identifier[i].Set, (void*)&KSINTERFACESETID_Standard, sizeof( GUID ) ) &&
            ( identifier[i].Id == KSINTERFACE_STANDARD_STREAMING ) )
        {
            result = paNoError;
            break;
        }
    }

    if( result != paNoError )
    {
        PA_DEBUG(("No standard streaming\n"));
        goto error;
    }

    /* Don't need interfaces any more */
    PaUtil_FreeMemory( item );
    item = NULL;

    /* Get the MEDIUM properties list */
    result = WdmGetPinPropertyMulti(
        parentFilter->handle,
        pinId,
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_MEDIUMS,
        &item);

    if( result != paNoError )
        goto error;

    identifier = (KSIDENTIFIER*)(item+1); /* Not actually necessary... */

    /* Check that at least one medium is STANDARD_DEVIO */
    result = paUnanticipatedHostError;
    for( i = 0; i < item->Count; i++ )
    {
        if( !memcmp( (void*)&identifier[i].Set, (void*)&KSMEDIUMSETID_Standard, sizeof( GUID ) ) &&
           ( identifier[i].Id == KSMEDIUM_STANDARD_DEVIO ) )
        {
            result = paNoError;
            break;
        }
    }

    if( result != paNoError )
    {
        PA_DEBUG(("No standard devio\n"));
        goto error;
    }
    /* Don't need mediums any more */
    PaUtil_FreeMemory( item );
    item = NULL;

    /* Get DATARANGES */
    result = WdmGetPinPropertyMulti(
        parentFilter->handle,
        pinId,
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_DATARANGES,
        &pin->dataRangesItem);

    if( result != paNoError )
        goto error;

    pin->dataRanges = (KSDATARANGE*)(pin->dataRangesItem +1);

    /* Check that at least one datarange supports audio */
    result = paUnanticipatedHostError;
    dataRange = pin->dataRanges;
    pin->maxChannels = 0;
    pin->bestSampleRate = 0;
    pin->formats = 0;
    for( i = 0; i <pin->dataRangesItem->Count; i++)
    {
        PA_DEBUG(("DR major format %x\n",*(unsigned long*)(&(dataRange->MajorFormat))));
        /* Check that subformat is WAVEFORMATEX, PCM or WILDCARD */
        if( IS_VALID_WAVEFORMATEX_GUID(&dataRange->SubFormat) ||
            !memcmp((void*)&dataRange->SubFormat, (void*)&KSDATAFORMAT_SUBTYPE_PCM, sizeof ( GUID ) ) ||
            ( !memcmp((void*)&dataRange->SubFormat, (void*)&KSDATAFORMAT_SUBTYPE_WILDCARD, sizeof ( GUID ) ) &&
            ( !memcmp((void*)&dataRange->MajorFormat, (void*)&KSDATAFORMAT_TYPE_AUDIO, sizeof ( GUID ) ) ) ) )
        {
            result = paNoError;
            /* Record the maximum possible channels with this pin */
            PA_DEBUG(("MaxChannel: %d\n",pin->maxChannels));
            if( (int)((KSDATARANGE_AUDIO*)dataRange)->MaximumChannels > pin->maxChannels )
            {
                pin->maxChannels = ((KSDATARANGE_AUDIO*)dataRange)->MaximumChannels;
                /*PA_DEBUG(("MaxChannel: %d\n",pin->maxChannels));*/
            }
            /* Record the formats (bit depths) that are supported */
            if( ((KSDATARANGE_AUDIO*)dataRange)->MinimumBitsPerSample <= 16 )
            {
                pin->formats |= paInt16;
                PA_DEBUG(("Format 16 bit supported\n"));
            }
            if( ((KSDATARANGE_AUDIO*)dataRange)->MaximumBitsPerSample >= 24 )
            {
                pin->formats |= paInt24;
                PA_DEBUG(("Format 24 bit supported\n"));
            }
            if( ( pin->bestSampleRate != 48000) &&
                (((KSDATARANGE_AUDIO*)dataRange)->MaximumSampleFrequency >= 48000) &&
                (((KSDATARANGE_AUDIO*)dataRange)->MinimumSampleFrequency <= 48000) )
            {
                pin->bestSampleRate = 48000;
                PA_DEBUG(("48kHz supported\n"));
            }
            else if(( pin->bestSampleRate != 48000) && ( pin->bestSampleRate != 44100 ) &&
                (((KSDATARANGE_AUDIO*)dataRange)->MaximumSampleFrequency >= 44100) &&
                (((KSDATARANGE_AUDIO*)dataRange)->MinimumSampleFrequency <= 44100) )
            {
                pin->bestSampleRate = 44100;
                PA_DEBUG(("44.1kHz supported\n"));
            }
            else
            {
                pin->bestSampleRate = ((KSDATARANGE_AUDIO*)dataRange)->MaximumSampleFrequency;
            }
        }
        dataRange = (KSDATARANGE*)( ((char*)dataRange) + dataRange->FormatSize);
    }

    if( result != paNoError )
        goto error;

    /* Get instance information */
    result = WdmGetPinPropertySimple(
        parentFilter->handle,
        pinId,
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_CINSTANCES,
        &pin->instances,
        sizeof(KSPIN_CINSTANCES));

    if( result != paNoError )
        goto error;

    /* Success */
    *error = paNoError;
    PA_DEBUG(("Pin created successfully\n"));
    PA_LOGL_;
    return pin;

error:
    /*
    Error cleanup
    */
    PaUtil_FreeMemory( item );
    if( pin )
    {
        PaUtil_FreeMemory( pin->pinConnect );
        PaUtil_FreeMemory( pin->dataRangesItem );
        PaUtil_FreeMemory( pin );
    }
    *error = result;
    PA_LOGL_;
    return NULL;
}

/*
Safely free all resources associated with the pin
*/
static void PinFree(PaWinWdmPin* pin)
{
    PA_LOGE_;
    if( pin )
    {
        PinClose(pin);
        if( pin->pinConnect )
        {
            PaUtil_FreeMemory( pin->pinConnect );
        }
        if( pin->dataRangesItem )
        {
            PaUtil_FreeMemory( pin->dataRangesItem );
        }
        PaUtil_FreeMemory( pin );
    }
    PA_LOGL_;
}

/*
If the pin handle is open, close it
*/
static void PinClose(PaWinWdmPin* pin)
{
    PA_LOGE_;
    if( pin == NULL )
    {
        PA_DEBUG(("Closing NULL pin!"));
        PA_LOGL_;
        return;
    }
    if( pin->handle != NULL )
    {
        PinSetState( pin, KSSTATE_PAUSE );
        PinSetState( pin, KSSTATE_STOP );
        CloseHandle( pin->handle );
        pin->handle = NULL;
        FilterRelease(pin->parentFilter);
    }
    PA_LOGL_;
}

/*
Set the state of this (instantiated) pin
*/
static PaError PinSetState(PaWinWdmPin* pin, KSSTATE state)
{
    PaError result;

    PA_LOGE_;
    if( pin == NULL )
        return paInternalError;
    if( pin->handle == NULL )
        return paInternalError;

    result = WdmSetPropertySimple(
        pin->handle,
        &KSPROPSETID_Connection,
        KSPROPERTY_CONNECTION_STATE,
        &state,
        sizeof(state),
        NULL,
        0);
    PA_LOGL_;
    return result;
}

static PaError PinInstantiate(PaWinWdmPin* pin)
{
    PaError result;
    unsigned long createResult;
    KSALLOCATOR_FRAMING ksaf;
    KSALLOCATOR_FRAMING_EX ksafex;

    PA_LOGE_;

    if( pin == NULL )
        return paInternalError;
    if(!pin->pinConnect)
        return paInternalError;

    FilterUse(pin->parentFilter);

    createResult = FunctionKsCreatePin(
        pin->parentFilter->handle,
        pin->pinConnect,
        GENERIC_WRITE | GENERIC_READ,
        &pin->handle
        );

    PA_DEBUG(("Pin create result = %x\n",createResult));
    if( createResult != ERROR_SUCCESS )
    {
        FilterRelease(pin->parentFilter);
        pin->handle = NULL;
        return paInvalidDevice;
    }

    result = WdmGetPropertySimple(
        pin->handle,
        &KSPROPSETID_Connection,
        KSPROPERTY_CONNECTION_ALLOCATORFRAMING,
        &ksaf,
        sizeof(ksaf),
        NULL,
        0);

    if( result != paNoError )
    {
        result = WdmGetPropertySimple(
            pin->handle,
            &KSPROPSETID_Connection,
            KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX,
            &ksafex,
            sizeof(ksafex),
            NULL,
            0);
        if( result == paNoError )
        {
            pin->frameSize = ksafex.FramingItem[0].FramingRange.Range.MinFrameSize;
        }
    }
    else
    {
        pin->frameSize = ksaf.FrameSize;
    }

    PA_LOGL_;

    return paNoError;
}

/* NOT USED
static PaError PinGetState(PaWinWdmPin* pin, KSSTATE* state)
{
    PaError result;

    if( state == NULL )
        return paInternalError;
    if( pin == NULL )
        return paInternalError;
    if( pin->handle == NULL )
        return paInternalError;

    result = WdmGetPropertySimple(
        pin->handle,
        KSPROPSETID_Connection,
        KSPROPERTY_CONNECTION_STATE,
        state,
        sizeof(KSSTATE),
        NULL,
        0);

    return result;
}
*/
static PaError PinSetFormat(PaWinWdmPin* pin, const WAVEFORMATEX* format)
{
    unsigned long size;
    void* newConnect;

    PA_LOGE_;

    if( pin == NULL )
        return paInternalError;
    if( format == NULL )
        return paInternalError;

    size = GetWfexSize(format) + sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX) - sizeof(WAVEFORMATEX);

    if( pin->pinConnectSize != size )
    {
        newConnect = PaUtil_AllocateMemory( size );
        if( newConnect == NULL )
            return paInsufficientMemory;
        memcpy( newConnect, (void*)pin->pinConnect, min(pin->pinConnectSize,size) );
        PaUtil_FreeMemory( pin->pinConnect );
        pin->pinConnect = (KSPIN_CONNECT*)newConnect;
        pin->pinConnectSize = size;
        pin->ksDataFormatWfx = (KSDATAFORMAT_WAVEFORMATEX*)((KSPIN_CONNECT*)newConnect + 1);
        pin->ksDataFormatWfx->DataFormat.FormatSize = size - sizeof(KSPIN_CONNECT);
    }

    memcpy( (void*)&(pin->ksDataFormatWfx->WaveFormatEx), format, GetWfexSize(format) );
    pin->ksDataFormatWfx->DataFormat.SampleSize = (unsigned short)(format->nChannels * (format->wBitsPerSample / 8));

    PA_LOGL_;

    return paNoError;
}

static PaError PinIsFormatSupported(PaWinWdmPin* pin, const WAVEFORMATEX* format)
{
    KSDATARANGE_AUDIO* dataRange;
    unsigned long count;
    GUID guid = DYNAMIC_GUID( DEFINE_WAVEFORMATEX_GUID(format->wFormatTag) );
    PaError result = paInvalidDevice;

    PA_LOGE_;

    if( format->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
    {
        guid = ((WAVEFORMATEXTENSIBLE*)format)->SubFormat;
    }
    dataRange = (KSDATARANGE_AUDIO*)pin->dataRanges;
    for(count = 0; count<pin->dataRangesItem->Count; count++)
    {
        if(( !memcmp(&(dataRange->DataRange.MajorFormat),&KSDATAFORMAT_TYPE_AUDIO,sizeof(GUID)) ) ||
           ( !memcmp(&(dataRange->DataRange.MajorFormat),&KSDATAFORMAT_TYPE_WILDCARD,sizeof(GUID)) ))
        {
            /* This is an audio or wildcard datarange... */
            if(( !memcmp(&(dataRange->DataRange.SubFormat),&KSDATAFORMAT_SUBTYPE_WILDCARD,sizeof(GUID)) ) ||
                ( !memcmp(&(dataRange->DataRange.SubFormat),&guid,sizeof(GUID)) ))
            {
                if(( !memcmp(&(dataRange->DataRange.Specifier),&KSDATAFORMAT_SPECIFIER_WILDCARD,sizeof(GUID)) ) ||
                  ( !memcmp(&(dataRange->DataRange.Specifier),&KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,sizeof(GUID) )))
                {

                    PA_DEBUG(("Pin:%x, DataRange:%d\n",(void*)pin,count));
                    PA_DEBUG(("\tFormatSize:%d, SampleSize:%d\n",dataRange->DataRange.FormatSize,dataRange->DataRange.SampleSize));
                    PA_DEBUG(("\tMaxChannels:%d\n",dataRange->MaximumChannels));
                    PA_DEBUG(("\tBits:%d-%d\n",dataRange->MinimumBitsPerSample,dataRange->MaximumBitsPerSample));
                    PA_DEBUG(("\tSampleRate:%d-%d\n",dataRange->MinimumSampleFrequency,dataRange->MaximumSampleFrequency));

                    if( dataRange->MaximumChannels < format->nChannels )
                    {
                        result = paInvalidChannelCount;
                        continue;
                    }
                    if( dataRange->MinimumBitsPerSample > format->wBitsPerSample )
                    {
                        result = paSampleFormatNotSupported;
                        continue;
                    }
                    if( dataRange->MaximumBitsPerSample < format->wBitsPerSample )
                    {
                        result = paSampleFormatNotSupported;
                        continue;
                    }
                    if( dataRange->MinimumSampleFrequency > format->nSamplesPerSec )
                    {
                        result = paInvalidSampleRate;
                        continue;
                    }
                    if( dataRange->MaximumSampleFrequency < format->nSamplesPerSec )
                    {
                        result = paInvalidSampleRate;
                        continue;
                    }
                    /* Success! */
                    PA_LOGL_;
                    return paNoError;
                }
            }
        }
        dataRange = (KSDATARANGE_AUDIO*)( ((char*)dataRange) + dataRange->DataRange.FormatSize);
    }

    PA_LOGL_;

    return result;
}

/**
 * Create a new filter object
 */
static PaWinWdmFilter* FilterNew(TCHAR* filterName, TCHAR* friendlyName, PaError* error)
{
    PaWinWdmFilter* filter;
    PaError result;
    int pinId;
    int valid;


    /* Allocate the new filter object */
    filter = (PaWinWdmFilter*)PaUtil_AllocateMemory( sizeof(PaWinWdmFilter) );
    if( !filter )
    {
        result = paInsufficientMemory;
        goto error;
    }

    /* Zero the filter object - done by AllocateMemory */
    /* memset( (void*)filter, 0, sizeof(PaWinWdmFilter) ); */

    /* Copy the filter name */
    _tcsncpy(filter->filterName, filterName, MAX_PATH);

    /* Copy the friendly name */
    _tcsncpy(filter->friendlyName, friendlyName, MAX_PATH);

    /* Open the filter handle */
    result = FilterUse(filter);
    if( result != paNoError )
    {
        goto error;
    }

    /* Get pin count */
    result = WdmGetPinPropertySimple
        (
        filter->handle,
        0,
        &KSPROPSETID_Pin,
        KSPROPERTY_PIN_CTYPES,
        &filter->pinCount,
        sizeof(filter->pinCount)
        );

    if( result != paNoError)
    {
        goto error;
    }

    /* Allocate pointer array to hold the pins */
    filter->pins = (PaWinWdmPin**)PaUtil_AllocateMemory( sizeof(PaWinWdmPin*) * filter->pinCount );
    if( !filter->pins )
    {
        result = paInsufficientMemory;
        goto error;
    }

    /* Create all the pins we can */
    filter->maxInputChannels = 0;
    filter->maxOutputChannels = 0;
    filter->bestSampleRate = 0;

    valid = 0;
    for(pinId = 0; pinId < filter->pinCount; pinId++)
    {
        /* Create the pin with this Id */
        PaWinWdmPin* newPin;
        newPin = PinNew(filter, pinId, &result);
        if( result == paInsufficientMemory )
            goto error;
        if( newPin != NULL )
        {
            filter->pins[pinId] = newPin;
            valid = 1;

            /* Get the max output channel count */
            if(( newPin->dataFlow == KSPIN_DATAFLOW_IN ) &&
                (( newPin->communication == KSPIN_COMMUNICATION_SINK) ||
                 ( newPin->communication == KSPIN_COMMUNICATION_BOTH)))
            {
                if(newPin->maxChannels > filter->maxOutputChannels)
                    filter->maxOutputChannels = newPin->maxChannels;
                filter->formats |= newPin->formats;
            }
            /* Get the max input channel count */
            if(( newPin->dataFlow == KSPIN_DATAFLOW_OUT ) &&
                (( newPin->communication == KSPIN_COMMUNICATION_SINK) ||
                 ( newPin->communication == KSPIN_COMMUNICATION_BOTH)))
            {
                if(newPin->maxChannels > filter->maxInputChannels)
                    filter->maxInputChannels = newPin->maxChannels;
                filter->formats |= newPin->formats;
            }

            if(newPin->bestSampleRate > filter->bestSampleRate)
            {
                filter->bestSampleRate = newPin->bestSampleRate;
            }
        }
    }

    if(( filter->maxInputChannels == 0) && ( filter->maxOutputChannels == 0))
    {
        /* No input or output... not valid */
        valid = 0;
    }

    if( !valid )
    {
        /* No valid pin was found on this filter so we destroy it */
        result = paDeviceUnavailable;
        goto error;
    }

    /* Close the filter handle for now
     * It will be opened later when needed */
    FilterRelease(filter);

    *error = paNoError;
    return filter;

error:
    /*
    Error cleanup
    */
    if( filter )
    {
        for( pinId = 0; pinId < filter->pinCount; pinId++ )
            PinFree(filter->pins[pinId]);
        PaUtil_FreeMemory( filter->pins );
        if( filter->handle )
            CloseHandle( filter->handle );
        PaUtil_FreeMemory( filter );
    }
    *error = result;
    return NULL;
}

/**
 * Free a previously created filter
 */
static void FilterFree(PaWinWdmFilter* filter)
{
    int pinId;
    PA_LOGL_;
    if( filter )
    {
        for( pinId = 0; pinId < filter->pinCount; pinId++ )
            PinFree(filter->pins[pinId]);
        PaUtil_FreeMemory( filter->pins );
        if( filter->handle )
            CloseHandle( filter->handle );
        PaUtil_FreeMemory( filter );
    }
    PA_LOGE_;
}

/**
 * Reopen the filter handle if necessary so it can be used
 **/
static PaError FilterUse(PaWinWdmFilter* filter)
{
    assert( filter );

    PA_LOGE_;
    if( filter->handle == NULL )
    {
        /* Open the filter */
        filter->handle = CreateFile(
            filter->filterName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);

        if( filter->handle == NULL )
        {
            return paDeviceUnavailable;
        }
    }
    filter->usageCount++;
    PA_LOGL_;
    return paNoError;
}

/**
 * Release the filter handle if nobody is using it
 **/
static void FilterRelease(PaWinWdmFilter* filter)
{
    assert( filter );
    assert( filter->usageCount > 0 );

    PA_LOGE_;
    filter->usageCount--;
    if( filter->usageCount == 0 )
    {
        if( filter->handle != NULL )
        {
            CloseHandle( filter->handle );
            filter->handle = NULL;
        }
    }
    PA_LOGL_;
}

/**
 * Create a render (playback) Pin using the supplied format
 **/
static PaWinWdmPin* FilterCreateRenderPin(PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error)
{
    PaError result;
    PaWinWdmPin* pin;

    assert( filter );

    pin = FilterFindViableRenderPin(filter,wfex,&result);
    if(!pin)
    {
        goto error;
    }
    result = PinSetFormat(pin,wfex);
    if( result != paNoError )
    {
        goto error;
    }
    result = PinInstantiate(pin);
    if( result != paNoError )
    {
        goto error;
    }

    *error = paNoError;
    return pin;

error:
    *error = result;
    return NULL;
}

/**
 * Find a pin that supports the given format
 **/
static PaWinWdmPin* FilterFindViableRenderPin(PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error)
{
    int pinId;
    PaWinWdmPin*  pin;
    PaError result = paDeviceUnavailable;
    *error = paNoError;

    assert( filter );

    for( pinId = 0; pinId<filter->pinCount; pinId++ )
    {
        pin = filter->pins[pinId];
        if( pin != NULL )
        {
            if(( pin->dataFlow == KSPIN_DATAFLOW_IN ) &&
                (( pin->communication == KSPIN_COMMUNICATION_SINK) ||
                 ( pin->communication == KSPIN_COMMUNICATION_BOTH)))
            {
                result = PinIsFormatSupported( pin, wfex );
                if( result == paNoError )
                {
                    return pin;
                }
            }
        }
    }

    *error = result;
    return NULL;
}

/**
 * Check if there is a pin that should playback
 * with the supplied format
 **/
static PaError FilterCanCreateRenderPin(PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex)
{
    PaWinWdmPin* pin;
    PaError result;

    assert ( filter );

    pin = FilterFindViableRenderPin(filter,wfex,&result);
    /* result will be paNoError if pin found
     * or else an error code indicating what is wrong with the format
     **/
    return result;
}

/**
 * Create a capture (record) Pin using the supplied format
 **/
static PaWinWdmPin* FilterCreateCapturePin(PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error)
{
    PaError result;
    PaWinWdmPin* pin;

    assert( filter );

    pin = FilterFindViableCapturePin(filter,wfex,&result);
    if(!pin)
    {
        goto error;
    }

    result = PinSetFormat(pin,wfex);
    if( result != paNoError )
    {
        goto error;
    }

    result = PinInstantiate(pin);
    if( result != paNoError )
    {
        goto error;
    }

    *error = paNoError;
    return pin;

error:
    *error = result;
    return NULL;
}

/**
 * Find a capture pin that supports the given format
 **/
static PaWinWdmPin* FilterFindViableCapturePin(PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex,
    PaError* error)
{
    int pinId;
    PaWinWdmPin*  pin;
    PaError result = paDeviceUnavailable;
    *error = paNoError;

    assert( filter );

    for( pinId = 0; pinId<filter->pinCount; pinId++ )
    {
        pin = filter->pins[pinId];
        if( pin != NULL )
        {
            if(( pin->dataFlow == KSPIN_DATAFLOW_OUT ) &&
                (( pin->communication == KSPIN_COMMUNICATION_SINK) ||
                 ( pin->communication == KSPIN_COMMUNICATION_BOTH)))
            {
                result = PinIsFormatSupported( pin, wfex );
                if( result == paNoError )
                {
                    return pin;
                }
            }
        }
    }

    *error = result;
    return NULL;
}

/**
 * Check if there is a pin that should playback
 * with the supplied format
 **/
static PaError FilterCanCreateCapturePin(PaWinWdmFilter* filter,
    const WAVEFORMATEX* wfex)
{
    PaWinWdmPin* pin;
    PaError result;

    assert ( filter );

    pin = FilterFindViableCapturePin(filter,wfex,&result);
    /* result will be paNoError if pin found
     * or else an error code indicating what is wrong with the format
     **/
    return result;
}

/**
 * Build the list of available filters
 * Use the SetupDi API to enumerate all devices in the KSCATEGORY_AUDIO which 
 * have a KSCATEGORY_RENDER or KSCATEGORY_CAPTURE alias. For each of these 
 * devices initialise a PaWinWdmFilter structure by calling our NewFilter() 
 * function. We enumerate devices twice, once to count how many there are, 
 * and once to initialize the PaWinWdmFilter structures.
 */
static PaError BuildFilterList(PaWinWdmHostApiRepresentation* wdmHostApi)
{
    PaError result = paNoError;
    HDEVINFO handle = NULL;
    int device;
    int invalidDevices;
    int slot;
    SP_DEVICE_INTERFACE_DATA interfaceData;
    SP_DEVICE_INTERFACE_DATA aliasData;
    SP_DEVINFO_DATA devInfoData;
    int noError;
    const int sizeInterface = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + (MAX_PATH * sizeof(WCHAR));
    unsigned char interfaceDetailsArray[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + (MAX_PATH * sizeof(WCHAR))];
    SP_DEVICE_INTERFACE_DETAIL_DATA* devInterfaceDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA*)interfaceDetailsArray;
    TCHAR friendlyName[MAX_PATH];
    HKEY hkey;
    DWORD sizeFriendlyName;
    DWORD type;
    PaWinWdmFilter* newFilter;
    GUID* category = (GUID*)&KSCATEGORY_AUDIO;
    GUID* alias_render = (GUID*)&KSCATEGORY_RENDER;
    GUID* alias_capture = (GUID*)&KSCATEGORY_CAPTURE;
    DWORD hasAlias;

    PA_LOGE_;

    devInterfaceDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    /* Open a handle to search for devices (filters) */
    handle = SetupDiGetClassDevs(category,NULL,NULL,DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if( handle == NULL )
    {
        return paUnanticipatedHostError;
    }
    PA_DEBUG(("Setup called\n"));

    /* First let's count the number of devices so we can allocate a list */
    invalidDevices = 0;
    for( device = 0;;device++ )
    {
        interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        interfaceData.Reserved = 0;
        aliasData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        aliasData.Reserved = 0;
        noError = SetupDiEnumDeviceInterfaces(handle,NULL,category,device,&interfaceData);
        PA_DEBUG(("Enum called\n"));
        if( !noError )
            break; /* No more devices */

        /* Check this one has the render or capture alias */
        hasAlias = 0;
        noError = SetupDiGetDeviceInterfaceAlias(handle,&interfaceData,alias_render,&aliasData);
        PA_DEBUG(("noError = %d\n",noError));
        if(noError)
        {
            if(aliasData.Flags && (!(aliasData.Flags & SPINT_REMOVED)))
            {
                PA_DEBUG(("Device %d has render alias\n",device));
                hasAlias |= 1; /* Has render alias */
            }
            else
            {
                PA_DEBUG(("Device %d has no render alias\n",device));
            }
        }
        noError = SetupDiGetDeviceInterfaceAlias(handle,&interfaceData,alias_capture,&aliasData);
        if(noError)
        {
            if(aliasData.Flags && (!(aliasData.Flags & SPINT_REMOVED)))
            {
                PA_DEBUG(("Device %d has capture alias\n",device));
                hasAlias |= 2; /* Has capture alias */
            }
            else
            {
                PA_DEBUG(("Device %d has no capture alias\n",device));
            }
        }
        if(!hasAlias)
            invalidDevices++; /* This was not a valid capture or render audio device */

    }
    /* Remember how many there are */
    wdmHostApi->filterCount = device-invalidDevices;

    PA_DEBUG(("Interfaces found: %d\n",device-invalidDevices));

    /* Now allocate the list of pointers to devices */
    wdmHostApi->filters  = (PaWinWdmFilter**)PaUtil_AllocateMemory( sizeof(PaWinWdmFilter*) * device );
    if( !wdmHostApi->filters )
    {
        if(handle != NULL)
            SetupDiDestroyDeviceInfoList(handle);
        return paInsufficientMemory;
    }

    /* Now create filter objects for each interface found */
    slot = 0;
    for( device = 0;;device++ )
    {
        interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        interfaceData.Reserved = 0;
        aliasData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        aliasData.Reserved = 0;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        devInfoData.Reserved = 0;

        noError = SetupDiEnumDeviceInterfaces(handle,NULL,category,device,&interfaceData);
        if( !noError )
            break; /* No more devices */

        /* Check this one has the render or capture alias */
        hasAlias = 0;
        noError = SetupDiGetDeviceInterfaceAlias(handle,&interfaceData,alias_render,&aliasData);
        if(noError)
        {
            if(aliasData.Flags && (!(aliasData.Flags & SPINT_REMOVED)))
            {
                PA_DEBUG(("Device %d has render alias\n",device));
                hasAlias |= 1; /* Has render alias */
            }
        }
        noError = SetupDiGetDeviceInterfaceAlias(handle,&interfaceData,alias_capture,&aliasData);
        if(noError)
        {
            if(aliasData.Flags && (!(aliasData.Flags & SPINT_REMOVED)))
            {
                PA_DEBUG(("Device %d has capture alias\n",device));
                hasAlias |= 2; /* Has capture alias */
            }
        }
        if(!hasAlias)
            continue; /* This was not a valid capture or render audio device */

        noError = SetupDiGetDeviceInterfaceDetail(handle,&interfaceData,devInterfaceDetails,sizeInterface,NULL,&devInfoData);
        if( noError )
        {
            /* Try to get the "friendly name" for this interface */
            sizeFriendlyName = sizeof(friendlyName);
            /* Fix contributed by Ben Allison
             * Removed KEY_SET_VALUE from flags on following call
             * as its causes failure when running without admin rights
             * and it was not required */
            hkey=SetupDiOpenDeviceInterfaceRegKey(handle,&interfaceData,0,KEY_QUERY_VALUE);
            if(hkey!=INVALID_HANDLE_VALUE)
            {
                noError = RegQueryValueEx(hkey,TEXT("FriendlyName"),0,&type,(BYTE*)friendlyName,&sizeFriendlyName);
                if( noError == ERROR_SUCCESS )
                {
                    PA_DEBUG(("Interface %d, Name: %s\n",device,friendlyName));
                    RegCloseKey(hkey);
                }
                else
                {
                    friendlyName[0] = 0;
                }
            }
            newFilter = FilterNew(devInterfaceDetails->DevicePath,friendlyName,&result);
            if( result == paNoError )
            {
                PA_DEBUG(("Filter created\n"));
                wdmHostApi->filters[slot] = newFilter;
                slot++;
            }
            else
            {
                PA_DEBUG(("Filter NOT created\n"));
                /* As there are now less filters than we initially thought
                 * we must reduce the count by one */
                wdmHostApi->filterCount--;
            }
        }
    }

    /* Clean up */
    if(handle != NULL)
        SetupDiDestroyDeviceInfoList(handle);

    return paNoError;
}

PaError PaWinWdm_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    int i, deviceCount;
    PaWinWdmHostApiRepresentation *wdmHostApi;
    PaWinWdmDeviceInfo *deviceInfoArray;
    PaWinWdmFilter* pFilter;
    PaWinWdmDeviceInfo *wdmDeviceInfo;
    PaDeviceInfo *deviceInfo;

    PA_LOGE_;

    /*
    Attempt to load the KSUSER.DLL without which we cannot create pins
    We will unload this on termination
    */
    if(DllKsUser == NULL)
    {
        DllKsUser = LoadLibrary(TEXT("ksuser.dll"));
        if(DllKsUser == NULL)
            goto error;
    }

    FunctionKsCreatePin = (KSCREATEPIN*)GetProcAddress(DllKsUser, "KsCreatePin");
    if(FunctionKsCreatePin == NULL)
        goto error;

    wdmHostApi = (PaWinWdmHostApiRepresentation*)PaUtil_AllocateMemory( sizeof(PaWinWdmHostApiRepresentation) );
    if( !wdmHostApi )
    {
        result = paInsufficientMemory;
        goto error;
    }

    wdmHostApi->allocations = PaUtil_CreateAllocationGroup();
    if( !wdmHostApi->allocations )
    {
        result = paInsufficientMemory;
        goto error;
    }

    result = BuildFilterList( wdmHostApi );
    if( result != paNoError )
    {
        goto error;
    }
    deviceCount = wdmHostApi->filterCount;

    *hostApi = &wdmHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;
    (*hostApi)->info.type = paWDMKS;
    (*hostApi)->info.name = "Windows WDM-KS";
    (*hostApi)->info.defaultInputDevice = paNoDevice;
    (*hostApi)->info.defaultOutputDevice = paNoDevice;

    if( deviceCount > 0 )
    {
        (*hostApi)->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory(
               wdmHostApi->allocations, sizeof(PaWinWdmDeviceInfo*) * deviceCount );
        if( !(*hostApi)->deviceInfos )
        {
            result = paInsufficientMemory;
            goto error;
        }

        /* allocate all device info structs in a contiguous block */
        deviceInfoArray = (PaWinWdmDeviceInfo*)PaUtil_GroupAllocateMemory(
                wdmHostApi->allocations, sizeof(PaWinWdmDeviceInfo) * deviceCount );
        if( !deviceInfoArray )
        {
            result = paInsufficientMemory;
            goto error;
        }

        for( i=0; i < deviceCount; ++i )
        {
            wdmDeviceInfo = &deviceInfoArray[i];
            deviceInfo = &wdmDeviceInfo->inheritedDeviceInfo;
            pFilter = wdmHostApi->filters[i];
            if( pFilter == NULL )
                continue;
            wdmDeviceInfo->filter = pFilter;
            deviceInfo->structVersion = 2;
            deviceInfo->hostApi = hostApiIndex;
            deviceInfo->name = (char*)pFilter->friendlyName;
            PA_DEBUG(("Device found name: %s\n",(char*)pFilter->friendlyName));
            deviceInfo->maxInputChannels = pFilter->maxInputChannels;
            if(deviceInfo->maxInputChannels > 0)
            {
                /* Set the default input device to the first device we find with
                 * more than zero input channels
                 **/
                if((*hostApi)->info.defaultInputDevice == paNoDevice)
                {
                    (*hostApi)->info.defaultInputDevice = i;
                }
            }

            deviceInfo->maxOutputChannels = pFilter->maxOutputChannels;
            if(deviceInfo->maxOutputChannels > 0)
            {
                /* Set the default output device to the first device we find with
                 * more than zero output channels
                 **/
                if((*hostApi)->info.defaultOutputDevice == paNoDevice)
                {
                    (*hostApi)->info.defaultOutputDevice = i;
                }
            }

            /* These low values are not very useful because
             * a) The lowest latency we end up with can depend on many factors such
             *    as the device buffer sizes/granularities, sample rate, channels and format
             * b) We cannot know the device buffer sizes until we try to open/use it at
             *    a particular setting
             * So: we give 512x48000Hz frames as the default low input latency
             **/
            deviceInfo->defaultLowInputLatency = (512.0/48000.0);
            deviceInfo->defaultLowOutputLatency = (512.0/48000.0);
            deviceInfo->defaultHighInputLatency = (4096.0/48000.0);
            deviceInfo->defaultHighOutputLatency = (4096.0/48000.0);
            deviceInfo->defaultSampleRate = (double)(pFilter->bestSampleRate);

            (*hostApi)->deviceInfos[i] = deviceInfo;
        }
    }

    (*hostApi)->info.deviceCount = deviceCount;

    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &wdmHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &wdmHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );

    PA_LOGL_;
    return result;

error:
    if( DllKsUser != NULL )
    {
        FreeLibrary( DllKsUser );
        DllKsUser = NULL;
    }

    if( wdmHostApi )
    {
        PaUtil_FreeMemory( wdmHostApi->filters );
        if( wdmHostApi->allocations )
        {
            PaUtil_FreeAllAllocations( wdmHostApi->allocations );
            PaUtil_DestroyAllocationGroup( wdmHostApi->allocations );
        }
        PaUtil_FreeMemory( wdmHostApi );
    }
    PA_LOGL_;
    return result;
}


static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaWinWdmHostApiRepresentation *wdmHostApi = (PaWinWdmHostApiRepresentation*)hostApi;
    int i;
    PA_LOGE_;

    if( wdmHostApi->filters )
    {
        for( i=0; i<wdmHostApi->filterCount; i++)
        {
            if( wdmHostApi->filters[i] != NULL )
            {
                FilterFree( wdmHostApi->filters[i] );
                wdmHostApi->filters[i] = NULL;
            }
        }
    }
    PaUtil_FreeMemory( wdmHostApi->filters );
    if( wdmHostApi->allocations )
    {
        PaUtil_FreeAllAllocations( wdmHostApi->allocations );
        PaUtil_DestroyAllocationGroup( wdmHostApi->allocations );
    }
    PaUtil_FreeMemory( wdmHostApi );
    PA_LOGL_;
}

static void FillWFEXT( WAVEFORMATEXTENSIBLE* pwfext, PaSampleFormat sampleFormat, double sampleRate, int channelCount)
{
    PA_LOGE_;
    PA_DEBUG(( "sampleFormat = %lx\n" , sampleFormat ));
    PA_DEBUG(( "sampleRate = %f\n" , sampleRate ));
    PA_DEBUG(( "chanelCount = %d\n", channelCount ));

    pwfext->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    pwfext->Format.nChannels = channelCount;
    pwfext->Format.nSamplesPerSec = (int)sampleRate;
    if(channelCount == 1)
        pwfext->dwChannelMask = KSAUDIO_SPEAKER_DIRECTOUT;
    else
        pwfext->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    if(sampleFormat == paFloat32)
    {
        pwfext->Format.nBlockAlign = channelCount * 4;
        pwfext->Format.wBitsPerSample = 32;
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 32;
        pwfext->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    }
    else if(sampleFormat == paInt32)
    {
        pwfext->Format.nBlockAlign = channelCount * 4;
        pwfext->Format.wBitsPerSample = 32;
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 32;
        pwfext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
    else if(sampleFormat == paInt24)
    {
        pwfext->Format.nBlockAlign = channelCount * 3;
        pwfext->Format.wBitsPerSample = 24;
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 24;
        pwfext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
    else if(sampleFormat == paInt16)
    {
        pwfext->Format.nBlockAlign = channelCount * 2;
        pwfext->Format.wBitsPerSample = 16;
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 16;
        pwfext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
    pwfext->Format.nAvgBytesPerSec = pwfext->Format.nSamplesPerSec * pwfext->Format.nBlockAlign;

    PA_LOGL_;
}

static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaWinWdmHostApiRepresentation *wdmHostApi = (PaWinWdmHostApiRepresentation*)hostApi;
    PaWinWdmFilter* pFilter;
    int result = paFormatIsSupported;
    WAVEFORMATEXTENSIBLE wfx;

    PA_LOGE_;

    if( inputParameters )
    {
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;

        /* all standard sample formats are supported by the buffer adapter,
            this implementation doesn't support any custom sample formats */
        if( inputSampleFormat & paCustomFormat )
            return paSampleFormatNotSupported;

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

        /* Check that the input format is supported */
        FillWFEXT(&wfx,paInt16,sampleRate,inputChannelCount);

        pFilter = wdmHostApi->filters[inputParameters->device];
        result = FilterCanCreateCapturePin(pFilter,(const WAVEFORMATEX*)&wfx);
        if( result != paNoError )
        {
            /* Try a WAVE_FORMAT_PCM instead */
            wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
            wfx.Format.cbSize = 0;
            wfx.Samples.wValidBitsPerSample = 0;
            wfx.dwChannelMask = 0;
            wfx.SubFormat = GUID_NULL;
            result = FilterCanCreateCapturePin(pFilter,(const WAVEFORMATEX*)&wfx);
            if( result != paNoError )
                 return result;
        }
    }
    else
    {
        inputChannelCount = 0;
    }

    if( outputParameters )
    {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;

        /* all standard sample formats are supported by the buffer adapter,
            this implementation doesn't support any custom sample formats */
        if( outputSampleFormat & paCustomFormat )
            return paSampleFormatNotSupported;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if( outputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that output device can support outputChannelCount */
        if( outputChannelCount > hostApi->deviceInfos[ outputParameters->device ]->maxOutputChannels )
            return paInvalidChannelCount;

        /* validate outputStreamInfo */
        if( outputParameters->hostApiSpecificStreamInfo )
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */

        /* Check that the output format is supported */
        FillWFEXT(&wfx,paInt16,sampleRate,outputChannelCount);

        pFilter = wdmHostApi->filters[outputParameters->device];
        result = FilterCanCreateRenderPin(pFilter,(const WAVEFORMATEX*)&wfx);
        if( result != paNoError )
        {
            /* Try a WAVE_FORMAT_PCM instead */
            wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
            wfx.Format.cbSize = 0;
            wfx.Samples.wValidBitsPerSample = 0;
            wfx.dwChannelMask = 0;
            wfx.SubFormat = GUID_NULL;
            result = FilterCanCreateRenderPin(pFilter,(const WAVEFORMATEX*)&wfx);
            if( result != paNoError )
                 return result;
        }

    }
    else
    {
        outputChannelCount = 0;
    }

    /*
        IMPLEMENT ME:

            - if a full duplex stream is requested, check that the combination
                of input and output parameters is supported if necessary

            - check that the device supports sampleRate

        Because the buffer adapter handles conversion between all standard
        sample formats, the following checks are only required if paCustomFormat
        is implemented, or under some other unusual conditions.

            - check that input device can support inputSampleFormat, or that
                we have the capability to convert from inputSampleFormat to
                a native format

            - check that output device can support outputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format
    */
    if((inputChannelCount == 0)&&(outputChannelCount == 0))
            result = paSampleFormatNotSupported; /* Not right error */

    PA_LOGL_;
    return result;
}

/* see pa_hostapi.h for a list of validity guarantees made about OpenStream parameters */

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
    PaWinWdmHostApiRepresentation *wdmHostApi = (PaWinWdmHostApiRepresentation*)hostApi;
    PaWinWdmStream *stream = 0;
    /* unsigned long framesPerHostBuffer; these may not be equivalent for all implementations */
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaSampleFormat hostInputSampleFormat, hostOutputSampleFormat;
    int userInputChannels,userOutputChannels;
    int size;
    PaWinWdmFilter* pFilter;
    WAVEFORMATEXTENSIBLE wfx;

    PA_LOGE_;
    PA_DEBUG(("OpenStream:sampleRate = %f\n",sampleRate));
    PA_DEBUG(("OpenStream:framesPerBuffer = %lu\n",framesPerBuffer));

    if( inputParameters )
    {
        userInputChannels = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if( inputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that input device can support stream->userInputChannels */
        if( userInputChannels > hostApi->deviceInfos[ inputParameters->device ]->maxInputChannels )
            return paInvalidChannelCount;

        /* validate inputStreamInfo */
        if( inputParameters->hostApiSpecificStreamInfo )
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */

    }
    else
    {
        userInputChannels = 0;
        inputSampleFormat = hostInputSampleFormat = paInt16; /* Surpress 'uninitialised var' warnings. */
    }

    if( outputParameters )
    {
        userOutputChannels = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if( outputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that output device can support stream->userInputChannels */
        if( userOutputChannels > hostApi->deviceInfos[ outputParameters->device ]->maxOutputChannels )
            return paInvalidChannelCount;

        /* validate outputStreamInfo */
        if( outputParameters->hostApiSpecificStreamInfo )
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */

    }
    else
    {
        userOutputChannels = 0;
        outputSampleFormat = hostOutputSampleFormat = paInt16; /* Surpress 'uninitialized var' warnings. */
    }

    /* validate platform specific flags */
    if( (streamFlags & paPlatformSpecificFlags) != 0 )
        return paInvalidFlag; /* unexpected platform specific flag */

    stream = (PaWinWdmStream*)PaUtil_AllocateMemory( sizeof(PaWinWdmStream) );
    if( !stream )
    {
        result = paInsufficientMemory;
        goto error;
    }
    /* Zero the stream object */
    /* memset((void*)stream,0,sizeof(PaWinWdmStream)); */

    if( streamCallback )
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &wdmHostApi->callbackStreamInterface, streamCallback, userData );
    }
    else
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &wdmHostApi->blockingStreamInterface, streamCallback, userData );
    }

    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, sampleRate );

    /* Instantiate the input pin if necessary */
    if(userInputChannels > 0)
    {
        result = paSampleFormatNotSupported;
        pFilter = wdmHostApi->filters[inputParameters->device];
        stream->userInputChannels = userInputChannels;

        if(((inputSampleFormat & ~paNonInterleaved) & pFilter->formats) != 0)
        {   /* inputSampleFormat is supported, so try to use it */
            hostInputSampleFormat = inputSampleFormat;
            FillWFEXT(&wfx, hostInputSampleFormat, sampleRate, stream->userInputChannels);
            stream->bytesPerInputFrame = wfx.Format.nBlockAlign;
            stream->recordingPin = FilterCreateCapturePin(pFilter, (const WAVEFORMATEX*)&wfx, &result);
            stream->deviceInputChannels = stream->userInputChannels;
        }
        
        if(result != paNoError)
        {   /* Search through all PaSampleFormats to find one that works */
            hostInputSampleFormat = paFloat32;

            do {
                FillWFEXT(&wfx, hostInputSampleFormat, sampleRate, stream->userInputChannels);
                stream->bytesPerInputFrame = wfx.Format.nBlockAlign;
                stream->recordingPin = FilterCreateCapturePin(pFilter, (const WAVEFORMATEX*)&wfx, &result);
                stream->deviceInputChannels = stream->userInputChannels;
                
                if(stream->recordingPin == NULL) result = paSampleFormatNotSupported;
                if(result != paNoError)    hostInputSampleFormat <<= 1;
            }
            while(result != paNoError && hostInputSampleFormat <= paUInt8);
        }

        if(result != paNoError)
        {    /* None of the PaSampleFormats worked.  Set the hostInputSampleFormat to the best fit
             * and try a PCM format.
             **/
            hostInputSampleFormat =
                PaUtil_SelectClosestAvailableFormat( pFilter->formats, inputSampleFormat );

            /* Try a WAVE_FORMAT_PCM instead */
            wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
            wfx.Format.cbSize = 0;
            wfx.Samples.wValidBitsPerSample = 0;
            wfx.dwChannelMask = 0;
            wfx.SubFormat = GUID_NULL;
            stream->recordingPin = FilterCreateCapturePin(pFilter,(const WAVEFORMATEX*)&wfx,&result);
            if(stream->recordingPin == NULL) result = paSampleFormatNotSupported;
        }

        if( result != paNoError )
        {
            /* Some or all KS devices can only handle the exact number of channels
             * they specify. But PortAudio clients expect to be able to
             * at least specify mono I/O on a multi-channel device
             * If this is the case, then we will do the channel mapping internally
             **/
            if( stream->userInputChannels < pFilter->maxInputChannels )
            {
                FillWFEXT(&wfx,hostInputSampleFormat,sampleRate,pFilter->maxInputChannels);
                stream->bytesPerInputFrame = wfx.Format.nBlockAlign;
                stream->recordingPin = FilterCreateCapturePin(pFilter,(const WAVEFORMATEX*)&wfx,&result);
                stream->deviceInputChannels = pFilter->maxInputChannels;

                if( result != paNoError )
                {
                    /* Try a WAVE_FORMAT_PCM instead */
                    wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
                    wfx.Format.cbSize = 0;
                    wfx.Samples.wValidBitsPerSample = 0;
                    wfx.dwChannelMask = 0;
                    wfx.SubFormat = GUID_NULL;
                    stream->recordingPin = FilterCreateCapturePin(pFilter,(const WAVEFORMATEX*)&wfx,&result);
                }
            }
        }

        if(stream->recordingPin == NULL)
        {
            goto error;
        }

        switch(hostInputSampleFormat)
        {
            case paInt16: stream->inputSampleSize = 2; break;
            case paInt24: stream->inputSampleSize = 3; break;
            case paInt32:
            case paFloat32:    stream->inputSampleSize = 4; break;
        }

        stream->recordingPin->frameSize /= stream->bytesPerInputFrame;
        PA_DEBUG(("Pin output frames: %d\n",stream->recordingPin->frameSize));
    }
    else
    {
        stream->recordingPin = NULL;
        stream->bytesPerInputFrame = 0;
    }

    /* Instantiate the output pin if necessary */
    if(userOutputChannels > 0)
    {
        result = paSampleFormatNotSupported;
        pFilter = wdmHostApi->filters[outputParameters->device];
        stream->userOutputChannels = userOutputChannels;

        if(((outputSampleFormat & ~paNonInterleaved) & pFilter->formats) != 0)
        {
            hostOutputSampleFormat = outputSampleFormat;
            FillWFEXT(&wfx,hostOutputSampleFormat,sampleRate,stream->userOutputChannels);
            stream->bytesPerOutputFrame = wfx.Format.nBlockAlign;
            stream->playbackPin = FilterCreateRenderPin(pFilter,(WAVEFORMATEX*)&wfx,&result);
            stream->deviceOutputChannels = stream->userOutputChannels;
        }

        if(result != paNoError)
        {
            hostOutputSampleFormat = paFloat32;

            do {
                FillWFEXT(&wfx,hostOutputSampleFormat,sampleRate,stream->userOutputChannels);
                stream->bytesPerOutputFrame = wfx.Format.nBlockAlign;
                stream->playbackPin = FilterCreateRenderPin(pFilter,(WAVEFORMATEX*)&wfx,&result);
                stream->deviceOutputChannels = stream->userOutputChannels;

                if(stream->playbackPin == NULL) result = paSampleFormatNotSupported;
                if(result != paNoError)    hostOutputSampleFormat <<= 1;
            }
            while(result != paNoError && hostOutputSampleFormat <= paUInt8);
        }

        if(result != paNoError)
        {
            hostOutputSampleFormat =
                PaUtil_SelectClosestAvailableFormat( pFilter->formats, outputSampleFormat );
       
            /* Try a WAVE_FORMAT_PCM instead */
            wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
            wfx.Format.cbSize = 0;
            wfx.Samples.wValidBitsPerSample = 0;
            wfx.dwChannelMask = 0;
            wfx.SubFormat = GUID_NULL;
            stream->playbackPin = FilterCreateRenderPin(pFilter,(WAVEFORMATEX*)&wfx,&result);
            if(stream->playbackPin == NULL) result = paSampleFormatNotSupported;
        }
            
        if( result != paNoError )
        {
            /* Some or all KS devices can only handle the exact number of channels
             * they specify. But PortAudio clients expect to be able to
             * at least specify mono I/O on a multi-channel device
             * If this is the case, then we will do the channel mapping internally
             **/
            if( stream->userOutputChannels < pFilter->maxOutputChannels )
            {
                FillWFEXT(&wfx,hostOutputSampleFormat,sampleRate,pFilter->maxOutputChannels);
                stream->bytesPerOutputFrame = wfx.Format.nBlockAlign;
                stream->playbackPin = FilterCreateRenderPin(pFilter,(const WAVEFORMATEX*)&wfx,&result);
                stream->deviceOutputChannels = pFilter->maxOutputChannels;
                if( result != paNoError )
                {
                    /* Try a WAVE_FORMAT_PCM instead */
                    wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
                    wfx.Format.cbSize = 0;
                    wfx.Samples.wValidBitsPerSample = 0;
                    wfx.dwChannelMask = 0;
                    wfx.SubFormat = GUID_NULL;
                    stream->playbackPin = FilterCreateRenderPin(pFilter,(const WAVEFORMATEX*)&wfx,&result);
                }
            }
        }

        if(stream->playbackPin == NULL)
        {
            goto error;
        }

        switch(hostOutputSampleFormat)
        {
            case paInt16: stream->outputSampleSize = 2; break;
            case paInt24: stream->outputSampleSize = 3; break;
            case paInt32:
            case paFloat32: stream->outputSampleSize = 4; break;
        }

        stream->playbackPin->frameSize /= stream->bytesPerOutputFrame;
        PA_DEBUG(("Pin output frames: %d\n",stream->playbackPin->frameSize));
    }
    else
    {
        stream->playbackPin = NULL;
        stream->bytesPerOutputFrame = 0;
    }

    /* Calculate the framesPerHostXxxxBuffer size based upon the suggested latency values */

    /* Record the buffer length */
    if(inputParameters)
    {
        /* Calculate the frames from the user's value - add a bit to round up */
            stream->framesPerHostIBuffer = (unsigned long)((inputParameters->suggestedLatency*sampleRate)+0.0001);
        if(stream->framesPerHostIBuffer > (unsigned long)sampleRate)
        { /* Upper limit is 1 second */
              stream->framesPerHostIBuffer = (unsigned long)sampleRate;
        }
        else if(stream->framesPerHostIBuffer < stream->recordingPin->frameSize)
        {
              stream->framesPerHostIBuffer = stream->recordingPin->frameSize;
        }
        PA_DEBUG(("Input frames chosen:%ld\n",stream->framesPerHostIBuffer));
    }

    if(outputParameters)
    {
        /* Calculate the frames from the user's value - add a bit to round up */
        stream->framesPerHostOBuffer = (unsigned long)((outputParameters->suggestedLatency*sampleRate)+0.0001);
        if(stream->framesPerHostOBuffer > (unsigned long)sampleRate)
        { /* Upper limit is 1 second */
                  stream->framesPerHostOBuffer = (unsigned long)sampleRate;
        }
        else if(stream->framesPerHostOBuffer < stream->playbackPin->frameSize)
        {
              stream->framesPerHostOBuffer = stream->playbackPin->frameSize;
        }
        PA_DEBUG(("Output frames chosen:%ld\n",stream->framesPerHostOBuffer));
    }

    /* Host buffer size is bounded to the largest of the input and output
    frame sizes */

    result =  PaUtil_InitializeBufferProcessor( &stream->bufferProcessor,
              stream->userInputChannels, inputSampleFormat, hostInputSampleFormat,
              stream->userOutputChannels, outputSampleFormat, hostOutputSampleFormat,
              sampleRate, streamFlags, framesPerBuffer,
              max(stream->framesPerHostOBuffer,stream->framesPerHostIBuffer),
              paUtilBoundedHostBufferSize,
              streamCallback, userData );
    if( result != paNoError )
        goto error;

    stream->streamRepresentation.streamInfo.inputLatency =
            ((double)stream->framesPerHostIBuffer) / sampleRate;
    stream->streamRepresentation.streamInfo.outputLatency =
            ((double)stream->framesPerHostOBuffer) / sampleRate;
    stream->streamRepresentation.streamInfo.sampleRate = sampleRate;

      PA_DEBUG(("BytesPerInputFrame = %d\n",stream->bytesPerInputFrame));
      PA_DEBUG(("BytesPerOutputFrame = %d\n",stream->bytesPerOutputFrame));

    /* Allocate all the buffers for host I/O */
    size = 2 * (stream->framesPerHostIBuffer*stream->bytesPerInputFrame +  stream->framesPerHostOBuffer*stream->bytesPerOutputFrame);
    PA_DEBUG(("Buffer size = %d\n",size));
    stream->hostBuffer = (char*)PaUtil_AllocateMemory(size);
    PA_DEBUG(("Buffer allocated\n"));
    if( !stream->hostBuffer )
    {
        PA_DEBUG(("Cannot allocate host buffer!\n"));
        result = paInsufficientMemory;
        goto error;
    }
    PA_DEBUG(("Buffer start = %p\n",stream->hostBuffer));
    /* memset(stream->hostBuffer,0,size); */

    /* Set up the packets */
    stream->events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    ResetEvent(stream->events[0]); /* Record buffer 1 */
    stream->events[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
    ResetEvent(stream->events[1]); /* Record buffer 2 */
    stream->events[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
    ResetEvent(stream->events[2]); /* Play buffer 1 */
    stream->events[3] = CreateEvent(NULL, FALSE, FALSE, NULL);
    ResetEvent(stream->events[3]); /* Play buffer 2 */
    stream->events[4] = CreateEvent(NULL, FALSE, FALSE, NULL);
    ResetEvent(stream->events[4]); /* Abort event */
    if(stream->userInputChannels > 0)
    {
        DATAPACKET *p = &(stream->packets[0]);
        p->Signal.hEvent = stream->events[0];
        p->Header.Data = stream->hostBuffer;
        p->Header.FrameExtent = stream->framesPerHostIBuffer*stream->bytesPerInputFrame;
        p->Header.DataUsed = 0;
        p->Header.Size = sizeof(p->Header);
        p->Header.PresentationTime.Numerator = 1;
        p->Header.PresentationTime.Denominator = 1;

        p = &(stream->packets[1]);
        p->Signal.hEvent = stream->events[1];
        p->Header.Data = stream->hostBuffer + stream->framesPerHostIBuffer*stream->bytesPerInputFrame;
        p->Header.FrameExtent = stream->framesPerHostIBuffer*stream->bytesPerInputFrame;
        p->Header.DataUsed = 0;
        p->Header.Size = sizeof(p->Header);
        p->Header.PresentationTime.Numerator = 1;
        p->Header.PresentationTime.Denominator = 1;
    }
    if(stream->userOutputChannels > 0)
    {
        DATAPACKET *p = &(stream->packets[2]);
        p->Signal.hEvent = stream->events[2];
        p->Header.Data = stream->hostBuffer + 2*stream->framesPerHostIBuffer*stream->bytesPerInputFrame;
        p->Header.FrameExtent = stream->framesPerHostOBuffer*stream->bytesPerOutputFrame;
        p->Header.DataUsed = stream->framesPerHostOBuffer*stream->bytesPerOutputFrame;
        p->Header.Size = sizeof(p->Header);
        p->Header.PresentationTime.Numerator = 1;
        p->Header.PresentationTime.Denominator = 1;
    
        p = &(stream->packets[3]);
        p->Signal.hEvent = stream->events[3];
        p->Header.Data = stream->hostBuffer + 2*stream->framesPerHostIBuffer*stream->bytesPerInputFrame + stream->framesPerHostOBuffer*stream->bytesPerOutputFrame;
        p->Header.FrameExtent = stream->framesPerHostOBuffer*stream->bytesPerOutputFrame;
        p->Header.DataUsed = stream->framesPerHostOBuffer*stream->bytesPerOutputFrame;
        p->Header.Size = sizeof(p->Header);
        p->Header.PresentationTime.Numerator = 1;
        p->Header.PresentationTime.Denominator = 1;
    }

    stream->streamStarted = 0;
    stream->streamActive = 0;
    stream->streamStop = 0;
    stream->streamAbort = 0;
    stream->streamFlags = streamFlags;
    stream->oldProcessPriority = REALTIME_PRIORITY_CLASS;

    *s = (PaStream*)stream;

    PA_LOGL_;
    return result;

error:
    size = 5;
    while(size--)
    {
        if(stream->events[size] != NULL)
        {
            CloseHandle(stream->events[size]);
            stream->events[size] = NULL;
        }
    }
    if(stream->hostBuffer)
        PaUtil_FreeMemory( stream->hostBuffer );

    if(stream->playbackPin)
        PinClose(stream->playbackPin);
    if(stream->recordingPin)
        PinClose(stream->recordingPin);

    if( stream )
        PaUtil_FreeMemory( stream );

    PA_LOGL_;
    return result;
}

/*
    When CloseStream() is called, the multi-api layer ensures that
    the stream has already been stopped or aborted.
*/
static PaError CloseStream( PaStream* s )
{
    PaError result = paNoError;
    PaWinWdmStream *stream = (PaWinWdmStream*)s;
    int size;

    PA_LOGE_;

    assert(!stream->streamStarted);
    assert(!stream->streamActive);

    PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );
    PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );
    size = 5;
    while(size--)
    {
        if(stream->events[size] != NULL)
        {
            CloseHandle(stream->events[size]);
            stream->events[size] = NULL;
        }
    }
    if(stream->hostBuffer)
        PaUtil_FreeMemory( stream->hostBuffer );

    if(stream->playbackPin)
        PinClose(stream->playbackPin);
    if(stream->recordingPin)
        PinClose(stream->recordingPin);

    PaUtil_FreeMemory( stream );

    PA_LOGL_;
    return result;
}

/*
Write the supplied packet to the pin
Asynchronous
Should return false on success
*/
static BOOL PinWrite(HANDLE h, DATAPACKET* p)
{
    unsigned long cbReturned = 0;
    return DeviceIoControl(h,IOCTL_KS_WRITE_STREAM,NULL,0,
                            &p->Header,p->Header.Size,&cbReturned,&p->Signal);
}

/*
Read to the supplied packet from the pin
Asynchronous
Should return false on success
*/
static BOOL PinRead(HANDLE h, DATAPACKET* p)
{
    unsigned long cbReturned = 0;
    return DeviceIoControl(h,IOCTL_KS_READ_STREAM,NULL,0,
                            &p->Header,p->Header.Size,&cbReturned,&p->Signal);
}

/*
Copy the first interleaved channel of 16 bit data to the other channels
*/
static void DuplicateFirstChannelInt16(void* buffer, int channels, int samples)
{
    unsigned short* data = (unsigned short*)buffer;
    int channel;
    unsigned short sourceSample;
    while( samples-- )
    {
        sourceSample = *data++;
        channel = channels-1;
        while( channel-- )
        {
            *data++ = sourceSample;
        }
    }
}

/*
Copy the first interleaved channel of 24 bit data to the other channels
*/
static void DuplicateFirstChannelInt24(void* buffer, int channels, int samples)
{
    unsigned char* data = (unsigned char*)buffer;
    int channel;
    unsigned char sourceSample[3];
    while( samples-- )
    {
        sourceSample[0] = data[0];
        sourceSample[1] = data[1];
        sourceSample[2] = data[2];
        data += 3;
        channel = channels-1;
        while( channel-- )
        {
            data[0] = sourceSample[0];
            data[1] = sourceSample[1];
            data[2] = sourceSample[2];
            data += 3;
        }
    }
}

/*
Copy the first interleaved channel of 32 bit data to the other channels
*/
static void DuplicateFirstChannelInt32(void* buffer, int channels, int samples)
{
    unsigned long* data = (unsigned long*)buffer;
    int channel;
    unsigned long sourceSample;
    while( samples-- )
    {
        sourceSample = *data++;
        channel = channels-1;
        while( channel-- )
        {
            *data++ = sourceSample;
        }
    }
}

static DWORD WINAPI ProcessingThread(LPVOID pParam)
{
    PaWinWdmStream *stream = (PaWinWdmStream*)pParam;
    PaStreamCallbackTimeInfo ti;
    int cbResult = paContinue;
    int inbuf = 0;
    int outbuf = 0;
    int pending = 0;
    PaError result;
    unsigned long wait;
    unsigned long eventSignaled;
    int fillPlaybuf = 0;
    int emptyRecordbuf = 0;
    int framesProcessed;
    unsigned long timeout;
    int i;
    int doChannelCopy;
    int priming = 0;
    PaStreamCallbackFlags underover = 0;

    PA_LOGE_;

    ti.inputBufferAdcTime = 0.0;
    ti.currentTime = 0.0;
    ti.outputBufferDacTime = 0.0;

    /* Get double buffering going */

    /* Submit buffers */
    if(stream->playbackPin)
    {
        result = PinSetState(stream->playbackPin, KSSTATE_RUN);

        PA_DEBUG(("play state run = %d;",(int)result));
        SetEvent(stream->events[outbuf+2]);
        outbuf = (outbuf+1)&1;
        SetEvent(stream->events[outbuf+2]);
        outbuf = (outbuf+1)&1;
        pending += 2;
        priming += 4;
    }
    if(stream->recordingPin)
    {
        result = PinSetState(stream->recordingPin, KSSTATE_RUN);

        PA_DEBUG(("recording state run = %d;",(int)result));
        PinRead(stream->recordingPin->handle,&stream->packets[inbuf]);
        inbuf = (inbuf+1)&1; /* Increment and wrap */
        PinRead(stream->recordingPin->handle,&stream->packets[inbuf]);
        inbuf = (inbuf+1)&1; /* Increment and wrap */
        /* FIXME - do error checking */
        pending += 2;
    }
    PA_DEBUG(("Out buffer len:%f\n",(2000*stream->framesPerHostOBuffer) / stream->streamRepresentation.streamInfo.sampleRate));
    PA_DEBUG(("In buffer len:%f\n",(2000*stream->framesPerHostIBuffer) / stream->streamRepresentation.streamInfo.sampleRate));
    timeout = max(
       ((2000*(DWORD)stream->framesPerHostOBuffer) / (DWORD)stream->streamRepresentation.streamInfo.sampleRate),
       ((2000*(DWORD)stream->framesPerHostIBuffer) / (DWORD)stream->streamRepresentation.streamInfo.sampleRate));
    timeout = max(timeout,1);
    PA_DEBUG(("Timeout = %ld\n",timeout));

    while(!stream->streamAbort)
    {
        fillPlaybuf = 0;
        emptyRecordbuf = 0;

        /* Wait for next input or output buffer to be finished with*/
        assert(pending>0);

        if(stream->streamStop)
        {
            PA_DEBUG(("ss1:pending=%d ",pending));
        }
        wait = WaitForMultipleObjects(5, stream->events, FALSE, 0);
        if( wait == WAIT_TIMEOUT )
        {
            /* No (under|over)flow has ocurred */
            wait = WaitForMultipleObjects(5, stream->events, FALSE, timeout);
            eventSignaled = wait - WAIT_OBJECT_0;
        }
        else
        {
            eventSignaled = wait - WAIT_OBJECT_0;
            if( eventSignaled < 2 )
            {
                underover |= paInputOverflow;
                PA_DEBUG(("Input overflow\n"));
            }
            else if(( eventSignaled < 4 )&&(!priming))
            {
                underover |= paOutputUnderflow;
                PA_DEBUG(("Output underflow\n"));
            }
        }

        if(stream->streamStop)
        {
            PA_DEBUG(("ss2:wait=%ld",wait));
        }
        if(wait == WAIT_FAILED)
        {
            PA_DEBUG(("Wait fail = %ld! ",wait));
            break;
        }
        if(wait == WAIT_TIMEOUT)
        {
            continue;
        }

        if(eventSignaled < 2)
        { /* Recording input buffer has been filled */
            if(stream->playbackPin)
            {
                /* First check if also the next playback buffer has been signaled */
                wait = WaitForSingleObject(stream->events[outbuf+2],0);
                if(wait == WAIT_OBJECT_0)
                {
                    /* Yes, so do both buffers at same time */
                    fillPlaybuf = 1;
                    pending--;
                    /* Was this an underflow situation? */
                    if( underover )
                        underover |= paOutputUnderflow; /* Yes! */
                }
            }
            emptyRecordbuf = 1;
            pending--;
        }
        else if(eventSignaled < 4)
        { /* Playback output buffer has been emptied */
            if(stream->recordingPin)
            {
                /* First check if also the next recording buffer has been signaled */
                wait = WaitForSingleObject(stream->events[inbuf],0);
                if(wait == WAIT_OBJECT_0)
                { /* Yes, so do both buffers at same time */
                    emptyRecordbuf = 1;
                    pending--;
                    /* Was this an overflow situation? */
                    if( underover )
                        underover |= paInputOverflow; /* Yes! */
                }
            }
            fillPlaybuf = 1;
            pending--;
        }
        else
        {
            /* Abort event! */
            assert(stream->streamAbort); /* Should have been set */
            PA_DEBUG(("ABORTING "));
            break;
        }
        ResetEvent(stream->events[eventSignaled]);

        if(stream->streamStop)
        {
            PA_DEBUG(("Stream stop! pending=%d",pending));
            cbResult = paComplete; /* Stop, but play remaining buffers */
        }

        /* Do necessary buffer processing (which will invoke user callback if necessary */
        doChannelCopy = 0;
        if(cbResult==paContinue)
        {
            PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );
            if((stream->bufferProcessor.hostInputFrameCount[0] + stream->bufferProcessor.hostInputFrameCount[1]) ==
                (stream->bufferProcessor.hostOutputFrameCount[0] + stream->bufferProcessor.hostOutputFrameCount[1]) )
                PaUtil_BeginBufferProcessing(&stream->bufferProcessor,&ti,underover);
            underover = 0; /* Reset the (under|over)flow status */
            if(fillPlaybuf)
            {
                PaUtil_SetOutputFrameCount(&stream->bufferProcessor,0);
                if( stream->userOutputChannels == 1 )
                {
                    /* Write the single user channel to the first interleaved block */
                    PaUtil_SetOutputChannel(&stream->bufferProcessor,0,stream->packets[outbuf+2].Header.Data,stream->deviceOutputChannels);
                    /* We will do a copy to the other channels after the data has been written */
                    doChannelCopy = 1;
                }
                else
                {
                    for(i=0;i<stream->userOutputChannels;i++)
                    {
                        /* Only write the user output channels. Leave the rest blank */
                        PaUtil_SetOutputChannel(&stream->bufferProcessor,i,((unsigned char*)(stream->packets[outbuf+2].Header.Data))+(i*stream->outputSampleSize),stream->deviceOutputChannels);
                    }
                }
            }
            if(emptyRecordbuf)
            {
                PaUtil_SetInputFrameCount(&stream->bufferProcessor,stream->packets[inbuf].Header.DataUsed/stream->bytesPerInputFrame);
                for(i=0;i<stream->userInputChannels;i++)
                {
                    /* Only read as many channels as the user wants */
                    PaUtil_SetInputChannel(&stream->bufferProcessor,i,((unsigned char*)(stream->packets[inbuf].Header.Data))+(i*stream->inputSampleSize),stream->deviceInputChannels);
                }
            }
            /* Only call the EndBufferProcessing function is the total input frames == total output frames */
            if((stream->bufferProcessor.hostInputFrameCount[0] + stream->bufferProcessor.hostInputFrameCount[1]) ==
                (stream->bufferProcessor.hostOutputFrameCount[0] + stream->bufferProcessor.hostOutputFrameCount[1]) )
                framesProcessed = PaUtil_EndBufferProcessing(&stream->bufferProcessor,&cbResult);
            else framesProcessed = 0;
            if( doChannelCopy )
            {
                /* Copy the first output channel to the other channels */
                switch(stream->outputSampleSize)
                {
                    case 2:
                        DuplicateFirstChannelInt16(stream->packets[outbuf+2].Header.Data,stream->deviceOutputChannels,stream->framesPerHostOBuffer);
                        break;
                    case 3:
                        DuplicateFirstChannelInt24(stream->packets[outbuf+2].Header.Data,stream->deviceOutputChannels,stream->framesPerHostOBuffer);
                        break;
                    case 4:
                        DuplicateFirstChannelInt32(stream->packets[outbuf+2].Header.Data,stream->deviceOutputChannels,stream->framesPerHostOBuffer);
                        break;
                    default:
                        assert(0); /* Unsupported format! */
                        break;
                }
            }
            PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, framesProcessed );
        }
        else
        {
            fillPlaybuf = 0;
            emptyRecordbuf = 0;
        }
            
        /*
        if(cbResult != paContinue)
        {
            PA_DEBUG(("cbResult=%d, pending=%d:",cbResult,pending));
        }
        */
        /* Submit buffers */
        if((fillPlaybuf)&&(cbResult!=paAbort))
        {
            if(!PinWrite(stream->playbackPin->handle,&stream->packets[outbuf+2]))
                outbuf = (outbuf+1)&1; /* Increment and wrap */
            pending++;
            if( priming )
                priming--; /* Have to prime twice */
        }
        if((emptyRecordbuf)&&(cbResult==paContinue))
        {
            stream->packets[inbuf].Header.DataUsed = 0; /* Reset for reuse */
            PinRead(stream->recordingPin->handle,&stream->packets[inbuf]);
            inbuf = (inbuf+1)&1; /* Increment and wrap */
            pending++;
        }
        if(pending==0)
        {
            PA_DEBUG(("pending==0 finished...;"));
            break;
        }
        if((!stream->playbackPin)&&(cbResult!=paContinue))
        {
            PA_DEBUG(("record only cbResult=%d...;",cbResult));
            break;
        }
    }

    PA_DEBUG(("Finished thread"));

    /* Finished, either normally or aborted */
    if(stream->playbackPin)
    {
        result = PinSetState(stream->playbackPin, KSSTATE_PAUSE);
        result = PinSetState(stream->playbackPin, KSSTATE_STOP);
    }
    if(stream->recordingPin)
    {
        result = PinSetState(stream->recordingPin, KSSTATE_PAUSE);
        result = PinSetState(stream->recordingPin, KSSTATE_STOP);
    }

    stream->streamActive = 0;

    if((!stream->streamStop)&&(!stream->streamAbort))
    {
          /* Invoke the user stream finished callback */
          /* Only do it from here if not being stopped/aborted by user */
          if( stream->streamRepresentation.streamFinishedCallback != 0 )
              stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );
    }
    stream->streamStop = 0;
    stream->streamAbort = 0;

    /* Reset process priority if necessary */
    if(stream->oldProcessPriority != REALTIME_PRIORITY_CLASS)
    {
        SetPriorityClass(GetCurrentProcess(),stream->oldProcessPriority);
        stream->oldProcessPriority = REALTIME_PRIORITY_CLASS;
    }

    PA_LOGL_;
    ExitThread(0);
    return 0;
}

static PaError StartStream( PaStream *s )
{
    PaError result = paNoError;
    PaWinWdmStream *stream = (PaWinWdmStream*)s;
    DWORD dwID;
    BOOL ret;
    int size;

    PA_LOGE_;

    stream->streamStop = 0;
    stream->streamAbort = 0;
    size = 5;
    while(size--)
    {
        if(stream->events[size] != NULL)
        {
            ResetEvent(stream->events[size]);
        }
    }

    PaUtil_ResetBufferProcessor( &stream->bufferProcessor );

    stream->oldProcessPriority = GetPriorityClass(GetCurrentProcess());
    /* Uncomment the following line to enable dynamic boosting of the process
     * priority to real time for best low latency support
     * Disabled by default because RT processes can easily block the OS */
    /*ret = SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
      PA_DEBUG(("Class ret = %d;",ret));*/

    stream->streamStarted = 1;
    stream->streamThread = CreateThread(NULL, 0, ProcessingThread, stream, 0, &dwID);
    if(stream->streamThread == NULL)
    {
        stream->streamStarted = 0;
        result = paInsufficientMemory;
        goto end;
    }
    ret = SetThreadPriority(stream->streamThread,THREAD_PRIORITY_TIME_CRITICAL);
    PA_DEBUG(("Priority ret = %d;",ret));
    /* Make the stream active */
    stream->streamActive = 1;

end:
    PA_LOGL_;
    return result;
}


static PaError StopStream( PaStream *s )
{
    PaError result = paNoError;
    PaWinWdmStream *stream = (PaWinWdmStream*)s;
    int doCb = 0;

    PA_LOGE_;

    if(stream->streamActive)
    {
        doCb = 1;
        stream->streamStop = 1;
        while(stream->streamActive)
        {
            PA_DEBUG(("W."));
            Sleep(10); /* Let thread sleep for 10 msec */
        }
    }

    PA_DEBUG(("Terminating thread"));
    if(stream->streamStarted && stream->streamThread)
    {
        TerminateThread(stream->streamThread,0);
        stream->streamThread = NULL;
    }

    stream->streamStarted = 0;

    if(stream->oldProcessPriority != REALTIME_PRIORITY_CLASS)
    {
        SetPriorityClass(GetCurrentProcess(),stream->oldProcessPriority);
        stream->oldProcessPriority = REALTIME_PRIORITY_CLASS;
    }

    if(doCb)
    {
        /* Do user callback now after all state has been reset */
        /* This means it should be safe for the called function */
        /* to invoke e.g. StartStream */
        if( stream->streamRepresentation.streamFinishedCallback != 0 )
             stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );
    }

    PA_LOGL_;
    return result;
}

static PaError AbortStream( PaStream *s )
{
    PaError result = paNoError;
    PaWinWdmStream *stream = (PaWinWdmStream*)s;
    int doCb = 0;

    PA_LOGE_;

    if(stream->streamActive)
    {
        doCb = 1;
        stream->streamAbort = 1;
        SetEvent(stream->events[4]); /* Signal immediately */
        while(stream->streamActive)
        {
            Sleep(10);
        }
    }

    if(stream->streamStarted && stream->streamThread)
    {
        TerminateThread(stream->streamThread,0);
        stream->streamThread = NULL;
    }

    stream->streamStarted = 0;

    if(stream->oldProcessPriority != REALTIME_PRIORITY_CLASS)
    {
        SetPriorityClass(GetCurrentProcess(),stream->oldProcessPriority);
        stream->oldProcessPriority = REALTIME_PRIORITY_CLASS;
    }

    if(doCb)
    {
        /* Do user callback now after all state has been reset */
        /* This means it should be safe for the called function */
        /* to invoke e.g. StartStream */
        if( stream->streamRepresentation.streamFinishedCallback != 0 )
            stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );
    }

    stream->streamActive = 0;
    stream->streamStarted = 0;

    PA_LOGL_;
    return result;
}


static PaError IsStreamStopped( PaStream *s )
{
    PaWinWdmStream *stream = (PaWinWdmStream*)s;
    int result = 0;

    PA_LOGE_;

    if(!stream->streamStarted)
        result = 1;

    PA_LOGL_;
    return result;
}


static PaError IsStreamActive( PaStream *s )
{
    PaWinWdmStream *stream = (PaWinWdmStream*)s;
    int result = 0;

    PA_LOGE_;

    if(stream->streamActive)
        result = 1;

    PA_LOGL_;
    return result;
}


static PaTime GetStreamTime( PaStream* s )
{
    PA_LOGE_;
    PA_LOGL_;
    (void)s;
    return PaUtil_GetTime();
}


static double GetStreamCpuLoad( PaStream* s )
{
    PaWinWdmStream *stream = (PaWinWdmStream*)s;
    double result;
    PA_LOGE_;
    result = PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer );
    PA_LOGL_;
    return result;
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
    PaWinWdmStream *stream = (PaWinWdmStream*)s;

    PA_LOGE_;

    /* suppress unused variable warnings */
    (void) buffer;
    (void) frames;
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    PA_LOGL_;
    return paNoError;
}


static PaError WriteStream( PaStream* s,
                            const void *buffer,
                            unsigned long frames )
{
    PaWinWdmStream *stream = (PaWinWdmStream*)s;

    PA_LOGE_;

    /* suppress unused variable warnings */
    (void) buffer;
    (void) frames;
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    PA_LOGL_;
    return paNoError;
}


static signed long GetStreamReadAvailable( PaStream* s )
{
    PaWinWdmStream *stream = (PaWinWdmStream*)s;

    PA_LOGE_;

    /* suppress unused variable warnings */
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    PA_LOGL_;
    return 0;
}


static signed long GetStreamWriteAvailable( PaStream* s )
{
    PaWinWdmStream *stream = (PaWinWdmStream*)s;

    PA_LOGE_;
    /* suppress unused variable warnings */
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    PA_LOGL_;
    return 0;
}