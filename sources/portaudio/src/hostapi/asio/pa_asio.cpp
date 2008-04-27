/*
 * $Id: pa_asio.cpp 1230 2007-06-15 16:16:33Z rossb $
 * Portable Audio I/O Library for ASIO Drivers
 *
 * Author: Stephane Letz
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 2000-2002 Stephane Letz, Phil Burk, Ross Bencina
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

/* Modification History

        08-03-01 First version : Stephane Letz
        08-06-01 Tweaks for PC, use C++, buffer allocation, Float32 to Int32 conversion : Phil Burk
        08-20-01 More conversion, PA_StreamTime, Pa_GetHostError : Stephane Letz
        08-21-01 PaUInt8 bug correction, implementation of ASIOSTFloat32LSB and ASIOSTFloat32MSB native formats : Stephane Letz
        08-24-01 MAX_INT32_FP hack, another Uint8 fix : Stephane and Phil
        08-27-01 Implementation of hostBufferSize < userBufferSize case, better management of the ouput buffer when
                 the stream is stopped : Stephane Letz
        08-28-01 Check the stream pointer for null in bufferSwitchTimeInfo, correct bug in bufferSwitchTimeInfo when
                 the stream is stopped : Stephane Letz
        10-12-01 Correct the PaHost_CalcNumHostBuffers function: computes FramesPerHostBuffer to be the lowest that
                 respect requested FramesPerUserBuffer and userBuffersPerHostBuffer : Stephane Letz
        10-26-01 Management of hostBufferSize and userBufferSize of any size : Stephane Letz
        10-27-01 Improve calculus of hostBufferSize to be multiple or divisor of userBufferSize if possible : Stephane and Phil
        10-29-01 Change MAX_INT32_FP to (2147483520.0f) to prevent roundup to 0x80000000 : Phil Burk
        10-31-01 Clear the ouput buffer and user buffers in PaHost_StartOutput, correct bug in GetFirstMultiple : Stephane Letz
        11-06-01 Rename functions : Stephane Letz
        11-08-01 New Pa_ASIO_Adaptor_Init function to init Callback adpatation variables, cleanup of Pa_ASIO_Callback_Input: Stephane Letz
        11-29-01 Break apart device loading to debug random failure in Pa_ASIO_QueryDeviceInfo ; Phil Burk
        01-03-02 Desallocate all resources in PaHost_Term for cases where Pa_CloseStream is not called properly :  Stephane Letz
        02-01-02 Cleanup, test of multiple-stream opening : Stephane Letz
        19-02-02 New Pa_ASIO_loadDriver that calls CoInitialize on each thread on Windows : Stephane Letz
        09-04-02 Correct error code management in PaHost_Term, removes various compiler warning : Stephane Letz
        12-04-02 Add Mac includes for <Devices.h> and <Timer.h> : Phil Burk
        13-04-02 Removes another compiler warning : Stephane Letz
        30-04-02 Pa_ASIO_QueryDeviceInfo bug correction, memory allocation checking, better error handling : D Viens, P Burk, S Letz
        12-06-02 Rehashed into new multi-api infrastructure, added support for all ASIO sample formats : Ross Bencina
        18-06-02 Added pa_asio.h, PaAsio_GetAvailableLatencyValues() : Ross B.
        21-06-02 Added SelectHostBufferSize() which selects host buffer size based on user latency parameters : Ross Bencina
        ** NOTE  maintanance history is now stored in CVS **
*/

/** @file
	@ingroup hostapi_src

    Note that specific support for paInputUnderflow, paOutputOverflow and
    paNeverDropInput is not necessary or possible with this driver due to the
    synchronous full duplex double-buffered architecture of ASIO.

    @todo check that CoInitialize()/CoUninitialize() are always correctly
        paired, even in error cases.

    @todo implement host api specific extension to set i/o buffer sizes in frames

    @todo implement ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable

    @todo implement IsFormatSupported

    @todo work out how to implement stream stoppage from callback and
            implement IsStreamActive properly. Stream stoppage could be implemented
            using a high-priority thread blocked on an Event which is signalled
            by the callback. Or, we could just call ASIO stop from the callback
            and see what happens.

    @todo rigorously check asio return codes and convert to pa error codes

    @todo Different channels of a multichannel stream can have different sample
            formats, but we assume that all are the same as the first channel for now.
            Fixing this will require the block processor to maintain per-channel
            conversion functions - could get nasty.

    @todo investigate whether the asio processNow flag needs to be honoured

    @todo handle asioMessages() callbacks in a useful way, or at least document
            what cases we don't handle.

    @todo miscellaneous other FIXMEs

    @todo provide an asio-specific method for setting the systems specific
        value (aka main window handle) - check that this matches the value
        passed to PaAsio_ShowControlPanel, or remove it entirely from
        PaAsio_ShowControlPanel. - this would allow PaAsio_ShowControlPanel
        to be called for the currently open stream (at present all streams
        must be closed).
*/



#include <stdio.h>
#include <assert.h>
#include <string.h>
//#include <values.h>

#include <windows.h>
#include <mmsystem.h>

#include "portaudio.h"
#include "pa_asio.h"
#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_debugprint.h"

/* This version of pa_asio.cpp is currently only targetted at Win32,
   It would require a few tweaks to work with pre-OS X Macintosh.
   To make configuration easier, we define WIN32 here to make sure
   that the ASIO SDK knows this is Win32.
*/
#ifndef WIN32
#define WIN32
#endif

#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"
#include "iasiothiscallresolver.h"

/*
#if MAC
#include <Devices.h>
#include <Timer.h>
#include <Math64.h>
#else
*/
/*
#include <math.h>
#include <windows.h>
#include <mmsystem.h>
*/
/*
#endif
*/

/* external references */
extern AsioDrivers* asioDrivers ;
bool loadAsioDriver(char *name);


/* We are trying to be compatible with CARBON but this has not been thoroughly tested. */
/* not tested at all since new code was introduced. */
#define CARBON_COMPATIBLE  (0)




/* prototypes for functions declared in this file */

extern "C" PaError PaAsio_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex );
static void Terminate( struct PaUtilHostApiRepresentation *hostApi );
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** s,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData );
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate );
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

/* our ASIO callback functions */

static void bufferSwitch(long index, ASIOBool processNow);
static ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow);
static void sampleRateChanged(ASIOSampleRate sRate);
static long asioMessages(long selector, long value, void* message, double* opt);

static ASIOCallbacks asioCallbacks_ =
    { bufferSwitch, sampleRateChanged, asioMessages, bufferSwitchTimeInfo };


#define PA_ASIO_SET_LAST_HOST_ERROR( errorCode, errorText ) \
    PaUtil_SetLastHostErrorInfo( paASIO, errorCode, errorText )


static void PaAsio_SetLastSystemError( DWORD errorCode )
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
    );
    PaUtil_SetLastHostErrorInfo( paASIO, errorCode, (const char*)lpMsgBuf );
    LocalFree( lpMsgBuf );
}

#define PA_ASIO_SET_LAST_SYSTEM_ERROR( errorCode ) \
    PaAsio_SetLastSystemError( errorCode )


static const char* PaAsio_GetAsioErrorText( ASIOError asioError )
{
    const char *result;

    switch( asioError ){
        case ASE_OK:
        case ASE_SUCCESS:           result = "Success"; break;
        case ASE_NotPresent:        result = "Hardware input or output is not present or available"; break;
        case ASE_HWMalfunction:     result = "Hardware is malfunctioning"; break;
        case ASE_InvalidParameter:  result = "Input parameter invalid"; break;
        case ASE_InvalidMode:       result = "Hardware is in a bad mode or used in a bad mode"; break;
        case ASE_SPNotAdvancing:    result = "Hardware is not running when sample position is inquired"; break;
        case ASE_NoClock:           result = "Sample clock or rate cannot be determined or is not present"; break;
        case ASE_NoMemory:          result = "Not enough memory for completing the request"; break;
        default:                    result = "Unknown ASIO error"; break;
    }

    return result;
}


#define PA_ASIO_SET_LAST_ASIO_ERROR( asioError ) \
    PaUtil_SetLastHostErrorInfo( paASIO, asioError, PaAsio_GetAsioErrorText( asioError ) )




// Atomic increment and decrement operations
#if MAC
	/* need to be implemented on Mac */
	inline long PaAsio_AtomicIncrement(volatile long* v) {return ++(*const_cast<long*>(v));}
	inline long PaAsio_AtomicDecrement(volatile long* v) {return --(*const_cast<long*>(v));}
#elif WINDOWS
	inline long PaAsio_AtomicIncrement(volatile long* v) {return InterlockedIncrement(const_cast<long*>(v));}
	inline long PaAsio_AtomicDecrement(volatile long* v) {return InterlockedDecrement(const_cast<long*>(v));}
#endif



typedef struct PaAsioDriverInfo
{
    ASIODriverInfo asioDriverInfo;
    long inputChannelCount, outputChannelCount;
    long bufferMinSize, bufferMaxSize, bufferPreferredSize, bufferGranularity;
    bool postOutput;
}
PaAsioDriverInfo;


/* PaAsioHostApiRepresentation - host api datastructure specific to this implementation */

typedef struct
{
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;

    PaUtilAllocationGroup *allocations;

    void *systemSpecific;
    
    /* the ASIO C API only allows one ASIO driver to be open at a time,
        so we keep track of whether we have the driver open here, and
        use this information to return errors from OpenStream if the
        driver is already open.

        openAsioDeviceIndex will be PaNoDevice if there is no device open
        and a valid pa_asio (not global) device index otherwise.

        openAsioDriverInfo is populated with the driver info for the
        currently open device (if any)
    */
    PaDeviceIndex openAsioDeviceIndex;
    PaAsioDriverInfo openAsioDriverInfo;
}
PaAsioHostApiRepresentation;


/*
    Retrieve <driverCount> driver names from ASIO, returned in a char**
    allocated in <group>.
*/
static char **GetAsioDriverNames( PaUtilAllocationGroup *group, long driverCount )
{
    char **result = 0;
    int i;

    result =(char**)PaUtil_GroupAllocateMemory(
            group, sizeof(char*) * driverCount );
    if( !result )
        goto error;

    result[0] = (char*)PaUtil_GroupAllocateMemory(
            group, 32 * driverCount );
    if( !result[0] )
        goto error;

    for( i=0; i<driverCount; ++i )
        result[i] = result[0] + (32 * i);

    asioDrivers->getDriverNames( result, driverCount );

error:
    return result;
}


static PaSampleFormat AsioSampleTypeToPaNativeSampleFormat(ASIOSampleType type)
{
    switch (type) {
        case ASIOSTInt16MSB:
        case ASIOSTInt16LSB:
                return paInt16;

        case ASIOSTFloat32MSB:
        case ASIOSTFloat32LSB:
        case ASIOSTFloat64MSB:
        case ASIOSTFloat64LSB:
                return paFloat32;

        case ASIOSTInt32MSB:
        case ASIOSTInt32LSB:
        case ASIOSTInt32MSB16:
        case ASIOSTInt32LSB16:
        case ASIOSTInt32MSB18:
        case ASIOSTInt32MSB20:
        case ASIOSTInt32MSB24:
        case ASIOSTInt32LSB18:
        case ASIOSTInt32LSB20:
        case ASIOSTInt32LSB24:
                return paInt32;

        case ASIOSTInt24MSB:
        case ASIOSTInt24LSB:
                return paInt24;

        default:
                return paCustomFormat;
    }
}

