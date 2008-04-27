#ifndef PA_PROCESS_H
#define PA_PROCESS_H
/*
 * $Id: pa_process.h 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library callback buffer processing adapters
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Phil Burk, Ross Bencina
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

 @brief Buffer Processor prototypes. A Buffer Processor performs buffer length
 adaption, coordinates sample format conversion, and interleaves/deinterleaves
 channels.

 <h3>Overview</h3>

 The "Buffer Processor" (PaUtilBufferProcessor) manages conversion of audio
 data from host buffers to user buffers and back again. Where required, the
 buffer processor takes care of converting between host and user sample formats,
 interleaving and deinterleaving multichannel buffers, and adapting between host
 and user buffers with different lengths. The buffer processor may be used with
 full and half duplex streams, for both callback streams and blocking read/write
 streams.

 One of the important capabilities provided by the buffer processor is
 the ability to adapt between user and host buffer sizes of different lengths
 with minimum latency. Although this task is relatively easy to perform when
 the host buffer size is an integer multiple of the user buffer size, the
 problem is more complicated when this is not the case - especially for
 full-duplex callback streams. Where necessary the adaption is implemented by
 internally buffering some input and/or output data. The buffer adation
 algorithm used by the buffer processor was originally implemented by
 Stephan Letz for the ASIO version of PortAudio, and is described in his
 Callback_adaption_.pdf which is included in the distribution.

 The buffer processor performs sample conversion using the functions provided
 by pa_converters.c.

 The following sections provide an overview of how to use the buffer processor.
 Interested readers are advised to consult the host API implementations for
 examples of buffer processor usage.
 

 <h4>Initialization, resetting and termination</h4>

 When a stream is opened, the buffer processor should be initialized using
 PaUtil_InitializeBufferProcessor. This function initializes internal state
 and allocates temporary buffers as neccesary according to the supplied
 configuration parameters. Some of the parameters correspond to those requested
 by the user in their call to Pa_OpenStream(), others reflect the requirements
 of the host API implementation - they indicate host buffer sizes, formats,
 and the type of buffering which the Host API uses. The buffer processor should
 be initialized for callback streams and blocking read/write streams.

 Call PaUtil_ResetBufferProcessor to clear any sample data which is present
 in the buffer processor before starting to use it (for example when
 Pa_StartStream is called).

 When the buffer processor is no longer used call
 PaUtil_TerminateBufferProcessor.

 
 <h4>Using the buffer processor for a callback stream</h4>

 The buffer processor's role in a callback stream is to take host input buffers
 process them with the stream callback, and fill host output buffers. For a
 full duplex stream, the buffer processor handles input and output simultaneously
 due to the requirements of the minimum-latency buffer adation algorithm.

 When a host buffer becomes available, the implementation should call
 the buffer processor to process the buffer. The buffer processor calls the
 stream callback to consume and/or produce audio data as necessary. The buffer
 processor will convert sample formats, interleave/deinterleave channels,
 and slice or chunk the data to the appropriate buffer lengths according to
 the requirements of the stream callback and the host API.

 To process a host buffer (or a pair of host buffers for a full-duplex stream)
 use the following calling sequence:

 -# Call PaUtil_BeginBufferProcessing
 -# For a stream which takes input:
    - Call PaUtil_SetInputFrameCount with the number of frames in the host input
        buffer.
    - Call one of the following functions one or more times to tell the
        buffer processor about the host input buffer(s): PaUtil_SetInputChannel,
        PaUtil_SetInterleavedInputChannels, PaUtil_SetNonInterleavedInputChannel.
        Which function you call will depend on whether the host buffer(s) are
        interleaved or not.
    - If the available host data is split accross two buffers (for example a
        data range at the end of a circular buffer and another range at the
        beginning of the circular buffer), also call
        PaUtil_Set2ndInputFrameCount, PaUtil_Set2ndInputChannel,
        PaUtil_Set2ndInterleavedInputChannels,
        PaUtil_Set2ndNonInterleavedInputChannel as necessary to tell the buffer
        processor about the second buffer.
 -# For a stream which generates output:
    - Call PaUtil_SetOutputFrameCount with the number of frames in the host
        output buffer.
    - Call one of the following functions one or more times to tell the
        buffer processor about the host output buffer(s): PaUtil_SetOutputChannel,
        PaUtil_SetInterleavedOutputChannels, PaUtil_SetNonInterleavedOutputChannel.
        Which function you call will depend on whether the host buffer(s) are
        interleaved or not.
    - If the available host output buffer space is split accross two buffers
        (for example a data range at the end of a circular buffer and another
        range at the beginning of the circular buffer), call
        PaUtil_Set2ndOutputFrameCount, PaUtil_Set2ndOutputChannel,
        PaUtil_Set2ndInterleavedOutputChannels,
        PaUtil_Set2ndNonInterleavedOutputChannel as necessary to tell the buffer
        processor about the second buffer.
 -# Call PaUtil_EndBufferProcessing, this function performs the actual data
    conversion and processing.


 <h4>Using the buffer processor for a blocking read/write stream</h4>

 Blocking read/write streams use the buffer processor to convert and copy user
 output data to a host buffer, and to convert and copy host input data to
 the user's buffer. The buffer processor does not perform any buffer adaption.
 When using the buffer processor in a blocking read/write stream the input and
 output conversion are performed separately by the PaUtil_CopyInput and
 PaUtil_CopyOutput functions.

 To copy data from a host input buffer to the buffer(s) which the user supplies
 to Pa_ReadStream, use the following calling sequence.

 - Repeat the following three steps until the user buffer(s) have been filled
    with samples from the host input buffers:
     -# Call PaUtil_SetInputFrameCount with the number of frames in the host
        input buffer.
     -# Call one of the following functions one or more times to tell the
        buffer processor about the host input buffer(s): PaUtil_SetInputChannel,
        PaUtil_SetInterleavedInputChannels, PaUtil_SetNonInterleavedInputChannel.
        Which function you call will depend on whether the host buffer(s) are
        interleaved or not.
     -# Call PaUtil_CopyInput with the user buffer pointer (or a copy of the
        array of buffer pointers for a non-interleaved stream) passed to
        Pa_ReadStream, along with the number of frames in the user buffer(s).
        Be careful to pass a <i>copy</i> of the user buffer pointers to
        PaUtil_CopyInput because PaUtil_CopyInput advances the pointers to
        the start of the next region to copy.
 - PaUtil_CopyInput will not copy more data than is available in the
    host buffer(s), so the above steps need to be repeated until the user
    buffer(s) are full.

 
 To copy data to the host output buffer from the user buffers(s) supplied
 to Pa_WriteStream use the following calling sequence.

 - Repeat the following three steps until all frames from the user buffer(s)
    have been copied to the host API:
     -# Call PaUtil_SetOutputFrameCount with the number of frames in the host
        output buffer.
     -# Call one of the following functions one or more times to tell the
        buffer processor about the host output buffer(s): PaUtil_SetOutputChannel,
        PaUtil_SetInterleavedOutputChannels, PaUtil_SetNonInterleavedOutputChannel.
        Which function you call will depend on whether the host buffer(s) are
        interleaved or not.
     -# Call PaUtil_CopyOutput with the user buffer pointer (or a copy of the
        array of buffer pointers for a non-interleaved stream) passed to
        Pa_WriteStream, along with the number of frames in the user buffer(s).
        Be careful to pass a <i>copy</i> of the user buffer pointers to 
        PaUtil_CopyOutput because PaUtil_CopyOutput advances the pointers to
        the start of the next region to copy.
 - PaUtil_CopyOutput will not copy more data than fits in the host buffer(s),
    so the above steps need to be repeated until all user data is copied.
*/


