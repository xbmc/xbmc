#ifndef PA_UTIL_H
#define PA_UTIL_H
/*
 * $Id: pa_util.h 1229 2007-06-15 16:11:11Z rossb $
 * Portable Audio I/O Library implementation utilities header
 * common implementation utilities and interfaces
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

    @brief Prototypes for utility functions used by PortAudio implementations.

    @todo Document and adhere to the alignment guarantees provided by
    PaUtil_AllocateMemory().
*/


#include "portaudio.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


struct PaUtilHostApiRepresentation;


/** Retrieve a specific host API representation. This function can be used
 by implementations to retrieve a pointer to their representation in
 host api specific extension functions which aren't passed a rep pointer
 by pa_front.c.

 @param hostApi A pointer to a host API represenation pointer. Apon success
 this will receive the requested representation pointer.

 @param type A valid host API type identifier.

 @returns An error code. If the result is PaNoError then a pointer to the
 requested host API representation will be stored in *hostApi. If the host API
 specified by type is not found, this function returns paHostApiNotFound.
*/
PaError PaUtil_GetHostApiRepresentation( struct PaUtilHostApiRepresentation **hostApi,
        PaHostApiTypeId type );


/** Convert a PortAudio device index into a host API specific device index.
 @param hostApiDevice Pointer to a device index, on success this will recieve the
 converted device index value.
 @param device The PortAudio device index to convert.
 @param hostApi The host api which the index should be converted for.

 @returns On success returns PaNoError and places the converted index in the
 hostApiDevice parameter.
*/
PaError PaUtil_DeviceIndexToHostApiDeviceIndex(
        PaDeviceIndex *hostApiDevice, PaDeviceIndex device,
        struct PaUtilHostApiRepresentation *hostApi );


/** Set the host error information returned by Pa_GetLastHostErrorInfo. This
 function and the paUnanticipatedHostError error code should be used as a
 last resort.  Implementors should use existing PA error codes where possible,
 or nominate new ones. Note that at it is always better to use
 PaUtil_SetLastHostErrorInfo() and paUnanticipatedHostError than to return an
 ambiguous or inaccurate PaError code.

 @param hostApiType  The host API which encountered the error (ie of the caller)

 @param errorCode The error code returned by the native API function.

 @param errorText A string describing the error. PaUtil_SetLastHostErrorInfo
 makes a copy of the string, so it is not necessary for the pointer to remain
 valid after the call to PaUtil_SetLastHostErrorInfo() returns.

*/
void PaUtil_SetLastHostErrorInfo( PaHostApiTypeId hostApiType, long errorCode,
        const char *errorText );


        
/* the following functions are implemented in a platform platform specific
 .c file
*/

/** Allocate size bytes, guaranteed to be aligned to a FIXME byte boundary */
void *PaUtil_AllocateMemory( long size );


/** Realease block if non-NULL. block may be NULL */
void PaUtil_FreeMemory( void *block );


/** Return the number of currently allocated blocks. This function can be
 used for detecting memory leaks.

 @note Allocations will only be tracked if PA_TRACK_MEMORY is #defined. If
 it isn't, this function will always return 0.
*/
int PaUtil_CountCurrentlyAllocatedBlocks( void );


/** Initialize the clock used by PaUtil_GetTime(). Call this before calling
 PaUtil_GetTime.

 @see PaUtil_GetTime
*/
void PaUtil_InitializeClock( void );


/** Return the system time in seconds. Used to implement CPU load functions

 @see PaUtil_InitializeClock
*/
double PaUtil_GetTime( void );


/* void Pa_Sleep( long msec );  must also be implemented in per-platform .c file */



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_UTIL_H */