void AsioSampleTypeLOG(ASIOSampleType type)
{
    switch (type) {
        case ASIOSTInt16MSB:  PA_DEBUG(("ASIOSTInt16MSB\n"));  break;
        case ASIOSTInt16LSB:  PA_DEBUG(("ASIOSTInt16LSB\n"));  break;
        case ASIOSTFloat32MSB:PA_DEBUG(("ASIOSTFloat32MSB\n"));break;
        case ASIOSTFloat32LSB:PA_DEBUG(("ASIOSTFloat32LSB\n"));break;
        case ASIOSTFloat64MSB:PA_DEBUG(("ASIOSTFloat64MSB\n"));break;
        case ASIOSTFloat64LSB:PA_DEBUG(("ASIOSTFloat64LSB\n"));break;
        case ASIOSTInt32MSB:  PA_DEBUG(("ASIOSTInt32MSB\n"));  break;
        case ASIOSTInt32LSB:  PA_DEBUG(("ASIOSTInt32LSB\n"));  break;
        case ASIOSTInt32MSB16:PA_DEBUG(("ASIOSTInt32MSB16\n"));break;
        case ASIOSTInt32LSB16:PA_DEBUG(("ASIOSTInt32LSB16\n"));break;
        case ASIOSTInt32MSB18:PA_DEBUG(("ASIOSTInt32MSB18\n"));break;
        case ASIOSTInt32MSB20:PA_DEBUG(("ASIOSTInt32MSB20\n"));break;
        case ASIOSTInt32MSB24:PA_DEBUG(("ASIOSTInt32MSB24\n"));break;
        case ASIOSTInt32LSB18:PA_DEBUG(("ASIOSTInt32LSB18\n"));break;
        case ASIOSTInt32LSB20:PA_DEBUG(("ASIOSTInt32LSB20\n"));break;
        case ASIOSTInt32LSB24:PA_DEBUG(("ASIOSTInt32LSB24\n"));break;
        case ASIOSTInt24MSB:  PA_DEBUG(("ASIOSTInt24MSB\n"));  break;
        case ASIOSTInt24LSB:  PA_DEBUG(("ASIOSTInt24LSB\n"));  break;
        default:              PA_DEBUG(("Custom Format%d\n",type));break;

    }
}

static int BytesPerAsioSample( ASIOSampleType sampleType )
{
    switch (sampleType) {
        case ASIOSTInt16MSB:
        case ASIOSTInt16LSB:
            return 2;

        case ASIOSTFloat64MSB:
        case ASIOSTFloat64LSB:
            return 8;

        case ASIOSTFloat32MSB:
        case ASIOSTFloat32LSB:
        case ASIOSTInt32MSB:
        case ASIOSTInt32LSB:
        case ASIOSTInt32MSB16:
        case ASIOSTInt32LSB16:
        case ASIOSTInt32MSB18:
        case ASIOSTInt32MSB20:
        case ASIOSTInt32MSB24:
        case ASIOSTInt32LSB18:
        case ASIOSTInt32LSB20:
        case ASIOSTInt32LSB24:
            return 4;

        case ASIOSTInt24MSB:
        case ASIOSTInt24LSB:
            return 3;

        default:
            return 0;
    }
}


static void Swap16( void *buffer, long shift, long count )
{
    unsigned short *p = (unsigned short*)buffer;
    unsigned short temp;
    (void) shift; /* unused parameter */

    while( count-- )
    {
        temp = *p;
        *p++ = (unsigned short)((temp<<8) | (temp>>8));
    }
}

static void Swap24( void *buffer, long shift, long count )
{
    unsigned char *p = (unsigned char*)buffer;
    unsigned char temp;
    (void) shift; /* unused parameter */

    while( count-- )
    {
        temp = *p;
        *p = *(p+2);
        *(p+2) = temp;
        p += 3;
    }
}

#define PA_SWAP32_( x ) ((x>>24) | ((x>>8)&0xFF00) | ((x<<8)&0xFF0000) | (x<<24));

static void Swap32( void *buffer, long shift, long count )
{
    unsigned long *p = (unsigned long*)buffer;
    unsigned long temp;
    (void) shift; /* unused parameter */

    while( count-- )
    {
        temp = *p;
        *p++ = PA_SWAP32_( temp);
    }
}

static void SwapShiftLeft32( void *buffer, long shift, long count )
{
    unsigned long *p = (unsigned long*)buffer;
    unsigned long temp;

    while( count-- )
    {
        temp = *p;
        temp = PA_SWAP32_( temp);
        *p++ = temp << shift;
    }
}

static void ShiftRightSwap32( void *buffer, long shift, long count )
{
    unsigned long *p = (unsigned long*)buffer;
    unsigned long temp;

    while( count-- )
    {
        temp = *p >> shift;
        *p++ = PA_SWAP32_( temp);
    }
}

static void ShiftLeft32( void *buffer, long shift, long count )
{
    unsigned long *p = (unsigned long*)buffer;
    unsigned long temp;

    while( count-- )
    {
        temp = *p;
        *p++ = temp << shift;
    }
}

static void ShiftRight32( void *buffer, long shift, long count )
{
    unsigned long *p = (unsigned long*)buffer;
    unsigned long temp;

    while( count-- )
    {
        temp = *p;
        *p++ = temp >> shift;
    }
}

#define PA_SWAP_( x, y ) temp=x; x = y; y = temp;

static void Swap64ConvertFloat64ToFloat32( void *buffer, long shift, long count )
{
    double *in = (double*)buffer;
    float *out = (float*)buffer;
    unsigned char *p;
    unsigned char temp;
    (void) shift; /* unused parameter */

    while( count-- )
    {
        p = (unsigned char*)in;
        PA_SWAP_( p[0], p[7] );
        PA_SWAP_( p[1], p[6] );
        PA_SWAP_( p[2], p[5] );
        PA_SWAP_( p[3], p[4] );

        *out++ = (float) (*in++);
    }
}

static void ConvertFloat64ToFloat32( void *buffer, long shift, long count )
{
    double *in = (double*)buffer;
    float *out = (float*)buffer;
    (void) shift; /* unused parameter */

    while( count-- )
        *out++ = (float) (*in++);
}

static void ConvertFloat32ToFloat64Swap64( void *buffer, long shift, long count )
{
    float *in = ((float*)buffer) + (count-1);
    double *out = ((double*)buffer) + (count-1);
    unsigned char *p;
    unsigned char temp;
    (void) shift; /* unused parameter */

    while( count-- )
    {
        *out = *in--;

        p = (unsigned char*)out;
        PA_SWAP_( p[0], p[7] );
        PA_SWAP_( p[1], p[6] );
        PA_SWAP_( p[2], p[5] );
        PA_SWAP_( p[3], p[4] );

        out--;
    }
}

static void ConvertFloat32ToFloat64( void *buffer, long shift, long count )
{
    float *in = ((float*)buffer) + (count-1);
    double *out = ((double*)buffer) + (count-1);
    (void) shift; /* unused parameter */

    while( count-- )
        *out-- = *in--;
}

#ifdef MAC
#define PA_MSB_IS_NATIVE_
#undef PA_LSB_IS_NATIVE_
#endif

#ifdef WINDOWS
#undef PA_MSB_IS_NATIVE_
#define PA_LSB_IS_NATIVE_
#endif

typedef void PaAsioBufferConverter( void *, long, long );

static void SelectAsioToPaConverter( ASIOSampleType type, PaAsioBufferConverter **converter, long *shift )
{
    *shift = 0;
    *converter = 0;

    switch (type) {
        case ASIOSTInt16MSB:
            /* dest: paInt16, no conversion necessary, possible byte swap*/
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap16;
            #endif
            break;
        case ASIOSTInt16LSB:
            /* dest: paInt16, no conversion necessary, possible byte swap*/
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap16;
            #endif
            break;
        case ASIOSTFloat32MSB:
            /* dest: paFloat32, no conversion necessary, possible byte swap*/
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTFloat32LSB:
            /* dest: paFloat32, no conversion necessary, possible byte swap*/
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTFloat64MSB:
            /* dest: paFloat32, in-place conversion to/from float32, possible byte swap*/
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap64ConvertFloat64ToFloat32;
            #else
                *converter = ConvertFloat64ToFloat32;
            #endif
            break;
        case ASIOSTFloat64LSB:
            /* dest: paFloat32, in-place conversion to/from float32, possible byte swap*/
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap64ConvertFloat64ToFloat32;
            #else
                *converter = ConvertFloat64ToFloat32;
            #endif
            break;
        case ASIOSTInt32MSB:
            /* dest: paInt32, no conversion necessary, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTInt32LSB:
            /* dest: paInt32, no conversion necessary, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTInt32MSB16:
            /* dest: paInt32, 16 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 16;
            break;
        case ASIOSTInt32MSB18:
            /* dest: paInt32, 14 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 14;
            break;
        case ASIOSTInt32MSB20:
            /* dest: paInt32, 12 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 12;
            break;
        case ASIOSTInt32MSB24:
            /* dest: paInt32, 8 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 8;
            break;
        case ASIOSTInt32LSB16:
            /* dest: paInt32, 16 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 16;
            break;
        case ASIOSTInt32LSB18:
            /* dest: paInt32, 14 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 14;
            break;
        case ASIOSTInt32LSB20:
            /* dest: paInt32, 12 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 12;
            break;
        case ASIOSTInt32LSB24:
            /* dest: paInt32, 8 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = SwapShiftLeft32;
            #else
                *converter = ShiftLeft32;
            #endif
            *shift = 8;
            break;
        case ASIOSTInt24MSB:
            /* dest: paInt24, no conversion necessary, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap24;
            #endif
            break;
        case ASIOSTInt24LSB:
            /* dest: paInt24, no conversion necessary, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap24;
            #endif
            break;
    }
}


static void SelectPaToAsioConverter( ASIOSampleType type, PaAsioBufferConverter **converter, long *shift )
{
    *shift = 0;
    *converter = 0;

    switch (type) {
        case ASIOSTInt16MSB:
            /* src: paInt16, no conversion necessary, possible byte swap*/
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap16;
            #endif
            break;
        case ASIOSTInt16LSB:
            /* src: paInt16, no conversion necessary, possible byte swap*/
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap16;
            #endif
            break;
        case ASIOSTFloat32MSB:
            /* src: paFloat32, no conversion necessary, possible byte swap*/
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTFloat32LSB:
            /* src: paFloat32, no conversion necessary, possible byte swap*/
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTFloat64MSB:
            /* src: paFloat32, in-place conversion to/from float32, possible byte swap*/
            #ifdef PA_LSB_IS_NATIVE_
                *converter = ConvertFloat32ToFloat64Swap64;
            #else
                *converter = ConvertFloat32ToFloat64;
            #endif
            break;
        case ASIOSTFloat64LSB:
            /* src: paFloat32, in-place conversion to/from float32, possible byte swap*/
            #ifdef PA_MSB_IS_NATIVE_
                *converter = ConvertFloat32ToFloat64Swap64;
            #else
                *converter = ConvertFloat32ToFloat64;
            #endif
            break;
        case ASIOSTInt32MSB:
            /* src: paInt32, no conversion necessary, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTInt32LSB:
            /* src: paInt32, no conversion necessary, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap32;
            #endif
            break;
        case ASIOSTInt32MSB16:
            /* src: paInt32, 16 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 16;
            break;
        case ASIOSTInt32MSB18:
            /* src: paInt32, 14 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 14;
            break;
        case ASIOSTInt32MSB20:
            /* src: paInt32, 12 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 12;
            break;
        case ASIOSTInt32MSB24:
            /* src: paInt32, 8 bit shift, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 8;
            break;
        case ASIOSTInt32LSB16:
            /* src: paInt32, 16 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 16;
            break;
        case ASIOSTInt32LSB18:
            /* src: paInt32, 14 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 14;
            break;
        case ASIOSTInt32LSB20:
            /* src: paInt32, 12 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 12;
            break;
        case ASIOSTInt32LSB24:
            /* src: paInt32, 8 bit shift, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = ShiftRightSwap32;
            #else
                *converter = ShiftRight32;
            #endif
            *shift = 8;
            break;
        case ASIOSTInt24MSB:
            /* src: paInt24, no conversion necessary, possible byte swap */
            #ifdef PA_LSB_IS_NATIVE_
                *converter = Swap24;
            #endif
            break;
        case ASIOSTInt24LSB:
            /* src: paInt24, no conversion necessary, possible byte swap */
            #ifdef PA_MSB_IS_NATIVE_
                *converter = Swap24;
            #endif
            break;
    }
}