#include "portaudio.h"
#include "pa_converters.h"
#include "pa_dither.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** @brief Mode flag passed to PaUtil_InitializeBufferProcessor indicating the type
 of buffering that the host API uses.

 The mode used depends on whether the host API or the implementation manages
 the buffers, and how these buffers are used (scatter gather, circular buffer).
*/
typedef enum {
/** The host buffer size is a fixed known size. */
    paUtilFixedHostBufferSize,

/** The host buffer size may vary, but has a known maximum size. */
    paUtilBoundedHostBufferSize,

/** Nothing is known about the host buffer size. */
    paUtilUnknownHostBufferSize,

/** The host buffer size varies, and the client does not require the buffer
 processor to consume all of the input and fill all of the output buffer. This
 is useful when the implementation has access to the host API's circular buffer
 and only needs to consume/fill some of it, not necessarily all of it, with each
 call to the buffer processor. This is the only mode where
 PaUtil_EndBufferProcessing() may not consume the whole buffer.
*/
    paUtilVariableHostBufferSizePartialUsageAllowed
}PaUtilHostBufferSizeMode;


/** @brief An auxilliary data structure used internally by the buffer processor
 to represent host input and output buffers. */
typedef struct PaUtilChannelDescriptor{
    void *data;
    unsigned int stride;  /**< stride in samples, not bytes */
}PaUtilChannelDescriptor;


