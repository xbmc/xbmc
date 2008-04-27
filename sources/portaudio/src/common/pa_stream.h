#ifndef PA_STREAM_H
#define PA_STREAM_H
/*
 * $Id: pa_stream.h 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library
 * stream interface
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
 streams.
*/


#include "portaudio.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#define PA_STREAM_MAGIC (0x18273645)


/** A structure representing an (abstract) interface to a host API. Contains
 pointers to functions which implement the interface.

 All PaStreamInterface functions are guaranteed to be called with a non-null,
 valid stream parameter.
*/
typedef struct {
    PaError (*Close)( PaStream* stream );
    PaError (*Start)( PaStream *stream );
    PaError (*Stop)( PaStream *stream );
    PaError (*Abort)( PaStream *stream );
    PaError (*IsStopped)( PaStream *stream );
    PaError (*IsActive)( PaStream *stream );
    PaTime (*GetTime)( PaStream *stream );
    double (*GetCpuLoad)( PaStream* stream );
    PaError (*Read)( PaStream* stream, void *buffer, unsigned long frames );
    PaError (*Write)( PaStream* stream, const void *buffer, unsigned long frames );
    signed long (*GetReadAvailable)( PaStream* stream );
    signed long (*GetWriteAvailable)( PaStream* stream );
} PaUtilStreamInterface;


/** Initialize the fields of a PaUtilStreamInterface structure.
*/
void PaUtil_InitializeStreamInterface( PaUtilStreamInterface *streamInterface,
    PaError (*Close)( PaStream* ),
    PaError (*Start)( PaStream* ),
    PaError (*Stop)( PaStream* ),
    PaError (*Abort)( PaStream* ),
    PaError (*IsStopped)( PaStream* ),
    PaError (*IsActive)( PaStream* ),
    PaTime (*GetTime)( PaStream* ),
    double (*GetCpuLoad)( PaStream* ),
    PaError (*Read)( PaStream* stream, void *buffer, unsigned long frames ),
    PaError (*Write)( PaStream* stream, const void *buffer, unsigned long frames ),
    signed long (*GetReadAvailable)( PaStream* stream ),
    signed long (*GetWriteAvailable)( PaStream* stream ) );


/** Dummy Read function for use in interfaces to a callback based streams.
 Pass to the Read parameter of PaUtil_InitializeStreamInterface.
 @return An error code indicating that the function has no effect
 because the stream is a callback stream.
*/
PaError PaUtil_DummyRead( PaStream* stream,
                       void *buffer,
                       unsigned long frames );


/** Dummy Write function for use in an interfaces to callback based streams.
 Pass to the Write parameter of PaUtil_InitializeStreamInterface.
 @return An error code indicating that the function has no effect
 because the stream is a callback stream.
*/
PaError PaUtil_DummyWrite( PaStream* stream,
                       const void *buffer,
                       unsigned long frames );


/** Dummy GetReadAvailable function for use in interfaces to callback based
 streams. Pass to the GetReadAvailable parameter of PaUtil_InitializeStreamInterface.
 @return An error code indicating that the function has no effect
 because the stream is a callback stream.
*/
signed long PaUtil_DummyGetReadAvailable( PaStream* stream );


/** Dummy GetWriteAvailable function for use in interfaces to callback based
 streams. Pass to the GetWriteAvailable parameter of PaUtil_InitializeStreamInterface.
 @return An error code indicating that the function has no effect
 because the stream is a callback stream.
*/
signed long PaUtil_DummyGetWriteAvailable( PaStream* stream );



/** Dummy GetCpuLoad function for use in an interface to a read/write stream.
 Pass to the GetCpuLoad parameter of PaUtil_InitializeStreamInterface.
 @return Returns 0.
*/
double PaUtil_DummyGetCpuLoad( PaStream* stream );


/** Non host specific data for a stream. This data is used by pa_front to
 forward to the appropriate functions in the streamInterface structure.
*/
typedef struct PaUtilStreamRepresentation {
    unsigned long magic;    /**< set to PA_STREAM_MAGIC */
    struct PaUtilStreamRepresentation *nextOpenStream; /**< field used by multi-api code */
    PaUtilStreamInterface *streamInterface;
    PaStreamCallback *streamCallback;
    PaStreamFinishedCallback *streamFinishedCallback;
    void *userData;
    PaStreamInfo streamInfo;
} PaUtilStreamRepresentation;


/** Initialize a PaUtilStreamRepresentation structure.

 @see PaUtil_InitializeStreamRepresentation
*/
void PaUtil_InitializeStreamRepresentation(
        PaUtilStreamRepresentation *streamRepresentation,
        PaUtilStreamInterface *streamInterface,
        PaStreamCallback *streamCallback,
        void *userData );
        

/** Clean up a PaUtilStreamRepresentation structure previously initialized
 by a call to PaUtil_InitializeStreamRepresentation.

 @see PaUtil_InitializeStreamRepresentation
*/
void PaUtil_TerminateStreamRepresentation( PaUtilStreamRepresentation *streamRepresentation );


/** Check that the stream pointer is valid.

 @return Returns paNoError if the stream pointer appears to be OK, otherwise
 returns an error indicating the cause of failure.
*/
PaError PaUtil_ValidateStreamPointer( PaStream *stream );


/** Cast an opaque stream pointer into a pointer to a PaUtilStreamRepresentation.

 @see PaUtilStreamRepresentation
*/
#define PA_STREAM_REP( stream )\
    ((PaUtilStreamRepresentation*) (stream) )


/** Cast an opaque stream pointer into a pointer to a PaUtilStreamInterface.

 @see PaUtilStreamRepresentation, PaUtilStreamInterface
*/
#define PA_STREAM_INTERFACE( stream )\
    PA_STREAM_REP( (stream) )->streamInterface


    
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_STREAM_H */