typedef struct PaAsioDeviceInfo
{
    PaDeviceInfo commonDeviceInfo;
    long minBufferSize;
    long maxBufferSize;
    long preferredBufferSize;
    long bufferGranularity;

    ASIOChannelInfo *asioChannelInfos;
}
PaAsioDeviceInfo;


PaError PaAsio_GetAvailableLatencyValues( PaDeviceIndex device,
		long *minLatency, long *maxLatency, long *preferredLatency, long *granularity )
{
    PaError result;
    PaUtilHostApiRepresentation *hostApi;
    PaDeviceIndex hostApiDevice;

    result = PaUtil_GetHostApiRepresentation( &hostApi, paASIO );

    if( result == paNoError )
    {
        result = PaUtil_DeviceIndexToHostApiDeviceIndex( &hostApiDevice, device, hostApi );

        if( result == paNoError )
        {
            PaAsioDeviceInfo *asioDeviceInfo =
                    (PaAsioDeviceInfo*)hostApi->deviceInfos[hostApiDevice];

            *minLatency = asioDeviceInfo->minBufferSize;
            *maxLatency = asioDeviceInfo->maxBufferSize;
            *preferredLatency = asioDeviceInfo->preferredBufferSize;
            *granularity = asioDeviceInfo->bufferGranularity;
        }
    }

    return result;
}



/*
    load the asio driver named by <driverName> and return statistics about
    the driver in info. If no error occurred, the driver will remain open
    and must be closed by the called by calling ASIOExit() - if an error
    is returned the driver will already be closed.
*/
static PaError LoadAsioDriver( const char *driverName,
        PaAsioDriverInfo *driverInfo, void *systemSpecific )
{
    PaError result = paNoError;
    ASIOError asioError;
    int asioIsInitialized = 0;

    if( !loadAsioDriver( const_cast<char*>(driverName) ) )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_HOST_ERROR( 0, "Failed to load ASIO driver" );
        goto error;
    }

    memset( &driverInfo->asioDriverInfo, 0, sizeof(ASIODriverInfo) );
    driverInfo->asioDriverInfo.asioVersion = 2;
    driverInfo->asioDriverInfo.sysRef = systemSpecific;
    if( (asioError = ASIOInit( &driverInfo->asioDriverInfo )) != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    }
    else
    {
        asioIsInitialized = 1;
    }

    if( (asioError = ASIOGetChannels(&driverInfo->inputChannelCount,
            &driverInfo->outputChannelCount)) != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    }

    if( (asioError = ASIOGetBufferSize(&driverInfo->bufferMinSize,
            &driverInfo->bufferMaxSize, &driverInfo->bufferPreferredSize,
            &driverInfo->bufferGranularity)) != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    }

    if( ASIOOutputReady() == ASE_OK )
        driverInfo->postOutput = true;
    else
        driverInfo->postOutput = false;

    return result;

error:
    if( asioIsInitialized )
        ASIOExit();

    return result;
}


#define PA_DEFAULTSAMPLERATESEARCHORDER_COUNT_     13   /* must be the same number of elements as in the array below */
static ASIOSampleRate defaultSampleRateSearchOrder_[]
     = {44100.0, 48000.0, 32000.0, 24000.0, 22050.0, 88200.0, 96000.0,
        192000.0, 16000.0, 12000.0, 11025.0, 9600.0, 8000.0 };


/* we look up IsDebuggerPresent at runtime incase it isn't present (on Win95 for example) */
typedef BOOL (WINAPI *IsDebuggerPresentPtr)(VOID);
IsDebuggerPresentPtr IsDebuggerPresent_ = 0;
//FARPROC IsDebuggerPresent_ = 0; // this is the current way to do it apparently according to davidv

PaError PaAsio_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    int i, driverCount;
    PaAsioHostApiRepresentation *asioHostApi;
    PaAsioDeviceInfo *deviceInfoArray;
    char **names;
    PaAsioDriverInfo paAsioDriverInfo;

    asioHostApi = (PaAsioHostApiRepresentation*)PaUtil_AllocateMemory( sizeof(PaAsioHostApiRepresentation) );
    if( !asioHostApi )
    {
        result = paInsufficientMemory;
        goto error;
    }

    asioHostApi->allocations = PaUtil_CreateAllocationGroup();
    if( !asioHostApi->allocations )
    {
        result = paInsufficientMemory;
        goto error;
    }

    asioHostApi->systemSpecific = 0;
    asioHostApi->openAsioDeviceIndex = paNoDevice;

    *hostApi = &asioHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;

    (*hostApi)->info.type = paASIO;
    (*hostApi)->info.name = "ASIO";
    (*hostApi)->info.deviceCount = 0;

    #ifdef WINDOWS
        /* use desktop window as system specific ptr */
        asioHostApi->systemSpecific = GetDesktopWindow();
        CoInitialize(NULL);
    #endif

    /* MUST BE CHECKED : to force fragments loading on Mac */
    loadAsioDriver( "dummy" ); 

    /* driverCount is the number of installed drivers - not necessarily
        the number of installed physical devices. */
    #if MAC
        driverCount = asioDrivers->getNumFragments();
    #elif WINDOWS
        driverCount = asioDrivers->asioGetNumDev();
    #endif

    if( driverCount > 0 )
    {
        names = GetAsioDriverNames( asioHostApi->allocations, driverCount );
        if( !names )
        {
            result = paInsufficientMemory;
            goto error;
        }


        /* allocate enough space for all drivers, even if some aren't installed */

        (*hostApi)->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory(
                asioHostApi->allocations, sizeof(PaDeviceInfo*) * driverCount );
        if( !(*hostApi)->deviceInfos )
        {
            result = paInsufficientMemory;
            goto error;
        }

        /* allocate all device info structs in a contiguous block */
        deviceInfoArray = (PaAsioDeviceInfo*)PaUtil_GroupAllocateMemory(
                asioHostApi->allocations, sizeof(PaAsioDeviceInfo) * driverCount );
        if( !deviceInfoArray )
        {
            result = paInsufficientMemory;
            goto error;
        }

		IsDebuggerPresent_ = GetProcAddress( LoadLibrary( "Kernel32.dll" ), "IsDebuggerPresent" );

        for( i=0; i < driverCount; ++i )
        {

            PA_DEBUG(("ASIO names[%d]:%s\n",i,names[i]));

            // Since portaudio opens ALL ASIO drivers, and no one else does that,
            // we face fact that some drivers were not meant for it, drivers which act
            // like shells on top of REAL drivers, for instance.
            // so we get duplicate handles, locks and other problems.
            // so lets NOT try to load any such wrappers. 
            // The ones i [davidv] know of so far are:

            if (   strcmp (names[i],"ASIO DirectX Full Duplex Driver") == 0
                || strcmp (names[i],"ASIO Multimedia Driver")          == 0
                || strncmp(names[i],"Premiere",8)                      == 0   //"Premiere Elements Windows Sound 1.0"
                || strncmp(names[i],"Adobe",5)                         == 0   //"Adobe Default Windows Sound 1.5"
                || strncmp(names[i],"ReaRoute ASIO",13)                == 0   //Reaper www.reaper.fm <- fix your stuff man.
               )
            {
                PA_DEBUG(("BLACKLISTED!!!\n"));
                continue;
            }


            if( IsDebuggerPresent_ && IsDebuggerPresent_() )  
            {
                /* ASIO Digidesign Driver uses PACE copy protection which quits out
                   if a debugger is running. So we don't load it if a debugger is running. */
                if( strcmp(names[i], "ASIO Digidesign Driver") == 0 )  
                {
                    PA_DEBUG(("BLACKLISTED!!! ASIO Digidesign Driver would quit the debugger\n"));  
                    continue;  
                }  
            }  


            /* Attempt to load the asio driver... */
            if( LoadAsioDriver( names[i], &paAsioDriverInfo, asioHostApi->systemSpecific ) == paNoError )
            {
                PaAsioDeviceInfo *asioDeviceInfo = &deviceInfoArray[ (*hostApi)->info.deviceCount ];
                PaDeviceInfo *deviceInfo = &asioDeviceInfo->commonDeviceInfo;

                deviceInfo->structVersion = 2;
                deviceInfo->hostApi = hostApiIndex;

                deviceInfo->name = names[i];
                PA_DEBUG(("PaAsio_Initialize: drv:%d name = %s\n",  i,deviceInfo->name));
                PA_DEBUG(("PaAsio_Initialize: drv:%d inputChannels       = %d\n", i, paAsioDriverInfo.inputChannelCount));
                PA_DEBUG(("PaAsio_Initialize: drv:%d outputChannels      = %d\n", i, paAsioDriverInfo.outputChannelCount));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferMinSize       = %d\n", i, paAsioDriverInfo.bufferMinSize));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferMaxSize       = %d\n", i, paAsioDriverInfo.bufferMaxSize));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferPreferredSize = %d\n", i, paAsioDriverInfo.bufferPreferredSize));
                PA_DEBUG(("PaAsio_Initialize: drv:%d bufferGranularity   = %d\n", i, paAsioDriverInfo.bufferGranularity));

                deviceInfo->maxInputChannels  = paAsioDriverInfo.inputChannelCount;
                deviceInfo->maxOutputChannels = paAsioDriverInfo.outputChannelCount;

                deviceInfo->defaultSampleRate = 0.;
                bool foundDefaultSampleRate = false;
                for( int j=0; j < PA_DEFAULTSAMPLERATESEARCHORDER_COUNT_; ++j )
                {
                    ASIOError asioError = ASIOCanSampleRate( defaultSampleRateSearchOrder_[j] );
                    if( asioError != ASE_NoClock && asioError != ASE_NotPresent )
                    {
                        deviceInfo->defaultSampleRate = defaultSampleRateSearchOrder_[j];
                        foundDefaultSampleRate = true;
                        break;
                    }
                }

                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultSampleRate = %f\n", i, deviceInfo->defaultSampleRate));

                if( foundDefaultSampleRate ){

                    /* calculate default latency values from bufferPreferredSize
                        for default low latency, and bufferPreferredSize * 3
                        for default high latency.
                        use the default sample rate to convert from samples to
                        seconds. Without knowing what sample rate the user will
                        use this is the best we can do.
                    */

                    double defaultLowLatency =
                            paAsioDriverInfo.bufferPreferredSize / deviceInfo->defaultSampleRate;

                    deviceInfo->defaultLowInputLatency = defaultLowLatency;
                    deviceInfo->defaultLowOutputLatency = defaultLowLatency;

                    long defaultHighLatencyBufferSize =
                            paAsioDriverInfo.bufferPreferredSize * 3;

                    if( defaultHighLatencyBufferSize > paAsioDriverInfo.bufferMaxSize )
                        defaultHighLatencyBufferSize = paAsioDriverInfo.bufferMaxSize;

                    double defaultHighLatency =
                            defaultHighLatencyBufferSize / deviceInfo->defaultSampleRate;

                    if( defaultHighLatency < defaultLowLatency )
                        defaultHighLatency = defaultLowLatency; /* just incase the driver returns something strange */ 
                            
                    deviceInfo->defaultHighInputLatency = defaultHighLatency;
                    deviceInfo->defaultHighOutputLatency = defaultHighLatency;
                    
                }else{

                    deviceInfo->defaultLowInputLatency = 0.;
                    deviceInfo->defaultLowOutputLatency = 0.;
                    deviceInfo->defaultHighInputLatency = 0.;
                    deviceInfo->defaultHighOutputLatency = 0.;
                }

                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultLowInputLatency = %f\n", i, deviceInfo->defaultLowInputLatency));
                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultLowOutputLatency = %f\n", i, deviceInfo->defaultLowOutputLatency));
                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultHighInputLatency = %f\n", i, deviceInfo->defaultHighInputLatency));
                PA_DEBUG(("PaAsio_Initialize: drv:%d defaultHighOutputLatency = %f\n", i, deviceInfo->defaultHighOutputLatency));

                asioDeviceInfo->minBufferSize = paAsioDriverInfo.bufferMinSize;
                asioDeviceInfo->maxBufferSize = paAsioDriverInfo.bufferMaxSize;
                asioDeviceInfo->preferredBufferSize = paAsioDriverInfo.bufferPreferredSize;
                asioDeviceInfo->bufferGranularity = paAsioDriverInfo.bufferGranularity;


                asioDeviceInfo->asioChannelInfos = (ASIOChannelInfo*)PaUtil_GroupAllocateMemory(
                        asioHostApi->allocations,
                        sizeof(ASIOChannelInfo) * (deviceInfo->maxInputChannels
                                + deviceInfo->maxOutputChannels) );
                if( !asioDeviceInfo->asioChannelInfos )
                {
                    result = paInsufficientMemory;
                    goto error;
                }

                int a;

                for( a=0; a < deviceInfo->maxInputChannels; ++a ){
                    asioDeviceInfo->asioChannelInfos[a].channel = a;
                    asioDeviceInfo->asioChannelInfos[a].isInput = ASIOTrue;
                    ASIOError asioError = ASIOGetChannelInfo( &asioDeviceInfo->asioChannelInfos[a] );
                    if( asioError != ASE_OK )
                    {
                        result = paUnanticipatedHostError;
                        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
                        goto error;
                    }
                }

                for( a=0; a < deviceInfo->maxOutputChannels; ++a ){
                    int b = deviceInfo->maxInputChannels + a;
                    asioDeviceInfo->asioChannelInfos[b].channel = a;
                    asioDeviceInfo->asioChannelInfos[b].isInput = ASIOFalse;
                    ASIOError asioError = ASIOGetChannelInfo( &asioDeviceInfo->asioChannelInfos[b] );
                    if( asioError != ASE_OK )
                    {
                        result = paUnanticipatedHostError;
                        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
                        goto error;
                    }
                }


                /* unload the driver */
                ASIOExit();

                (*hostApi)->deviceInfos[ (*hostApi)->info.deviceCount ] = deviceInfo;
                ++(*hostApi)->info.deviceCount;
            }
        }
    }

    if( (*hostApi)->info.deviceCount > 0 )
    {
        (*hostApi)->info.defaultInputDevice = 0;
        (*hostApi)->info.defaultOutputDevice = 0;
    }
    else
    {
        (*hostApi)->info.defaultInputDevice = paNoDevice;
        (*hostApi)->info.defaultOutputDevice = paNoDevice;
    }


    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &asioHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &asioHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );

    return result;