/** @brief The main buffer processor data structure.

 Allocate one of these, initialize it with PaUtil_InitializeBufferProcessor
 and terminate it with PaUtil_TerminateBufferProcessor.
*/
typedef struct {
    unsigned long framesPerUserBuffer;
    unsigned long framesPerHostBuffer;

    PaUtilHostBufferSizeMode hostBufferSizeMode;
    int useNonAdaptingProcess;
    unsigned long framesPerTempBuffer;

    unsigned int inputChannelCount;
    unsigned int bytesPerHostInputSample;
    unsigned int bytesPerUserInputSample;
    int userInputIsInterleaved;
    PaUtilConverter *inputConverter;
    PaUtilZeroer *inputZeroer;
    
    unsigned int outputChannelCount;
    unsigned int bytesPerHostOutputSample;
    unsigned int bytesPerUserOutputSample;
    int userOutputIsInterleaved;
    PaUtilConverter *outputConverter;
    PaUtilZeroer *outputZeroer;

    unsigned long initialFramesInTempInputBuffer;
    unsigned long initialFramesInTempOutputBuffer;

    void *tempInputBuffer;          /**< used for slips, block adaption, and conversion. */
    void **tempInputBufferPtrs;     /**< storage for non-interleaved buffer pointers, NULL for interleaved user input */
    unsigned long framesInTempInputBuffer; /**< frames remaining in input buffer from previous adaption iteration */

    void *tempOutputBuffer;         /**< used for slips, block adaption, and conversion. */
    void **tempOutputBufferPtrs;    /**< storage for non-interleaved buffer pointers, NULL for interleaved user output */
    unsigned long framesInTempOutputBuffer; /**< frames remaining in input buffer from previous adaption iteration */

    PaStreamCallbackTimeInfo *timeInfo;

    PaStreamCallbackFlags callbackStatusFlags;

    unsigned long hostInputFrameCount[2];
    PaUtilChannelDescriptor *hostInputChannels[2]; /**< pointers to arrays of channel descriptors.
                                                        pointers are NULL for half-duplex output processing.
                                                        hostInputChannels[i].data is NULL when the caller
                                                        calls PaUtil_SetNoInput()
                                                        */
    unsigned long hostOutputFrameCount[2];
    PaUtilChannelDescriptor *hostOutputChannels[2]; /**< pointers to arrays of channel descriptors.
                                                         pointers are NULL for half-duplex input processing.
                                                         hostOutputChannels[i].data is NULL when the caller
                                                         calls PaUtil_SetNoOutput()
                                                         */

    PaUtilTriangularDitherGenerator ditherGenerator;

    double samplePeriod;

    PaStreamCallback *streamCallback;
    void *userData;
} PaUtilBufferProcessor;


/** @name Initialization, termination, resetting and info */
/*@{*/

/** Initialize a buffer processor's representation stored in a
 PaUtilBufferProcessor structure. Be sure to call
 PaUtil_TerminateBufferProcessor after finishing with a buffer processor.

 @param bufferProcessor The buffer processor structure to initialize.

 @param inputChannelCount The number of input channels as passed to
 Pa_OpenStream or 0 for an output-only stream.

 @param userInputSampleFormat Format of user input samples, as passed to
 Pa_OpenStream. This parameter is ignored for ouput-only streams.
 
 @param hostInputSampleFormat Format of host input samples. This parameter is
 ignored for output-only streams. See note about host buffer interleave below.

 @param outputChannelCount The number of output channels as passed to
 Pa_OpenStream or 0 for an input-only stream.

 @param userOutputSampleFormat Format of user output samples, as passed to
 Pa_OpenStream. This parameter is ignored for input-only streams.
 
 @param hostOutputSampleFormat Format of host output samples. This parameter is
 ignored for input-only streams. See note about host buffer interleave below.

 @param sampleRate Sample rate of the stream. The more accurate this is the
 better - it is used for updating time stamps when adapting buffers.
 
 @param streamFlags Stream flags as passed to Pa_OpenStream, this parameter is
 used for selecting special sample conversion options such as clipping and
 dithering.
 
 @param framesPerUserBuffer Number of frames per user buffer, as requested
 by the framesPerBuffer parameter to Pa_OpenStream. This parameter may be
 zero to indicate that the user will accept any (and varying) buffer sizes.

 @param framesPerHostBuffer Specifies the number of frames per host buffer
 for the fixed buffer size mode, and the maximum number of frames
 per host buffer for the bounded host buffer size mode. It is ignored for
 the other modes.

 @param hostBufferSizeMode A mode flag indicating the size variability of
 host buffers that will be passed to the buffer processor. See
 PaUtilHostBufferSizeMode for further details.
 
 @param streamCallback The user stream callback passed to Pa_OpenStream.

 @param userData The user data field passed to Pa_OpenStream.
    
 @note The interleave flag is ignored for host buffer formats. Host
 interleave is determined by the use of different SetInput and SetOutput
 functions.

 @return An error code indicating whether the initialization was successful.
 If the error code is not PaNoError, the buffer processor was not initialized
 and should not be used.
 
 @see Pa_OpenStream, PaUtilHostBufferSizeMode, PaUtil_TerminateBufferProcessor
*/
PaError PaUtil_InitializeBufferProcessor( PaUtilBufferProcessor* bufferProcessor,
            int inputChannelCount, PaSampleFormat userInputSampleFormat,
            PaSampleFormat hostInputSampleFormat,
            int outputChannelCount, PaSampleFormat userOutputSampleFormat,
            PaSampleFormat hostOutputSampleFormat,
            double sampleRate,
            PaStreamFlags streamFlags,
            unsigned long framesPerUserBuffer, /* 0 indicates don't care */
            unsigned long framesPerHostBuffer,
            PaUtilHostBufferSizeMode hostBufferSizeMode,
            PaStreamCallback *streamCallback, void *userData );


/** Terminate a buffer processor's representation. Deallocates any temporary
 buffers allocated by PaUtil_InitializeBufferProcessor.
 
 @param bufferProcessor The buffer processor structure to terminate.

 @see PaUtil_InitializeBufferProcessor.
*/
void PaUtil_TerminateBufferProcessor( PaUtilBufferProcessor* bufferProcessor );


/** Clear any internally buffered data. If you call
 PaUtil_InitializeBufferProcessor in your OpenStream routine, make sure you
 call PaUtil_ResetBufferProcessor in your StartStream call.

 @param bufferProcessor The buffer processor to reset.
*/
void PaUtil_ResetBufferProcessor( PaUtilBufferProcessor* bufferProcessor );


/** Retrieve the input latency of a buffer processor.

 @param bufferProcessor The buffer processor examine.

 @return The input latency introduced by the buffer processor, in frames.

 @see PaUtil_GetBufferProcessorOutputLatency
*/
unsigned long PaUtil_GetBufferProcessorInputLatency( PaUtilBufferProcessor* bufferProcessor );