error:
    if( asioHostApi )
    {
        if( asioHostApi->allocations )
        {
            PaUtil_FreeAllAllocations( asioHostApi->allocations );
            PaUtil_DestroyAllocationGroup( asioHostApi->allocations );
        }

        PaUtil_FreeMemory( asioHostApi );
    }
    return result;
}


static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaAsioHostApiRepresentation *asioHostApi = (PaAsioHostApiRepresentation*)hostApi;

    /*
        IMPLEMENT ME:
            - clean up any resources not handled by the allocation group
    */

    if( asioHostApi->allocations )
    {
        PaUtil_FreeAllAllocations( asioHostApi->allocations );
        PaUtil_DestroyAllocationGroup( asioHostApi->allocations );
    }

    PaUtil_FreeMemory( asioHostApi );
}


static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    PaError result = paNoError;
    PaAsioHostApiRepresentation *asioHostApi = (PaAsioHostApiRepresentation*)hostApi;
    PaAsioDriverInfo *driverInfo = &asioHostApi->openAsioDriverInfo;
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaDeviceIndex asioDeviceIndex;                                  
    ASIOError asioError;
    
    if( inputParameters && outputParameters )
    {
        /* full duplex ASIO stream must use the same device for input and output */

        if( inputParameters->device != outputParameters->device )
            return paBadIODeviceCombination;
    }
    
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

        asioDeviceIndex = inputParameters->device;

        /* validate inputStreamInfo */
        /** @todo do more validation here */
        // if( inputParameters->hostApiSpecificStreamInfo )
        //    return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */
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

        asioDeviceIndex = outputParameters->device;

        /* validate outputStreamInfo */
        /** @todo do more validation here */
        // if( outputParameters->hostApiSpecificStreamInfo )
        //    return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */
    }
    else
    {
        outputChannelCount = 0;
    }



    /* if an ASIO device is open we can only get format information for the currently open device */

    if( asioHostApi->openAsioDeviceIndex != paNoDevice 
			&& asioHostApi->openAsioDeviceIndex != asioDeviceIndex )
    {
        return paDeviceUnavailable;
    }


    /* NOTE: we load the driver and use its current settings
        rather than the ones in our device info structure which may be stale */

    /* open the device if it's not already open */
    if( asioHostApi->openAsioDeviceIndex == paNoDevice )
    {
        result = LoadAsioDriver( asioHostApi->inheritedHostApiRep.deviceInfos[ asioDeviceIndex ]->name,
                driverInfo, asioHostApi->systemSpecific );
        if( result != paNoError )
            return result;
    }

    /* check that input device can support inputChannelCount */
    if( inputChannelCount > 0 )
    {
        if( inputChannelCount > driverInfo->inputChannelCount )
        {
            result = paInvalidChannelCount;
            goto done;
        }
    }

    /* check that output device can support outputChannelCount */
    if( outputChannelCount )
    {
        if( outputChannelCount > driverInfo->outputChannelCount )
        {
            result = paInvalidChannelCount;
            goto done;
        }
    }
    
    /* query for sample rate support */
    asioError = ASIOCanSampleRate( sampleRate );
    if( asioError == ASE_NoClock || asioError == ASE_NotPresent )
    {
        result = paInvalidSampleRate;
        goto done;
    }

done:
    /* close the device if it wasn't already open */
    if( asioHostApi->openAsioDeviceIndex == paNoDevice )
    {
        ASIOExit(); /* not sure if we should check for errors here */
    }

    if( result == paNoError )
        return paFormatIsSupported;
    else
        return result;
}



/* PaAsioStream - a stream data structure specifically for this implementation */

typedef struct PaAsioStream
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    PaAsioHostApiRepresentation *asioHostApi;
    unsigned long framesPerHostCallback;

    /* ASIO driver info  - these may not be needed for the life of the stream,
        but store them here until we work out how format conversion is going
        to work. */

    ASIOBufferInfo *asioBufferInfos;
    ASIOChannelInfo *asioChannelInfos;
    long inputLatency, outputLatency; // actual latencies returned by asio

    long inputChannelCount, outputChannelCount;
    bool postOutput;

    void **bufferPtrs; /* this is carved up for inputBufferPtrs and outputBufferPtrs */
    void **inputBufferPtrs[2];
    void **outputBufferPtrs[2];

    PaAsioBufferConverter *inputBufferConverter;
    long inputShift;
    PaAsioBufferConverter *outputBufferConverter;
    long outputShift;

    volatile bool stopProcessing;
    int stopPlayoutCount;
    HANDLE completedBuffersPlayedEvent;

    bool streamFinishedCallbackCalled;
    volatile int isActive;
    volatile bool zeroOutput; /* all future calls to the callback will output silence */

    volatile long reenterCount;
    volatile long reenterError;

    PaStreamCallbackFlags callbackFlags;
}
PaAsioStream;

static PaAsioStream *theAsioStream = 0; /* due to ASIO sdk limitations there can be only one stream */


static void ZeroOutputBuffers( PaAsioStream *stream, long index )
{
    int i;

    for( i=0; i < stream->outputChannelCount; ++i )
    {
        void *buffer = stream->asioBufferInfos[ i + stream->inputChannelCount ].buffers[index];

        int bytesPerSample = BytesPerAsioSample( stream->asioChannelInfos[ i + stream->inputChannelCount ].type );

        memset( buffer, 0, stream->framesPerHostCallback * bytesPerSample );
    }
}


static unsigned long SelectHostBufferSize( unsigned long suggestedLatencyFrames,
        PaAsioDriverInfo *driverInfo )
{
    unsigned long result;

    if( suggestedLatencyFrames == 0 )
    {
        result = driverInfo->bufferPreferredSize;
    }
    else{
        if( suggestedLatencyFrames <= (unsigned long)driverInfo->bufferMinSize )
        {
            result = driverInfo->bufferMinSize;
        }
        else if( suggestedLatencyFrames >= (unsigned long)driverInfo->bufferMaxSize )
        {
            result = driverInfo->bufferMaxSize;
        }
        else
        {
            if( driverInfo->bufferGranularity == -1 )
            {
                /* power-of-two */
                result = 2;

                while( result < suggestedLatencyFrames )
                    result *= 2;

                if( result < (unsigned long)driverInfo->bufferMinSize )
                    result = driverInfo->bufferMinSize;

                if( result > (unsigned long)driverInfo->bufferMaxSize )
                    result = driverInfo->bufferMaxSize;
            }
            else if( driverInfo->bufferGranularity == 0 )
            {
                /* the documentation states that bufferGranularity should be
                    zero when bufferMinSize, bufferMaxSize and
                    bufferPreferredSize are the same. We assume that is the case.
                */

                result = driverInfo->bufferPreferredSize;
            }
            else
            {
                /* modulo granularity */

                unsigned long remainder =
                        suggestedLatencyFrames % driverInfo->bufferGranularity;

                if( remainder == 0 )
                {
                    result = suggestedLatencyFrames;
                }
                else
                {
                    result = suggestedLatencyFrames
                            + (driverInfo->bufferGranularity - remainder);

                    if( result > (unsigned long)driverInfo->bufferMaxSize )
                        result = driverInfo->bufferMaxSize;
                }
            }
        }
    }

    return result;
}