/** Retrieve the output latency of a buffer processor.

 @param bufferProcessor The buffer processor examine.

 @return The output latency introduced by the buffer processor, in frames.

 @see PaUtil_GetBufferProcessorInputLatency
*/
unsigned long PaUtil_GetBufferProcessorOutputLatency( PaUtilBufferProcessor* bufferProcessor );

/*@}*/


/** @name Host buffer pointer configuration

 Functions to set host input and output buffers, used by both callback streams
 and blocking read/write streams.
*/
/*@{*/ 


/** Set the number of frames in the input host buffer(s) specified by the
 PaUtil_Set*InputChannel functions.

 @param bufferProcessor The buffer processor.

 @param frameCount The number of host input frames. A 0 frameCount indicates to
 use the framesPerHostBuffer value passed to PaUtil_InitializeBufferProcessor.

 @see PaUtil_SetNoInput, PaUtil_SetInputChannel,
 PaUtil_SetInterleavedInputChannels, PaUtil_SetNonInterleavedInputChannel
*/
void PaUtil_SetInputFrameCount( PaUtilBufferProcessor* bufferProcessor,
        unsigned long frameCount );

        
/** Indicate that no input is avalable. This function should be used when
 priming the output of a full-duplex stream opened with the
 paPrimeOutputBuffersUsingStreamCallback flag. Note that it is not necessary
 to call this or any othe PaUtil_Set*Input* functions for ouput-only streams.

 @param bufferProcessor The buffer processor.
*/
void PaUtil_SetNoInput( PaUtilBufferProcessor* bufferProcessor );


/** Provide the buffer processor with a pointer to a host input channel.

 @param bufferProcessor The buffer processor.
 @param channel The channel number.
 @param data The buffer.
 @param stride The stride from one sample to the next, in samples. For
 interleaved host buffers, the stride will usually be the same as the number of
 channels in the buffer.
*/
void PaUtil_SetInputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data, unsigned int stride );


/** Provide the buffer processor with a pointer to an number of interleaved
 host input channels.

 @param bufferProcessor The buffer processor.
 @param firstChannel The first channel number.
 @param data The buffer.
 @param channelCount The number of interleaved channels in the buffer. If
 channelCount is zero, the number of channels specified to
 PaUtil_InitializeBufferProcessor will be used.
*/
void PaUtil_SetInterleavedInputChannels( PaUtilBufferProcessor* bufferProcessor,
        unsigned int firstChannel, void *data, unsigned int channelCount );


/** Provide the buffer processor with a pointer to one non-interleaved host
 output channel.

 @param bufferProcessor The buffer processor.
 @param channel The channel number.
 @param data The buffer.
*/
void PaUtil_SetNonInterleavedInputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data );


/** Use for the second buffer half when the input buffer is split in two halves.
 @see PaUtil_SetInputFrameCount
*/
void PaUtil_Set2ndInputFrameCount( PaUtilBufferProcessor* bufferProcessor,
        unsigned long frameCount );

/** Use for the second buffer half when the input buffer is split in two halves.
 @see PaUtil_SetInputChannel
*/
void PaUtil_Set2ndInputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data, unsigned int stride );

/** Use for the second buffer half when the input buffer is split in two halves.
 @see PaUtil_SetInterleavedInputChannels
*/
void PaUtil_Set2ndInterleavedInputChannels( PaUtilBufferProcessor* bufferProcessor,
        unsigned int firstChannel, void *data, unsigned int channelCount );