/* returns channelSelectors if present */

static PaError ValidateAsioSpecificStreamInfo(
        const PaStreamParameters *streamParameters,
        const PaAsioStreamInfo *streamInfo,
        int deviceChannelCount,
        int **channelSelectors )
{
	if( streamInfo )
	{
	    if( streamInfo->size != sizeof( PaAsioStreamInfo )
	            || streamInfo->version != 1 )
	    {
	        return paIncompatibleHostApiSpecificStreamInfo;
	    }

	    if( streamInfo->flags & paAsioUseChannelSelectors )
            *channelSelectors = streamInfo->channelSelectors;

        if( !(*channelSelectors) )
            return paIncompatibleHostApiSpecificStreamInfo;

        for( int i=0; i < streamParameters->channelCount; ++i ){
             if( (*channelSelectors)[i] < 0
                    || (*channelSelectors)[i] >= deviceChannelCount ){
                return paInvalidChannelCount;
             }           
        }
	}

	return paNoError;
}


/* see pa_hostapi.h for a list of validity guarantees made about OpenStream  parameters */

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
    PaAsioHostApiRepresentation *asioHostApi = (PaAsioHostApiRepresentation*)hostApi;
    PaAsioStream *stream = 0;
    PaAsioStreamInfo *inputStreamInfo, *outputStreamInfo;
    unsigned long framesPerHostBuffer;
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaSampleFormat hostInputSampleFormat, hostOutputSampleFormat;
    unsigned long suggestedInputLatencyFrames;
    unsigned long suggestedOutputLatencyFrames;
    PaDeviceIndex asioDeviceIndex;
    ASIOError asioError;
    int asioIsInitialized = 0;
    int asioBuffersCreated = 0;
    int completedBuffersPlayedEventInited = 0;
    int i;
    PaAsioDriverInfo *driverInfo;
    int *inputChannelSelectors = 0;
    int *outputChannelSelectors = 0;
    bool isExternal = false;

    /* unless we move to using lower level ASIO calls, we can only have
        one device open at a time */
    if( asioHostApi->openAsioDeviceIndex != paNoDevice ){
        PA_DEBUG(("OpenStream paDeviceUnavailable\n"));
        return paDeviceUnavailable;
    }

    if( inputParameters && outputParameters )
    {
        /* full duplex ASIO stream must use the same device for input and output */

        if( inputParameters->device != outputParameters->device ){
            PA_DEBUG(("OpenStream paBadIODeviceCombination\n"));
            return paBadIODeviceCombination;
    }
    }

    if( inputParameters )
    {
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;
        suggestedInputLatencyFrames = (unsigned long)((inputParameters->suggestedLatency * sampleRate)+0.5f);

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */
        if( inputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        asioDeviceIndex = inputParameters->device;

        PaAsioDeviceInfo *asioDeviceInfo = (PaAsioDeviceInfo*)hostApi->deviceInfos[asioDeviceIndex];

        /* validate hostApiSpecificStreamInfo */
        inputStreamInfo = (PaAsioStreamInfo*)inputParameters->hostApiSpecificStreamInfo;
        result = ValidateAsioSpecificStreamInfo( inputParameters, inputStreamInfo,
            asioDeviceInfo->commonDeviceInfo.maxInputChannels,
            &inputChannelSelectors
        );
        if( result != paNoError ) return result;
    }
    else
    {
        inputChannelCount = 0;
        inputSampleFormat = 0;
        suggestedInputLatencyFrames = 0;
    }

    if( outputParameters )
    {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
        suggestedOutputLatencyFrames = (unsigned long)((outputParameters->suggestedLatency * sampleRate)+0.5f);

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */
        if( outputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        asioDeviceIndex = outputParameters->device;

        PaAsioDeviceInfo *asioDeviceInfo = (PaAsioDeviceInfo*)hostApi->deviceInfos[asioDeviceIndex];

        /* validate hostApiSpecificStreamInfo */
        outputStreamInfo = (PaAsioStreamInfo*)outputParameters->hostApiSpecificStreamInfo;
        result = ValidateAsioSpecificStreamInfo( outputParameters, outputStreamInfo,
            asioDeviceInfo->commonDeviceInfo.maxOutputChannels,
            &outputChannelSelectors
        );
        if( result != paNoError ) return result;
    }
    else
    {
        outputChannelCount = 0;
        outputSampleFormat = 0;
        suggestedOutputLatencyFrames = 0;
    }

    driverInfo = &asioHostApi->openAsioDriverInfo;

    /* NOTE: we load the driver and use its current settings
        rather than the ones in our device info structure which may be stale */

    result = LoadAsioDriver( asioHostApi->inheritedHostApiRep.deviceInfos[ asioDeviceIndex ]->name,
            driverInfo, asioHostApi->systemSpecific );
    if( result == paNoError )
        asioIsInitialized = 1;
    else{
        PA_DEBUG(("OpenStream ERROR1\n"));
        goto error;
    }

    /* check that input device can support inputChannelCount */
    if( inputChannelCount > 0 )
    {
        if( inputChannelCount > driverInfo->inputChannelCount )
        {
            result = paInvalidChannelCount;
            PA_DEBUG(("OpenStream ERROR2\n"));
            goto error;
        }
    }

    /* check that output device can support outputChannelCount */
    if( outputChannelCount )
    {
        if( outputChannelCount > driverInfo->outputChannelCount )
        {
            result = paInvalidChannelCount;
            PA_DEBUG(("OpenStream ERROR3\n"));
            goto error;
        }
    }


    /* davidv: listing ASIO Clock sources, there is an ongoing investigation by
       me about whether or not call ASIOSetSampleRate if an external Clock is
       used. A few drivers expected different things here */
    {
        ASIOClockSource clocks[32];
        long numSources=32;
        asioError = ASIOGetClockSources(clocks, &numSources);
        if( asioError != ASE_OK ){
            PA_DEBUG(("ERROR: ASIOGetClockSources: %s\n", PaAsio_GetAsioErrorText(asioError) ));
        }else{
            PA_DEBUG(("INFO ASIOGetClockSources listing %d clocks\n", numSources ));
            for (int i=0;i<numSources;++i){
                PA_DEBUG(("ASIOClockSource%d %s current:%d\n", i,clocks[i].name, clocks[i].isCurrentSource ));
               
                /*
                  If you have problems with some drivers when externally clocked, 
                  uncomment the next two lines
                 */
                //if (clocks[i].isCurrentSource)
                //    isExternal = true;
            }
        }
    }

    // check that the device supports the requested sample rate 

    asioError = ASIOCanSampleRate( sampleRate );
    PA_DEBUG(("ASIOCanSampleRate(%f):%d\n",sampleRate, asioError ));

    if( asioError != ASE_OK )
    {
        result = paInvalidSampleRate;
        PA_DEBUG(("ERROR: ASIOCanSampleRate: %s\n", PaAsio_GetAsioErrorText(asioError) ));
        goto error;
    }


    // retrieve the current sample rate, we only change to the requested
    // sample rate if the device is not already in that rate.

    ASIOSampleRate oldRate;
    asioError = ASIOGetSampleRate(&oldRate);
    if( asioError != ASE_OK )
    {
        result = paInvalidSampleRate;
        PA_DEBUG(("ERROR: ASIOGetSampleRate: %s\n", PaAsio_GetAsioErrorText(asioError) ));
        goto error;
    }
    PA_DEBUG(("ASIOGetSampleRate:%f\n",oldRate));

    if (oldRate != sampleRate){

        PA_DEBUG(("before ASIOSetSampleRate(%f)\n",sampleRate));

        asioError = ASIOSetSampleRate( isExternal?0:sampleRate );
        /* Set sample rate */
        if( asioError != ASE_OK )
        {
            result = paInvalidSampleRate;
            PA_DEBUG(("ERROR: ASIOSetSampleRate: %s\n", PaAsio_GetAsioErrorText(asioError) ));
            goto error;
        }
        PA_DEBUG(("after ASIOSetSampleRate(%f)\n",sampleRate));
    }
    else
    {
        PA_DEBUG(("No Need to change SR\n"));
    }


    /*
        IMPLEMENT ME:
            - if a full duplex stream is requested, check that the combination
                of input and output parameters is supported
    */

    /* validate platform specific flags */
    if( (streamFlags & paPlatformSpecificFlags) != 0 ){
        PA_DEBUG(("OpenStream invalid flags!!\n"));
        return paInvalidFlag; /* unexpected platform specific flag */
    }


    stream = (PaAsioStream*)PaUtil_AllocateMemory( sizeof(PaAsioStream) );
    if( !stream )
    {
        result = paInsufficientMemory;
        PA_DEBUG(("OpenStream ERROR5\n"));
        goto error;
    }

    stream->completedBuffersPlayedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( stream->completedBuffersPlayedEvent == NULL )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_SYSTEM_ERROR( GetLastError() );
        PA_DEBUG(("OpenStream ERROR6\n"));
        goto error;
    }
    completedBuffersPlayedEventInited = 1;


    stream->asioBufferInfos = 0; /* for deallocation in error */
    stream->asioChannelInfos = 0; /* for deallocation in error */
    stream->bufferPtrs = 0; /* for deallocation in error */

    if( streamCallback )
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &asioHostApi->callbackStreamInterface, streamCallback, userData );
    }
    else
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &asioHostApi->blockingStreamInterface, streamCallback, userData );
    }


    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, sampleRate );


    stream->asioBufferInfos = (ASIOBufferInfo*)PaUtil_AllocateMemory(
            sizeof(ASIOBufferInfo) * (inputChannelCount + outputChannelCount) );
    if( !stream->asioBufferInfos )
    {
        result = paInsufficientMemory;
        PA_DEBUG(("OpenStream ERROR7\n"));
        goto error;
    }


    for( i=0; i < inputChannelCount; ++i )
    {
        ASIOBufferInfo *info = &stream->asioBufferInfos[i];

        info->isInput = ASIOTrue;

        if( inputChannelSelectors ){
            // inputChannelSelectors values have already been validated in
            // ValidateAsioSpecificStreamInfo() above
            info->channelNum = inputChannelSelectors[i];
        }else{
            info->channelNum = i;
        }

        info->buffers[0] = info->buffers[1] = 0;
    }

    for( i=0; i < outputChannelCount; ++i ){
        ASIOBufferInfo *info = &stream->asioBufferInfos[inputChannelCount+i];

        info->isInput = ASIOFalse;

        if( outputChannelSelectors ){
            // outputChannelSelectors values have already been validated in
            // ValidateAsioSpecificStreamInfo() above
            info->channelNum = outputChannelSelectors[i];
        }else{
            info->channelNum = i;
        }
        
        info->buffers[0] = info->buffers[1] = 0;
    }


    framesPerHostBuffer = SelectHostBufferSize(
            (( suggestedInputLatencyFrames > suggestedOutputLatencyFrames )
                    ? suggestedInputLatencyFrames : suggestedOutputLatencyFrames),
            driverInfo );


	PA_DEBUG(("PaAsioOpenStream: framesPerHostBuffer :%d\n",  framesPerHostBuffer));

    asioError = ASIOCreateBuffers( stream->asioBufferInfos,
            inputChannelCount+outputChannelCount,
            framesPerHostBuffer, &asioCallbacks_ );

    if( asioError != ASE_OK
            && framesPerHostBuffer != (unsigned long)driverInfo->bufferPreferredSize )
    {
        PA_DEBUG(("ERROR: ASIOCreateBuffers: %s\n", PaAsio_GetAsioErrorText(asioError) ));
        /*
            Some buggy drivers (like the Hoontech DSP24) give incorrect
            [min, preferred, max] values They should work with the preferred size
            value, thus if Pa_ASIO_CreateBuffers fails with the hostBufferSize
            computed in SelectHostBufferSize, we try again with the preferred size.
        */

        framesPerHostBuffer = driverInfo->bufferPreferredSize;

        PA_DEBUG(("PaAsioOpenStream: CORRECTED framesPerHostBuffer :%d\n",  framesPerHostBuffer));

        ASIOError asioError2 = ASIOCreateBuffers( stream->asioBufferInfos,
                inputChannelCount+outputChannelCount,
                 framesPerHostBuffer, &asioCallbacks_ );
        if( asioError2 == ASE_OK )
            asioError = ASE_OK;
    }

    if( asioError != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        PA_DEBUG(("OpenStream ERROR9\n"));
        goto error;
    }

    asioBuffersCreated = 1;

    stream->asioChannelInfos = (ASIOChannelInfo*)PaUtil_AllocateMemory(
            sizeof(ASIOChannelInfo) * (inputChannelCount + outputChannelCount) );
    if( !stream->asioChannelInfos )
    {
        result = paInsufficientMemory;
        PA_DEBUG(("OpenStream ERROR10\n"));
        goto error;
    }

    for( i=0; i < inputChannelCount + outputChannelCount; ++i )
    {
        stream->asioChannelInfos[i].channel = stream->asioBufferInfos[i].channelNum;
        stream->asioChannelInfos[i].isInput = stream->asioBufferInfos[i].isInput;
        asioError = ASIOGetChannelInfo( &stream->asioChannelInfos[i] );
        if( asioError != ASE_OK )
        {
            result = paUnanticipatedHostError;
            PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
            PA_DEBUG(("OpenStream ERROR11\n"));
            goto error;
        }
    }

    stream->bufferPtrs = (void**)PaUtil_AllocateMemory(
            2 * sizeof(void*) * (inputChannelCount + outputChannelCount) );
    if( !stream->bufferPtrs )
    {
        result = paInsufficientMemory;
        PA_DEBUG(("OpenStream ERROR12\n"));
        goto error;
    }

    if( inputChannelCount > 0 )
    {
        stream->inputBufferPtrs[0] = stream-> bufferPtrs;
        stream->inputBufferPtrs[1] = &stream->bufferPtrs[inputChannelCount];

        for( i=0; i<inputChannelCount; ++i )
        {
            stream->inputBufferPtrs[0][i] = stream->asioBufferInfos[i].buffers[0];
            stream->inputBufferPtrs[1][i] = stream->asioBufferInfos[i].buffers[1];
        }
    }
    else
    {
        stream->inputBufferPtrs[0] = 0;
        stream->inputBufferPtrs[1] = 0;
    }

    if( outputChannelCount > 0 )
    {
        stream->outputBufferPtrs[0] = &stream->bufferPtrs[inputChannelCount*2];
        stream->outputBufferPtrs[1] = &stream->bufferPtrs[inputChannelCount*2 + outputChannelCount];

        for( i=0; i<outputChannelCount; ++i )
        {
            stream->outputBufferPtrs[0][i] = stream->asioBufferInfos[inputChannelCount+i].buffers[0];
            stream->outputBufferPtrs[1][i] = stream->asioBufferInfos[inputChannelCount+i].buffers[1];
        }
    }
    else
    {
        stream->outputBufferPtrs[0] = 0;
        stream->outputBufferPtrs[1] = 0;
    }

    if( inputChannelCount > 0 )
    {
        /* FIXME: assume all channels use the same type for now */
        ASIOSampleType inputType = stream->asioChannelInfos[0].type;

        PA_DEBUG(("ASIO Input  type:%d",inputType));
        AsioSampleTypeLOG(inputType);
        hostInputSampleFormat = AsioSampleTypeToPaNativeSampleFormat( inputType );

        SelectAsioToPaConverter( inputType, &stream->inputBufferConverter, &stream->inputShift );
    }
    else
    {
        hostInputSampleFormat = 0;
        stream->inputBufferConverter = 0;
    }

    if( outputChannelCount > 0 )
    {
        /* FIXME: assume all channels use the same type for now */
        ASIOSampleType outputType = stream->asioChannelInfos[inputChannelCount].type;

        PA_DEBUG(("ASIO Output type:%d",outputType));
        AsioSampleTypeLOG(outputType);
        hostOutputSampleFormat = AsioSampleTypeToPaNativeSampleFormat( outputType );

        SelectPaToAsioConverter( outputType, &stream->outputBufferConverter, &stream->outputShift );
    }
    else
    {
        hostOutputSampleFormat = 0;
        stream->outputBufferConverter = 0;
    }

    result =  PaUtil_InitializeBufferProcessor( &stream->bufferProcessor,
                    inputChannelCount, inputSampleFormat, hostInputSampleFormat,
                    outputChannelCount, outputSampleFormat, hostOutputSampleFormat,
                    sampleRate, streamFlags, framesPerBuffer,
                    framesPerHostBuffer, paUtilFixedHostBufferSize,
                    streamCallback, userData );
    if( result != paNoError ){
        PA_DEBUG(("OpenStream ERROR13\n"));
        goto error;
    }


    ASIOGetLatencies( &stream->inputLatency, &stream->outputLatency );

    stream->streamRepresentation.streamInfo.inputLatency =
            (double)( PaUtil_GetBufferProcessorInputLatency(&stream->bufferProcessor)
                + stream->inputLatency) / sampleRate;   // seconds
    stream->streamRepresentation.streamInfo.outputLatency =
            (double)( PaUtil_GetBufferProcessorOutputLatency(&stream->bufferProcessor)
                + stream->outputLatency) / sampleRate; // seconds
    stream->streamRepresentation.streamInfo.sampleRate = sampleRate;

    // the code below prints the ASIO latency which doesn't include the
    // buffer processor latency. it reports the added latency separately
    PA_DEBUG(("PaAsio : ASIO InputLatency = %ld (%ld ms), added buffProc:%ld (%ld ms)\n",
            stream->inputLatency,
            (long)((stream->inputLatency*1000)/ sampleRate),  
            PaUtil_GetBufferProcessorInputLatency(&stream->bufferProcessor),
            (long)((PaUtil_GetBufferProcessorInputLatency(&stream->bufferProcessor)*1000)/ sampleRate)
            ));

    PA_DEBUG(("PaAsio : ASIO OuputLatency = %ld (%ld ms), added buffProc:%ld (%ld ms)\n",
            stream->outputLatency,
            (long)((stream->outputLatency*1000)/ sampleRate), 
            PaUtil_GetBufferProcessorOutputLatency(&stream->bufferProcessor),
            (long)((PaUtil_GetBufferProcessorOutputLatency(&stream->bufferProcessor)*1000)/ sampleRate)
            ));

    stream->asioHostApi = asioHostApi;
    stream->framesPerHostCallback = framesPerHostBuffer;

    stream->inputChannelCount = inputChannelCount;
    stream->outputChannelCount = outputChannelCount;
    stream->postOutput = driverInfo->postOutput;
    stream->isActive = 0;

    asioHostApi->openAsioDeviceIndex = asioDeviceIndex;

    *s = (PaStream*)stream;

    return result;