/** Use for the second buffer half when the input buffer is split in two halves.
 @see PaUtil_SetNonInterleavedInputChannel
*/
void PaUtil_Set2ndNonInterleavedInputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data );

        
/** Set the number of frames in the output host buffer(s) specified by the
 PaUtil_Set*OutputChannel functions.

 @param bufferProcessor The buffer processor.

 @param frameCount The number of host output frames. A 0 frameCount indicates to
 use the framesPerHostBuffer value passed to PaUtil_InitializeBufferProcessor.

 @see PaUtil_SetOutputChannel, PaUtil_SetInterleavedOutputChannels,
 PaUtil_SetNonInterleavedOutputChannel
*/
void PaUtil_SetOutputFrameCount( PaUtilBufferProcessor* bufferProcessor,
        unsigned long frameCount );


/** Indicate that the output will be discarded. This function should be used
 when implementing the paNeverDropInput mode for full duplex streams.

 @param bufferProcessor The buffer processor.
*/
void PaUtil_SetNoOutput( PaUtilBufferProcessor* bufferProcessor );


/** Provide the buffer processor with a pointer to a host output channel.

 @param bufferProcessor The buffer processor.
 @param channel The channel number.
 @param data The buffer.
 @param stride The stride from one sample to the next, in samples. For
 interleaved host buffers, the stride will usually be the same as the number of
 channels in the buffer.
*/
void PaUtil_SetOutputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data, unsigned int stride );


/** Provide the buffer processor with a pointer to a number of interleaved
 host output channels.

 @param bufferProcessor The buffer processor.
 @param firstChannel The first channel number.
 @param data The buffer.
 @param channelCount The number of interleaved channels in the buffer. If
 channelCount is zero, the number of channels specified to
 PaUtil_InitializeBufferProcessor will be used.
*/
void PaUtil_SetInterleavedOutputChannels( PaUtilBufferProcessor* bufferProcessor,
        unsigned int firstChannel, void *data, unsigned int channelCount );

        
/** Provide the buffer processor with a pointer to one non-interleaved host
 output channel.

 @param bufferProcessor The buffer processor.
 @param channel The channel number.
 @param data The buffer.
*/
void PaUtil_SetNonInterleavedOutputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data );


/** Use for the second buffer half when the output buffer is split in two halves.
 @see PaUtil_SetOutputFrameCount
*/
void PaUtil_Set2ndOutputFrameCount( PaUtilBufferProcessor* bufferProcessor,
        unsigned long frameCount );

/** Use for the second buffer half when the output buffer is split in two halves.
 @see PaUtil_SetOutputChannel
*/
void PaUtil_Set2ndOutputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data, unsigned int stride );

/** Use for the second buffer half when the output buffer is split in two halves.
 @see PaUtil_SetInterleavedOutputChannels
*/
void PaUtil_Set2ndInterleavedOutputChannels( PaUtilBufferProcessor* bufferProcessor,
        unsigned int firstChannel, void *data, unsigned int channelCount );

/** Use for the second buffer half when the output buffer is split in two halves.
 @see PaUtil_SetNonInterleavedOutputChannel
*/
void PaUtil_Set2ndNonInterleavedOutputChannel( PaUtilBufferProcessor* bufferProcessor,
        unsigned int channel, void *data );

/*@}*/


/** @name Buffer processing functions for callback streams
*/
/*@{*/

/** Commence processing a host buffer (or a pair of host buffers in the
 full-duplex case) for a callback stream.

 @param bufferProcessor The buffer processor.

 @param timeInfo Timing information for the first sample of the host
 buffer(s). This information may be adjusted when buffer adaption is being
 performed.

 @param callbackStatusFlags Flags indicating whether underruns and overruns
 have occurred since the last time the buffer processor was called.
*/
void PaUtil_BeginBufferProcessing( PaUtilBufferProcessor* bufferProcessor,
        PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags callbackStatusFlags );

        