error:
    PA_DEBUG(("goto errored\n"));
    if( stream )
    {
        if( completedBuffersPlayedEventInited )
            CloseHandle( stream->completedBuffersPlayedEvent );

        if( stream->asioBufferInfos )
            PaUtil_FreeMemory( stream->asioBufferInfos );

        if( stream->asioChannelInfos )
            PaUtil_FreeMemory( stream->asioChannelInfos );

        if( stream->bufferPtrs )
            PaUtil_FreeMemory( stream->bufferPtrs );

        PaUtil_FreeMemory( stream );
    }

    if( asioBuffersCreated )
        ASIODisposeBuffers();

    if( asioIsInitialized )
        ASIOExit();

    return result;
}


/*
    When CloseStream() is called, the multi-api layer ensures that
    the stream has already been stopped or aborted.
*/
static PaError CloseStream( PaStream* s )
{
    PaError result = paNoError;
    PaAsioStream *stream = (PaAsioStream*)s;

    /*
        IMPLEMENT ME:
            - additional stream closing + cleanup
    */

    PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );
    PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );

    stream->asioHostApi->openAsioDeviceIndex = paNoDevice;

    CloseHandle( stream->completedBuffersPlayedEvent );

    PaUtil_FreeMemory( stream->asioBufferInfos );
    PaUtil_FreeMemory( stream->asioChannelInfos );
    PaUtil_FreeMemory( stream->bufferPtrs );
    PaUtil_FreeMemory( stream );

    ASIODisposeBuffers();
    ASIOExit();

    return result;
}


static void bufferSwitch(long index, ASIOBool directProcess)
{
//TAKEN FROM THE ASIO SDK

    // the actual processing callback.
    // Beware that this is normally in a seperate thread, hence be sure that
    // you take care about thread synchronization. This is omitted here for
    // simplicity.

    // as this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs
    // to be created though it will only set the timeInfo.samplePosition and
    // timeInfo.systemTime fields and the according flags

    ASIOTime  timeInfo;
    memset( &timeInfo, 0, sizeof (timeInfo) );

    // get the time stamp of the buffer, not necessary if no
    // synchronization to other media is required
    if( ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK)
            timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;

    // Call the real callback
    bufferSwitchTimeInfo( &timeInfo, index, directProcess );
}


// conversion from 64 bit ASIOSample/ASIOTimeStamp to double float
#if NATIVE_INT64
	#define ASIO64toDouble(a)  (a)
#else
	const double twoRaisedTo32 = 4294967296.;
	#define ASIO64toDouble(a)  ((a).lo + (a).hi * twoRaisedTo32)
#endif

static ASIOTime *bufferSwitchTimeInfo( ASIOTime *timeInfo, long index, ASIOBool directProcess )
{
    // the actual processing callback.
    // Beware that this is normally in a seperate thread, hence be sure that
    // you take care about thread synchronization.


    /* The SDK says the following about the directProcess flag:
        suggests to the host whether it should immediately start processing
        (directProcess == ASIOTrue), or whether its process should be deferred
        because the call comes from a very low level (for instance, a high level
        priority interrupt), and direct processing would cause timing instabilities for
        the rest of the system. If in doubt, directProcess should be set to ASIOFalse.

        We just ignore directProcess. This could cause incompatibilities with
        drivers which really don't want the audio processing to occur in this
        callback, but none have been identified yet.
    */

    (void) directProcess; /* suppress unused parameter warning */

#if 0
    // store the timeInfo for later use
    asioDriverInfo.tInfo = *timeInfo;

    // get the time stamp of the buffer, not necessary if no
    // synchronization to other media is required

    if (timeInfo->timeInfo.flags & kSystemTimeValid)
            asioDriverInfo.nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
    else
            asioDriverInfo.nanoSeconds = 0;

    if (timeInfo->timeInfo.flags & kSamplePositionValid)
            asioDriverInfo.samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
    else
            asioDriverInfo.samples = 0;

    if (timeInfo->timeCode.flags & kTcValid)
            asioDriverInfo.tcSamples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
    else
            asioDriverInfo.tcSamples = 0;

    // get the system reference time
    asioDriverInfo.sysRefTime = get_sys_reference_time();
#endif

#if 0
    // a few debug messages for the Windows device driver developer
    // tells you the time when driver got its interrupt and the delay until the app receives
    // the event notification.
    static double last_samples = 0;
    char tmp[128];
    sprintf (tmp, "diff: %d / %d ms / %d ms / %d samples                 \n", asioDriverInfo.sysRefTime - (long)(asioDriverInfo.nanoSeconds / 1000000.0), asioDriverInfo.sysRefTime, (long)(asioDriverInfo.nanoSeconds / 1000000.0), (long)(asioDriverInfo.samples - last_samples));
    OutputDebugString (tmp);
    last_samples = asioDriverInfo.samples;
#endif


    if( !theAsioStream )
        return 0L;

    // Keep sample position
    // FIXME: asioDriverInfo.pahsc_NumFramesDone = timeInfo->timeInfo.samplePosition.lo;


    // protect against reentrancy
    if( PaAsio_AtomicIncrement(&theAsioStream->reenterCount) )
    {
        theAsioStream->reenterError++;
        //DBUG(("bufferSwitchTimeInfo : reentrancy detection = %d\n", asioDriverInfo.reenterError));
        return 0L;
    }

    int buffersDone = 0;
    
    do
    {
        if( buffersDone > 0 )
        {
            // this is a reentered buffer, we missed processing it on time
            // set the input overflow and output underflow flags as appropriate
            
            if( theAsioStream->inputChannelCount > 0 )
                theAsioStream->callbackFlags |= paInputOverflow;
                
            if( theAsioStream->outputChannelCount > 0 )
                theAsioStream->callbackFlags |= paOutputUnderflow;
        }
        else
        {
            if( theAsioStream->zeroOutput )
            {
                ZeroOutputBuffers( theAsioStream, index );

                // Finally if the driver supports the ASIOOutputReady() optimization,
                // do it here, all data are in place
                if( theAsioStream->postOutput )
                    ASIOOutputReady();

                if( theAsioStream->stopProcessing )
                {
                    if( theAsioStream->stopPlayoutCount < 2 )
                    {
                        ++theAsioStream->stopPlayoutCount;
                        if( theAsioStream->stopPlayoutCount == 2 )
                        {
                            theAsioStream->isActive = 0;
                            if( theAsioStream->streamRepresentation.streamFinishedCallback != 0 )
                                theAsioStream->streamRepresentation.streamFinishedCallback( theAsioStream->streamRepresentation.userData );
                            theAsioStream->streamFinishedCallbackCalled = true;
                            SetEvent( theAsioStream->completedBuffersPlayedEvent );
                        }
                    }
                }
            }
            else
            {

#if 0
// test code to try to detect slip conditions... these may work on some systems
// but neither of them work on the RME Digi96

// check that sample delta matches buffer size (otherwise we must have skipped
// a buffer.
static double last_samples = -512;
double samples;
//if( timeInfo->timeCode.flags & kTcValid )
//    samples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
//else
    samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
int delta = samples - last_samples;
//printf( "%d\n", delta);
last_samples = samples;

if( delta > theAsioStream->framesPerHostCallback )
{
    if( theAsioStream->inputChannelCount > 0 )
        theAsioStream->callbackFlags |= paInputOverflow;

    if( theAsioStream->outputChannelCount > 0 )
        theAsioStream->callbackFlags |= paOutputUnderflow;
}

// check that the buffer index is not the previous index (which would indicate
// that a buffer was skipped.
static int previousIndex = 1;
if( index == previousIndex )
{
    if( theAsioStream->inputChannelCount > 0 )
        theAsioStream->callbackFlags |= paInputOverflow;

    if( theAsioStream->outputChannelCount > 0 )
        theAsioStream->callbackFlags |= paOutputUnderflow;
}
previousIndex = index;
#endif

                int i;

                PaUtil_BeginCpuLoadMeasurement( &theAsioStream->cpuLoadMeasurer );

                PaStreamCallbackTimeInfo paTimeInfo;

                // asio systemTime is supposed to be measured according to the same
                // clock as timeGetTime
                paTimeInfo.currentTime = (ASIO64toDouble( timeInfo->timeInfo.systemTime ) * .000000001);

                /* patch from Paul Boege */
                paTimeInfo.inputBufferAdcTime = paTimeInfo.currentTime -
                    ((double)theAsioStream->inputLatency/theAsioStream->streamRepresentation.streamInfo.sampleRate);

                paTimeInfo.outputBufferDacTime = paTimeInfo.currentTime +
                    ((double)theAsioStream->outputLatency/theAsioStream->streamRepresentation.streamInfo.sampleRate);

                /* old version is buggy because the buffer processor also adds in its latency to the time parameters
                paTimeInfo.inputBufferAdcTime = paTimeInfo.currentTime - theAsioStream->streamRepresentation.streamInfo.inputLatency;
                paTimeInfo.outputBufferDacTime = paTimeInfo.currentTime + theAsioStream->streamRepresentation.streamInfo.outputLatency;
                */
#if 1
// detect underflows by checking inter-callback time > 2 buffer period
static double previousTime = -1;
if( previousTime > 0 ){

    double delta = paTimeInfo.currentTime - previousTime;

    if( delta >= 2. * (theAsioStream->framesPerHostCallback / theAsioStream->streamRepresentation.streamInfo.sampleRate) ){
        if( theAsioStream->inputChannelCount > 0 )
            theAsioStream->callbackFlags |= paInputOverflow;

        if( theAsioStream->outputChannelCount > 0 )
            theAsioStream->callbackFlags |= paOutputUnderflow;
    }
}
previousTime = paTimeInfo.currentTime;
#endif

                // note that the above input and output times do not need to be
                // adjusted for the latency of the buffer processor -- the buffer
                // processor handles that.

                if( theAsioStream->inputBufferConverter )
                {
                    for( i=0; i<theAsioStream->inputChannelCount; i++ )
                    {
                        theAsioStream->inputBufferConverter( theAsioStream->inputBufferPtrs[index][i],
                                theAsioStream->inputShift, theAsioStream->framesPerHostCallback );
                    }
                }

                PaUtil_BeginBufferProcessing( &theAsioStream->bufferProcessor, &paTimeInfo, theAsioStream->callbackFlags );

                /* reset status flags once they've been passed to the callback */
                theAsioStream->callbackFlags = 0;

                PaUtil_SetInputFrameCount( &theAsioStream->bufferProcessor, 0 /* default to host buffer size */ );
                for( i=0; i<theAsioStream->inputChannelCount; ++i )
                    PaUtil_SetNonInterleavedInputChannel( &theAsioStream->bufferProcessor, i, theAsioStream->inputBufferPtrs[index][i] );

                PaUtil_SetOutputFrameCount( &theAsioStream->bufferProcessor, 0 /* default to host buffer size */ );
                for( i=0; i<theAsioStream->outputChannelCount; ++i )
                    PaUtil_SetNonInterleavedOutputChannel( &theAsioStream->bufferProcessor, i, theAsioStream->outputBufferPtrs[index][i] );

                int callbackResult;
                if( theAsioStream->stopProcessing )
                    callbackResult = paComplete;
                else
                    callbackResult = paContinue;
                unsigned long framesProcessed = PaUtil_EndBufferProcessing( &theAsioStream->bufferProcessor, &callbackResult );

                if( theAsioStream->outputBufferConverter )
                {
                    for( i=0; i<theAsioStream->outputChannelCount; i++ )
                    {
                        theAsioStream->outputBufferConverter( theAsioStream->outputBufferPtrs[index][i],
                                theAsioStream->outputShift, theAsioStream->framesPerHostCallback );
                    }
                }

                PaUtil_EndCpuLoadMeasurement( &theAsioStream->cpuLoadMeasurer, framesProcessed );

                // Finally if the driver supports the ASIOOutputReady() optimization,
                // do it here, all data are in place
                if( theAsioStream->postOutput )
                    ASIOOutputReady();

                if( callbackResult == paContinue )
                {
                    /* nothing special to do */
                }
                else if( callbackResult == paAbort )
                {
                    /* finish playback immediately  */
                    theAsioStream->isActive = 0;
                    if( theAsioStream->streamRepresentation.streamFinishedCallback != 0 )
                        theAsioStream->streamRepresentation.streamFinishedCallback( theAsioStream->streamRepresentation.userData );
                    theAsioStream->streamFinishedCallbackCalled = true;
                    SetEvent( theAsioStream->completedBuffersPlayedEvent );
                    theAsioStream->zeroOutput = true;
                }
                else /* paComplete or other non-zero value indicating complete */
                {
                    /* Finish playback once currently queued audio has completed. */
                    theAsioStream->stopProcessing = true;

                    if( PaUtil_IsBufferProcessorOutputEmpty( &theAsioStream->bufferProcessor ) )
                    {
                        theAsioStream->zeroOutput = true;
                        theAsioStream->stopPlayoutCount = 0;
                    }
                }
            }
        }
        
        ++buffersDone;
    }while( PaAsio_AtomicDecrement(&theAsioStream->reenterCount) >= 0 );

    return 0L;
}


static void sampleRateChanged(ASIOSampleRate sRate)
{
    // TAKEN FROM THE ASIO SDK
    // do whatever you need to do if the sample rate changed
    // usually this only happens during external sync.
    // Audio processing is not stopped by the driver, actual sample rate
    // might not have even changed, maybe only the sample rate status of an
    // AES/EBU or S/PDIF digital input at the audio device.
    // You might have to update time/sample related conversion routines, etc.

    (void) sRate; /* unused parameter */
    PA_DEBUG( ("sampleRateChanged : %d \n", sRate));
}

static long asioMessages(long selector, long value, void* message, double* opt)
{
// TAKEN FROM THE ASIO SDK
    // currently the parameters "value", "message" and "opt" are not used.
    long ret = 0;

    (void) message; /* unused parameters */
    (void) opt;

    PA_DEBUG( ("asioMessages : %d , %d \n", selector, value));

    switch(selector)
    {
        case kAsioSelectorSupported:
            if(value == kAsioResetRequest
            || value == kAsioEngineVersion
            || value == kAsioResyncRequest
            || value == kAsioLatenciesChanged
            // the following three were added for ASIO 2.0, you don't necessarily have to support them
            || value == kAsioSupportsTimeInfo
            || value == kAsioSupportsTimeCode
            || value == kAsioSupportsInputMonitor)
                    ret = 1L;
            break;

        case kAsioBufferSizeChange:
            //printf("kAsioBufferSizeChange \n");
            break;

        case kAsioResetRequest:
            // defer the task and perform the reset of the driver during the next "safe" situation
            // You cannot reset the driver right now, as this code is called from the driver.
            // Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
            // Afterwards you initialize the driver again.

            /*FIXME: commented the next line out */
            //asioDriverInfo.stopped;  // In this sample the processing will just stop
            ret = 1L;
            break;

        case kAsioResyncRequest:
            // This informs the application, that the driver encountered some non fatal data loss.
            // It is used for synchronization purposes of different media.
            // Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
            // Windows Multimedia system, which could loose data because the Mutex was hold too long
            // by another thread.
            // However a driver can issue it in other situations, too.
            ret = 1L;
            break;

        case kAsioLatenciesChanged:
            // This will inform the host application that the drivers were latencies changed.
            // Beware, it this does not mean that the buffer sizes have changed!
            // You might need to update internal delay data.
            ret = 1L;
            //printf("kAsioLatenciesChanged \n");
            break;

        case kAsioEngineVersion:
            // return the supported ASIO version of the host application
            // If a host applications does not implement this selector, ASIO 1.0 is assumed
            // by the driver
            ret = 2L;
            break;

        case kAsioSupportsTimeInfo:
            // informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
            // is supported.
            // For compatibility with ASIO 1.0 drivers the host application should always support
            // the "old" bufferSwitch method, too.
            ret = 1;
            break;

        case kAsioSupportsTimeCode:
            // informs the driver wether application is interested in time code info.
            // If an application does not need to know about time code, the driver has less work
            // to do.
            ret = 0;
            break;
    }
    return ret;
}