/** Finish processing a host buffer (or a pair of host buffers in the
 full-duplex case) for a callback stream.

 @param bufferProcessor The buffer processor.
 
 @param callbackResult On input, indicates a previous callback result, and on
 exit, the result of the user stream callback, if it is called.
 On entry callbackResult should contain one of { paContinue, paComplete, or
 paAbort}. If paComplete is passed, the stream callback will not be called
 but any audio that was generated by previous stream callbacks will be copied
 to the output buffer(s). You can check whether the buffer processor's internal
 buffer is empty by calling PaUtil_IsBufferProcessorOutputEmpty.

 If the stream callback is called its result is stored in *callbackResult. If
 the stream callback returns paComplete or paAbort, all output buffers will be
 full of valid data - some of which may be zeros to acount for data that
 wasn't generated by the terminating callback.

 @return The number of frames processed. This usually corresponds to the
 number of frames specified by the PaUtil_Set*FrameCount functions, exept in
 the paUtilVariableHostBufferSizePartialUsageAllowed buffer size mode when a
 smaller value may be returned.
*/
unsigned long PaUtil_EndBufferProcessing( PaUtilBufferProcessor* bufferProcessor,
        int *callbackResult );


/** Determine whether any callback generated output remains in the bufffer
 processor's internal buffers. This method may be used to determine when to
 continue calling PaUtil_EndBufferProcessing() after the callback has returned
 a callbackResult of paComplete.

 @param bufferProcessor The buffer processor.
 
 @return Returns non-zero when callback generated output remains in the internal
 buffer and zero (0) when there internal buffer contains no callback generated
 data.
*/
int PaUtil_IsBufferProcessorOutputEmpty( PaUtilBufferProcessor* bufferProcessor );

/*@}*/


/** @name Buffer processing functions for blocking read/write streams
*/
/*@{*/

/** Copy samples from host input channels set up by the PaUtil_Set*InputChannels
 functions to a user supplied buffer. This function is intended for use with
 blocking read/write streams. Copies the minimum of the number of
 user frames (specified by the frameCount parameter) and the number of available
 host frames (specified in a previous call to SetInputFrameCount()).

 @param bufferProcessor The buffer processor.

 @param buffer A pointer to the user buffer pointer, or a pointer to a pointer
 to an array of user buffer pointers for a non-interleaved stream. It is
 important that this parameter points to a copy of the user buffer pointers,
 not to the actual user buffer pointers, because this function updates the
 pointers before returning.

 @param frameCount The number of frames of data in the buffer(s) pointed to by
 the buffer parameter.

 @return The number of frames copied. The buffer pointer(s) pointed to by the
 buffer parameter are advanced to point to the frame(s) following the last one
 filled.
*/
unsigned long PaUtil_CopyInput( PaUtilBufferProcessor* bufferProcessor,
        void **buffer, unsigned long frameCount );


/* Copy samples from a user supplied buffer to host output channels set up by
 the PaUtil_Set*OutputChannels functions. This function is intended for use with
 blocking read/write streams. Copies the minimum of the number of
 user frames (specified by the frameCount parameter) and the number of
 host frames (specified in a previous call to SetOutputFrameCount()).

 @param bufferProcessor The buffer processor.

 @param buffer A pointer to the user buffer pointer, or a pointer to a pointer
 to an array of user buffer pointers for a non-interleaved stream. It is
 important that this parameter points to a copy of the user buffer pointers,
 not to the actual user buffer pointers, because this function updates the
 pointers before returning.

 @param frameCount The number of frames of data in the buffer(s) pointed to by
 the buffer parameter.

 @return The number of frames copied. The buffer pointer(s) pointed to by the
 buffer parameter are advanced to point to the frame(s) following the last one
 copied.
*/
unsigned long PaUtil_CopyOutput( PaUtilBufferProcessor* bufferProcessor,
        const void ** buffer, unsigned long frameCount );


/* Zero samples in host output channels set up by the PaUtil_Set*OutputChannels
 functions. This function is useful for flushing streams.
 Zeros the minimum of frameCount and the number of host frames specified in a
 previous call to SetOutputFrameCount().

 @param bufferProcessor The buffer processor.

 @param frameCount The maximum number of frames to zero.
 
 @return The number of frames zeroed.
*/
unsigned long PaUtil_ZeroOutput( PaUtilBufferProcessor* bufferProcessor,
        unsigned long frameCount );


/*@}*/


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_PROCESS_H */