static PaError StartStream( PaStream *s )
{
    PaError result = paNoError;
    PaAsioStream *stream = (PaAsioStream*)s;
    ASIOError asioError;

    if( stream->outputChannelCount > 0 )
    {
        ZeroOutputBuffers( stream, 0 );
        ZeroOutputBuffers( stream, 1 );
    }

    PaUtil_ResetBufferProcessor( &stream->bufferProcessor );
    stream->stopProcessing = false;
    stream->zeroOutput = false;

    /* Reentrancy counter initialisation */
    stream->reenterCount = -1;
    stream->reenterError = 0;

    stream->callbackFlags = 0;

    if( ResetEvent( stream->completedBuffersPlayedEvent ) == 0 )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_SYSTEM_ERROR( GetLastError() );
    }

    if( result == paNoError )
    {
        theAsioStream = stream;
        asioError = ASIOStart();
        if( asioError == ASE_OK )
        {
            stream->isActive = 1;
            stream->streamFinishedCallbackCalled = false;
        }
        else
        {
            theAsioStream = 0;
            result = paUnanticipatedHostError;
            PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        }
    }

    return result;
}


static PaError StopStream( PaStream *s )
{
    PaError result = paNoError;
    PaAsioStream *stream = (PaAsioStream*)s;
    ASIOError asioError;

    if( stream->isActive )
    {
        stream->stopProcessing = true;

        /* wait for the stream to finish playing out enqueued buffers.
            timeout after four times the stream latency.

            @todo should use a better time out value - if the user buffer
            length is longer than the asio buffer size then that should
            be taken into account.
        */
        if( WaitForSingleObject( theAsioStream->completedBuffersPlayedEvent,
                (DWORD)(stream->streamRepresentation.streamInfo.outputLatency * 1000. * 4.) )
                    == WAIT_TIMEOUT	 )
        {
            PA_DEBUG(("WaitForSingleObject() timed out in StopStream()\n" ));
        }
    }

    asioError = ASIOStop();
    if( asioError != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
    }

    theAsioStream = 0;
    stream->isActive = 0;

    if( !stream->streamFinishedCallbackCalled )
    {
        if( stream->streamRepresentation.streamFinishedCallback != 0 )
            stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );
    }

    return result;
}


static PaError AbortStream( PaStream *s )
{
    PaError result = paNoError;
    PaAsioStream *stream = (PaAsioStream*)s;
    ASIOError asioError;

    stream->zeroOutput = true;

    asioError = ASIOStop();
    if( asioError != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
    }
    else
    {
        // make sure that the callback is not still in-flight when ASIOStop()
        // returns. This has been observed to happen on the Hoontech DSP24 for
        // example.
        int count = 2000;  // only wait for 2 seconds, rather than hanging.
        while( theAsioStream->reenterCount != -1 && count > 0 )
        {
            Sleep(1);
            --count;
        }
    }

    /* it is questionable whether we should zero theAsioStream if ASIOStop()
        returns an error, because the callback could still be active. We assume
        not - this is based on the fact that ASIOStop is unlikely to fail
        if the callback is running - it's more likely to fail because the
        callback is not running. */
        
    theAsioStream = 0;
    stream->isActive = 0;

    if( !stream->streamFinishedCallbackCalled )
    {
        if( stream->streamRepresentation.streamFinishedCallback != 0 )
            stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );
    }

    return result;
}


static PaError IsStreamStopped( PaStream *s )
{
    //PaAsioStream *stream = (PaAsioStream*)s;
    (void) s; /* unused parameter */
    return theAsioStream == 0;
}


static PaError IsStreamActive( PaStream *s )
{
    PaAsioStream *stream = (PaAsioStream*)s;

    return stream->isActive;
}


static PaTime GetStreamTime( PaStream *s )
{
    (void) s; /* unused parameter */
    return (double)timeGetTime() * .001;
}


static double GetStreamCpuLoad( PaStream* s )
{
    PaAsioStream *stream = (PaAsioStream*)s;

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
    PaAsioStream *stream = (PaAsioStream*)s;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    (void) stream; /* unused parameters */
    (void) buffer;
    (void) frames;

    return paNoError;
}


static PaError WriteStream( PaStream* s,
                            const void *buffer,
                            unsigned long frames )
{
    PaAsioStream *stream = (PaAsioStream*)s;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    (void) stream; /* unused parameters */
    (void) buffer;
    (void) frames;

    return paNoError;
}


static signed long GetStreamReadAvailable( PaStream* s )
{
    PaAsioStream *stream = (PaAsioStream*)s;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    (void) stream; /* unused parameter */

    return 0;
}


static signed long GetStreamWriteAvailable( PaStream* s )
{
    PaAsioStream *stream = (PaAsioStream*)s;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/
    (void) stream; /* unused parameter */

    return 0;
}


PaError PaAsio_ShowControlPanel( PaDeviceIndex device, void* systemSpecific )
{
	PaError result = paNoError;
    PaUtilHostApiRepresentation *hostApi;
    PaDeviceIndex hostApiDevice;
    ASIODriverInfo asioDriverInfo;
	ASIOError asioError;
    int asioIsInitialized = 0;
    PaAsioHostApiRepresentation *asioHostApi;
    PaAsioDeviceInfo *asioDeviceInfo;


    result = PaUtil_GetHostApiRepresentation( &hostApi, paASIO );
    if( result != paNoError )
        goto error;

    result = PaUtil_DeviceIndexToHostApiDeviceIndex( &hostApiDevice, device, hostApi );
    if( result != paNoError )
        goto error;

    /*
        In theory we could proceed if the currently open device was the same
        one for which the control panel was requested, however  because the
        window pointer is not available until this function is called we
        currently need to call ASIOInit() again here, which of course can't be
        done safely while a stream is open.
    */

    asioHostApi = (PaAsioHostApiRepresentation*)hostApi;
    if( asioHostApi->openAsioDeviceIndex != paNoDevice )
    {
        result = paDeviceUnavailable;
        goto error;
    }

    asioDeviceInfo = (PaAsioDeviceInfo*)hostApi->deviceInfos[hostApiDevice];

    if( !loadAsioDriver( const_cast<char*>(asioDeviceInfo->commonDeviceInfo.name) ) )
    {
        result = paUnanticipatedHostError;
        goto error;
    }

    /* CRUCIAL!!! */
    memset( &asioDriverInfo, 0, sizeof(ASIODriverInfo) );
    asioDriverInfo.asioVersion = 2;
    asioDriverInfo.sysRef = systemSpecific;
    asioError = ASIOInit( &asioDriverInfo );
    if( asioError != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    }
    else
    {
        asioIsInitialized = 1;
    }

PA_DEBUG(("PaAsio_ShowControlPanel: ASIOInit(): %s\n", PaAsio_GetAsioErrorText(asioError) ));
PA_DEBUG(("asioVersion: ASIOInit(): %ld\n",   asioDriverInfo.asioVersion )); 
PA_DEBUG(("driverVersion: ASIOInit(): %ld\n", asioDriverInfo.driverVersion )); 
PA_DEBUG(("Name: ASIOInit(): %s\n",           asioDriverInfo.name )); 
PA_DEBUG(("ErrorMessage: ASIOInit(): %s\n",   asioDriverInfo.errorMessage )); 

    asioError = ASIOControlPanel();
    if( asioError != ASE_OK )
    {
        PA_DEBUG(("PaAsio_ShowControlPanel: ASIOControlPanel(): %s\n", PaAsio_GetAsioErrorText(asioError) ));
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        goto error;
    }

PA_DEBUG(("PaAsio_ShowControlPanel: ASIOControlPanel(): %s\n", PaAsio_GetAsioErrorText(asioError) ));

    asioError = ASIOExit();
    if( asioError != ASE_OK )
    {
        result = paUnanticipatedHostError;
        PA_ASIO_SET_LAST_ASIO_ERROR( asioError );
        asioIsInitialized = 0;
        goto error;
    }

PA_DEBUG(("PaAsio_ShowControlPanel: ASIOExit(): %s\n", PaAsio_GetAsioErrorText(asioError) ));

	return result;

error:
    if( asioIsInitialized )
        ASIOExit();

	return result;
}


PaError PaAsio_GetInputChannelName( PaDeviceIndex device, int channelIndex,
        const char** channelName )
{
    PaError result = paNoError;
    PaUtilHostApiRepresentation *hostApi;
    PaDeviceIndex hostApiDevice;
    PaAsioDeviceInfo *asioDeviceInfo;


    result = PaUtil_GetHostApiRepresentation( &hostApi, paASIO );
    if( result != paNoError )
        goto error;

    result = PaUtil_DeviceIndexToHostApiDeviceIndex( &hostApiDevice, device, hostApi );
    if( result != paNoError )
        goto error;

    asioDeviceInfo = (PaAsioDeviceInfo*)hostApi->deviceInfos[hostApiDevice];

    if( channelIndex < 0 || channelIndex >= asioDeviceInfo->commonDeviceInfo.maxInputChannels ){
        result = paInvalidChannelCount;
        goto error;
    }

    *channelName = asioDeviceInfo->asioChannelInfos[channelIndex].name;

    return paNoError;
    
error:
    return result;
}


PaError PaAsio_GetOutputChannelName( PaDeviceIndex device, int channelIndex,
        const char** channelName )
{
    PaError result = paNoError;
    PaUtilHostApiRepresentation *hostApi;
    PaDeviceIndex hostApiDevice;
    PaAsioDeviceInfo *asioDeviceInfo;


    result = PaUtil_GetHostApiRepresentation( &hostApi, paASIO );
    if( result != paNoError )
        goto error;

    result = PaUtil_DeviceIndexToHostApiDeviceIndex( &hostApiDevice, device, hostApi );
    if( result != paNoError )
        goto error;

    asioDeviceInfo = (PaAsioDeviceInfo*)hostApi->deviceInfos[hostApiDevice];

    if( channelIndex < 0 || channelIndex >= asioDeviceInfo->commonDeviceInfo.maxOutputChannels ){
        result = paInvalidChannelCount;
        goto error;
    }

    *channelName = asioDeviceInfo->asioChannelInfos[
            asioDeviceInfo->commonDeviceInfo.maxInputChannels + channelIndex].name;

    return paNoError;
    
error:
    return result;
}
