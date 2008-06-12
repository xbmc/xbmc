/*
 * $Id: pa_process.c 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library
 * streamCallback <-> host buffer processing adapter
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

 @brief Buffer Processor implementation.
    
 The code in this file is not optimised yet - although it's not clear that
 it needs to be. there may appear to be redundancies
 that could be factored into common functions, but the redundanceis are left
 intentionally as each appearance may have different optimisation possibilities.

 The optimisations which are planned involve only converting data in-place
 where possible, rather than copying to the temp buffer(s).

 Note that in the extreme case of being able to convert in-place, and there
 being no conversion necessary there should be some code which short-circuits
 the operation.

    @todo Consider cache tilings for intereave<->deinterleave.

    @todo implement timeInfo->currentTime int PaUtil_BeginBufferProcessing()

    @todo specify and implement some kind of logical policy for handling the
        underflow and overflow stream flags when the underflow/overflow overlaps
        multiple user buffers/callbacks.

	@todo provide support for priming the buffers with data from the callback.
        The client interface is now implemented through PaUtil_SetNoInput()
        which sets bp->hostInputChannels[0][0].data to zero. However this is
        currently only implemented in NonAdaptingProcess(). It shouldn't be
        needed for AdaptingInputOnlyProcess() (no priming should ever be
        requested for AdaptingInputOnlyProcess()).
        Not sure if additional work should be required to make it work with
        AdaptingOutputOnlyProcess, but it definitely is required for
        AdaptingProcess.

    @todo implement PaUtil_SetNoOutput for AdaptingProcess

    @todo don't allocate temp buffers for blocking streams unless they are
        needed. At the moment they are needed, but perhaps for host APIs
        where the implementation passes a buffer to the host they could be
        used.
*/


#include <assert.h>
#include <string.h> /* memset() */

#include "pa_process.h"
#include "pa_util.h"


#define PA_FRAMES_PER_TEMP_BUFFER_WHEN_HOST_BUFFER_SIZE_IS_UNKNOWN_    1024

#define PA_MIN_( a, b ) ( ((a)<(b)) ? (a) : (b) )


/* greatest common divisor - PGCD in French */
static unsigned long GCD( unsigned long a, unsigned long b )
{
    return (b==0) ? a : GCD( b, a%b);
}

/* least common multiple - PPCM in French */
static unsigned long LCM( unsigned long a, unsigned long b )
{
    return (a*b) / GCD(a,b);
}

#define PA_MAX_( a, b ) (((a) > (b)) ? (a) : (b))

static unsigned long CalculateFrameShift( unsigned long M, unsigned long N )
{
    unsigned long result = 0;
    unsigned long i;
    unsigned long lcm;

    assert( M > 0 );
    assert( N > 0 );

    lcm = LCM( M, N );
    for( i = M; i < lcm; i += M )
        result = PA_MAX_( result, i % N );

    return result;
}


PaError PaUtil_InitializeBufferProcessor( PaUtilBufferProcessor* bp,
        int inputChannelCount, PaSampleFormat userInputSampleFormat,
        PaSampleFormat hostInputSampleFormat,
        int outputChannelCount, PaSampleFormat userOutputSampleFormat,
        PaSampleFormat hostOutputSampleFormat,
        double sampleRate,
        PaStreamFlags streamFlags,
        unsigned long framesPerUserBuffer,
        unsigned long framesPerHostBuffer,
        PaUtilHostBufferSizeMode hostBufferSizeMode,
        PaStreamCallback *streamCallback, void *userData )
{
    PaError result = paNoError;
    PaError bytesPerSample;
    unsigned long tempInputBufferSize, tempOutputBufferSize;

    if( streamFlags & paNeverDropInput )
    {
        /* paNeverDropInput is only valid for full-duplex callback streams, with an unspecified number of frames per buffer. */
        if( !streamCallback || !(inputChannelCount > 0 && outputChannelCount > 0) ||
                framesPerUserBuffer != paFramesPerBufferUnspecified )
            return paInvalidFlag;
    }

    /* initialize buffer ptrs to zero so they can be freed if necessary in error */
    bp->tempInputBuffer = 0;
    bp->tempInputBufferPtrs = 0;
    bp->tempOutputBuffer = 0;
    bp->tempOutputBufferPtrs = 0;

    bp->framesPerUserBuffer = framesPerUserBuffer;
    bp->framesPerHostBuffer = framesPerHostBuffer;

    bp->inputChannelCount = inputChannelCount;
    bp->outputChannelCount = outputChannelCount;

    bp->hostBufferSizeMode = hostBufferSizeMode;

    bp->hostInputChannels[0] = bp->hostInputChannels[1] = 0;
    bp->hostOutputChannels[0] = bp->hostOutputChannels[1] = 0;

    if( framesPerUserBuffer == 0 ) /* streamCallback will accept any buffer size */
    {
        bp->useNonAdaptingProcess = 1;
        bp->initialFramesInTempInputBuffer = 0;
        bp->initialFramesInTempOutputBuffer = 0;

        if( hostBufferSizeMode == paUtilFixedHostBufferSize
                || hostBufferSizeMode == paUtilBoundedHostBufferSize )
        {
            bp->framesPerTempBuffer = framesPerHostBuffer;
        }
        else /* unknown host buffer size */
        {
             bp->framesPerTempBuffer = PA_FRAMES_PER_TEMP_BUFFER_WHEN_HOST_BUFFER_SIZE_IS_UNKNOWN_;
        }
    }
    else
    {
        bp->framesPerTempBuffer = framesPerUserBuffer;

        if( hostBufferSizeMode == paUtilFixedHostBufferSize
                && framesPerHostBuffer % framesPerUserBuffer == 0 )
        {
            bp->useNonAdaptingProcess = 1;
            bp->initialFramesInTempInputBuffer = 0;
            bp->initialFramesInTempOutputBuffer = 0;
        }
        else
        {
            bp->useNonAdaptingProcess = 0;

            if( inputChannelCount > 0 && outputChannelCount > 0 )
            {
                /* full duplex */
                if( hostBufferSizeMode == paUtilFixedHostBufferSize )
                {
                    unsigned long frameShift =
                        CalculateFrameShift( framesPerHostBuffer, framesPerUserBuffer );

                    if( framesPerUserBuffer > framesPerHostBuffer )
                    {
                        bp->initialFramesInTempInputBuffer = frameShift;
                        bp->initialFramesInTempOutputBuffer = 0;
                    }
                    else
                    {
                        bp->initialFramesInTempInputBuffer = 0;
                        bp->initialFramesInTempOutputBuffer = frameShift;
                    }
                }
                else /* variable host buffer size, add framesPerUserBuffer latency */
                {
                    bp->initialFramesInTempInputBuffer = 0;
                    bp->initialFramesInTempOutputBuffer = framesPerUserBuffer;
                }
            }
            else
            {
                /* half duplex */
                bp->initialFramesInTempInputBuffer = 0;
                bp->initialFramesInTempOutputBuffer = 0;
            }
        }
    }


    bp->framesInTempInputBuffer = bp->initialFramesInTempInputBuffer;
    bp->framesInTempOutputBuffer = bp->initialFramesInTempOutputBuffer;

    
    if( inputChannelCount > 0 )
    {
        bytesPerSample = Pa_GetSampleSize( hostInputSampleFormat );
        if( bytesPerSample > 0 )
        {
            bp->bytesPerHostInputSample = bytesPerSample;
        }
        else
        {
            result = bytesPerSample;
            goto error;
        }

        bytesPerSample = Pa_GetSampleSize( userInputSampleFormat );
        if( bytesPerSample > 0 )
        {
            bp->bytesPerUserInputSample = bytesPerSample;
        }
        else
        {
            result = bytesPerSample;
            goto error;
        }

        bp->inputConverter =
            PaUtil_SelectConverter( hostInputSampleFormat, userInputSampleFormat, streamFlags );

        bp->inputZeroer = PaUtil_SelectZeroer( hostInputSampleFormat );
            
        bp->userInputIsInterleaved = (userInputSampleFormat & paNonInterleaved)?0:1;


        tempInputBufferSize =
            bp->framesPerTempBuffer * bp->bytesPerUserInputSample * inputChannelCount;
         
        bp->tempInputBuffer = PaUtil_AllocateMemory( tempInputBufferSize );
        if( bp->tempInputBuffer == 0 )
        {
            result = paInsufficientMemory;
            goto error;
        }
        
        if( bp->framesInTempInputBuffer > 0 )
            memset( bp->tempInputBuffer, 0, tempInputBufferSize );

        if( userInputSampleFormat & paNonInterleaved )
        {
            bp->tempInputBufferPtrs =
                (void **)PaUtil_AllocateMemory( sizeof(void*)*inputChannelCount );
            if( bp->tempInputBufferPtrs == 0 )
            {
                result = paInsufficientMemory;
                goto error;
            }
        }

        bp->hostInputChannels[0] = (PaUtilChannelDescriptor*)
                PaUtil_AllocateMemory( sizeof(PaUtilChannelDescriptor) * inputChannelCount * 2);
        if( bp->hostInputChannels[0] == 0 )
        {
            result = paInsufficientMemory;
            goto error;
        }

        bp->hostInputChannels[1] = &bp->hostInputChannels[0][inputChannelCount];
    }

    if( outputChannelCount > 0 )
    {
        bytesPerSample = Pa_GetSampleSize( hostOutputSampleFormat );
        if( bytesPerSample > 0 )
        {
            bp->bytesPerHostOutputSample = bytesPerSample;
        }
        else
        {
            result = bytesPerSample;
            goto error;
        }

        bytesPerSample = Pa_GetSampleSize( userOutputSampleFormat );
        if( bytesPerSample > 0 )
        {
            bp->bytesPerUserOutputSample = bytesPerSample;
        }
        else
        {
            result = bytesPerSample;
            goto error;
        }

        bp->outputConverter =
            PaUtil_SelectConverter( userOutputSampleFormat, hostOutputSampleFormat, streamFlags );

        bp->outputZeroer = PaUtil_SelectZeroer( hostOutputSampleFormat );

        bp->userOutputIsInterleaved = (userOutputSampleFormat & paNonInterleaved)?0:1;

        tempOutputBufferSize =
                bp->framesPerTempBuffer * bp->bytesPerUserOutputSample * outputChannelCount;

        bp->tempOutputBuffer = PaUtil_AllocateMemory( tempOutputBufferSize );
        if( bp->tempOutputBuffer == 0 )
        {
            result = paInsufficientMemory;
            goto error;
        }

        if( bp->framesInTempOutputBuffer > 0 )
            memset( bp->tempOutputBuffer, 0, tempOutputBufferSize );
        
        if( userOutputSampleFormat & paNonInterleaved )
        {
            bp->tempOutputBufferPtrs =
                (void **)PaUtil_AllocateMemory( sizeof(void*)*outputChannelCount );
            if( bp->tempOutputBufferPtrs == 0 )
            {
                result = paInsufficientMemory;
                goto error;
            }
        }

        bp->hostOutputChannels[0] = (PaUtilChannelDescriptor*)
                PaUtil_AllocateMemory( sizeof(PaUtilChannelDescriptor)*outputChannelCount * 2 );
        if( bp->hostOutputChannels[0] == 0 )
        {                                                                     
            result = paInsufficientMemory;
            goto error;
        }

        bp->hostOutputChannels[1] = &bp->hostOutputChannels[0][outputChannelCount];
    }

    PaUtil_InitializeTriangularDitherState( &bp->ditherGenerator );

    bp->samplePeriod = 1. / sampleRate;

    bp->streamCallback = streamCallback;
    bp->userData = userData;

    return result;

error:
    if( bp->tempInputBuffer )
        PaUtil_FreeMemory( bp->tempInputBuffer );

    if( bp->tempInputBufferPtrs )
        PaUtil_FreeMemory( bp->tempInputBufferPtrs );

    if( bp->hostInputChannels[0] )
        PaUtil_FreeMemory( bp->hostInputChannels[0] );

    if( bp->tempOutputBuffer )
        PaUtil_FreeMemory( bp->tempOutputBuffer );

    if( bp->tempOutputBufferPtrs )
        PaUtil_FreeMemory( bp->tempOutputBufferPtrs );

    if( bp->hostOutputChannels[0] )
        PaUtil_FreeMemory( bp->hostOutputChannels[0] );

    return result;
}


void PaUtil_TerminateBufferProcessor( PaUtilBufferProcessor* bp )
{
    if( bp->tempInputBuffer )
        PaUtil_FreeMemory( bp->tempInputBuffer );

    if( bp->tempInputBufferPtrs )
        PaUtil_FreeMemory( bp->tempInputBufferPtrs );

    if( bp->hostInputChannels[0] )
        PaUtil_FreeMemory( bp->hostInputChannels[0] );
        
    if( bp->tempOutputBuffer )
        PaUtil_FreeMemory( bp->tempOutputBuffer );

    if( bp->tempOutputBufferPtrs )
        PaUtil_FreeMemory( bp->tempOutputBufferPtrs );

    if( bp->hostOutputChannels[0] )
        PaUtil_FreeMemory( bp->hostOutputChannels[0] );
}


void PaUtil_ResetBufferProcessor( PaUtilBufferProcessor* bp )
{
    unsigned long tempInputBufferSize, tempOutputBufferSize;

    bp->framesInTempInputBuffer = bp->initialFramesInTempInputBuffer;
    bp->framesInTempOutputBuffer = bp->initialFramesInTempOutputBuffer;

    if( bp->framesInTempInputBuffer > 0 )
    {
        tempInputBufferSize =
            bp->framesPerTempBuffer * bp->bytesPerUserInputSample * bp->inputChannelCount;
        memset( bp->tempInputBuffer, 0, tempInputBufferSize );
    }

    if( bp->framesInTempOutputBuffer > 0 )
    {      
        tempOutputBufferSize =
            bp->framesPerTempBuffer * bp->bytesPerUserOutputSample * bp->outputChannelCount;
        memset( bp->tempOutputBuffer, 0, tempOutputBufferSize );
    }
}


unsigned long PaUtil_GetBufferProcessorInputLatency( PaUtilBufferProcessor* bp )
{
    return bp->initialFramesInTempInputBuffer;
}


unsigned long PaUtil_GetBufferProcessorOutputLatency( PaUtilBufferProcessor* bp )
{
    return bp->initialFramesInTempOutputBuffer;
}


void PaUtil_SetInputFrameCount( PaUtilBufferProcessor* bp,
        unsigned long frameCount )
{
    if( frameCount == 0 )
        bp->hostInputFrameCount[0] = bp->framesPerHostBuffer;
    else
        bp->hostInputFrameCount[0] = frameCount;
}
        

void PaUtil_SetNoInput( PaUtilBufferProcessor* bp )
{
    assert( bp->inputChannelCount > 0 );

    bp->hostInputChannels[0][0].data = 0;
}


void PaUtil_SetInputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data, unsigned int stride )
{
    assert( channel < bp->inputChannelCount );
    
    bp->hostInputChannels[0][channel].data = data;
    bp->hostInputChannels[0][channel].stride = stride;
}


void PaUtil_SetInterleavedInputChannels( PaUtilBufferProcessor* bp,
        unsigned int firstChannel, void *data, unsigned int channelCount )
{
    unsigned int i;
    unsigned int channel = firstChannel;
    unsigned char *p = (unsigned char*)data;

    if( channelCount == 0 )
        channelCount = bp->inputChannelCount;

    assert( firstChannel < bp->inputChannelCount );
    assert( firstChannel + channelCount <= bp->inputChannelCount );

    for( i=0; i< channelCount; ++i )
    {
        bp->hostInputChannels[0][channel+i].data = p;
        p += bp->bytesPerHostInputSample;
        bp->hostInputChannels[0][channel+i].stride = channelCount;
    }
}


void PaUtil_SetNonInterleavedInputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data )
{
    assert( channel < bp->inputChannelCount );
    
    bp->hostInputChannels[0][channel].data = data;
    bp->hostInputChannels[0][channel].stride = 1;
}


void PaUtil_Set2ndInputFrameCount( PaUtilBufferProcessor* bp,
        unsigned long frameCount )
{
    bp->hostInputFrameCount[1] = frameCount;
}


void PaUtil_Set2ndInputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data, unsigned int stride )
{
    assert( channel < bp->inputChannelCount );

    bp->hostInputChannels[1][channel].data = data;
    bp->hostInputChannels[1][channel].stride = stride;
}


void PaUtil_Set2ndInterleavedInputChannels( PaUtilBufferProcessor* bp,
        unsigned int firstChannel, void *data, unsigned int channelCount )
{
    unsigned int i;
    unsigned int channel = firstChannel;
    unsigned char *p = (unsigned char*)data;

    if( channelCount == 0 )
        channelCount = bp->inputChannelCount;

    assert( firstChannel < bp->inputChannelCount );
    assert( firstChannel + channelCount <= bp->inputChannelCount );
    
    for( i=0; i< channelCount; ++i )
    {
        bp->hostInputChannels[1][channel+i].data = p;
        p += bp->bytesPerHostInputSample;
        bp->hostInputChannels[1][channel+i].stride = channelCount;
    }
}

        
void PaUtil_Set2ndNonInterleavedInputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data )
{
    assert( channel < bp->inputChannelCount );
    
    bp->hostInputChannels[1][channel].data = data;
    bp->hostInputChannels[1][channel].stride = 1;
}


void PaUtil_SetOutputFrameCount( PaUtilBufferProcessor* bp,
        unsigned long frameCount )
{
    if( frameCount == 0 )
        bp->hostOutputFrameCount[0] = bp->framesPerHostBuffer;
    else
        bp->hostOutputFrameCount[0] = frameCount;
}


void PaUtil_SetNoOutput( PaUtilBufferProcessor* bp )
{
    assert( bp->outputChannelCount > 0 );

    bp->hostOutputChannels[0][0].data = 0;
}


void PaUtil_SetOutputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data, unsigned int stride )
{
    assert( channel < bp->outputChannelCount );
    assert( data != NULL );

    bp->hostOutputChannels[0][channel].data = data;
    bp->hostOutputChannels[0][channel].stride = stride;
}


void PaUtil_SetInterleavedOutputChannels( PaUtilBufferProcessor* bp,
        unsigned int firstChannel, void *data, unsigned int channelCount )
{
    unsigned int i;
    unsigned int channel = firstChannel;
    unsigned char *p = (unsigned char*)data;

    if( channelCount == 0 )
        channelCount = bp->outputChannelCount;

    assert( firstChannel < bp->outputChannelCount );
    assert( firstChannel + channelCount <= bp->outputChannelCount );
    
    for( i=0; i< channelCount; ++i )
    {
        PaUtil_SetOutputChannel( bp, channel + i, p, channelCount );
        p += bp->bytesPerHostOutputSample;
    }
}


void PaUtil_SetNonInterleavedOutputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data )
{
    assert( channel < bp->outputChannelCount );

    PaUtil_SetOutputChannel( bp, channel, data, 1 );
}


void PaUtil_Set2ndOutputFrameCount( PaUtilBufferProcessor* bp,
        unsigned long frameCount )
{
    bp->hostOutputFrameCount[1] = frameCount;
}


void PaUtil_Set2ndOutputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data, unsigned int stride )
{
    assert( channel < bp->outputChannelCount );
    assert( data != NULL );

    bp->hostOutputChannels[1][channel].data = data;
    bp->hostOutputChannels[1][channel].stride = stride;
}


void PaUtil_Set2ndInterleavedOutputChannels( PaUtilBufferProcessor* bp,
        unsigned int firstChannel, void *data, unsigned int channelCount )
{
    unsigned int i;
    unsigned int channel = firstChannel;
    unsigned char *p = (unsigned char*)data;

    if( channelCount == 0 )
        channelCount = bp->outputChannelCount;

    assert( firstChannel < bp->outputChannelCount );
    assert( firstChannel + channelCount <= bp->outputChannelCount );
    
    for( i=0; i< channelCount; ++i )
    {
        PaUtil_Set2ndOutputChannel( bp, channel + i, p, channelCount );
        p += bp->bytesPerHostOutputSample;
    }
}

        
void PaUtil_Set2ndNonInterleavedOutputChannel( PaUtilBufferProcessor* bp,
        unsigned int channel, void *data )
{
    assert( channel < bp->outputChannelCount );
    
    PaUtil_Set2ndOutputChannel( bp, channel, data, 1 );
}


void PaUtil_BeginBufferProcessing( PaUtilBufferProcessor* bp,
        PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags callbackStatusFlags )
{
    bp->timeInfo = timeInfo;

    /* the first streamCallback will be called to process samples which are
        currently in the input buffer before the ones starting at the timeInfo time */
        
    bp->timeInfo->inputBufferAdcTime -= bp->framesInTempInputBuffer * bp->samplePeriod;
    
    bp->timeInfo->currentTime = 0; /** FIXME: @todo time info currentTime not implemented */

    /* the first streamCallback will be called to generate samples which will be
        outputted after the frames currently in the output buffer have been
        outputted. */
    bp->timeInfo->outputBufferDacTime += bp->framesInTempOutputBuffer * bp->samplePeriod;

    bp->callbackStatusFlags = callbackStatusFlags;

    bp->hostInputFrameCount[1] = 0;
    bp->hostOutputFrameCount[1] = 0;
}


/*
    NonAdaptingProcess() is a simple buffer copying adaptor that can handle
    both full and half duplex copies. It processes framesToProcess frames,
    broken into blocks bp->framesPerTempBuffer long.
    This routine can be used when the streamCallback doesn't care what length
    the buffers are, or when framesToProcess is an integer multiple of
    bp->framesPerTempBuffer, in which case streamCallback will always be called
    with bp->framesPerTempBuffer samples.
*/
static unsigned long NonAdaptingProcess( PaUtilBufferProcessor *bp,
        int *streamCallbackResult,
        PaUtilChannelDescriptor *hostInputChannels,
        PaUtilChannelDescriptor *hostOutputChannels,
        unsigned long framesToProcess )
{
    void *userInput, *userOutput;
    unsigned char *srcBytePtr, *destBytePtr;
    unsigned int srcSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int srcChannelStrideBytes; /* stride from one channel to the next, in bytes */
    unsigned int destSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int destChannelStrideBytes; /* stride from one channel to the next, in bytes */
    unsigned int i;
    unsigned long frameCount;
    unsigned long framesToGo = framesToProcess;
    unsigned long framesProcessed = 0;


    if( *streamCallbackResult == paContinue )
    {
        do
        {
            frameCount = PA_MIN_( bp->framesPerTempBuffer, framesToGo );

            /* configure user input buffer and convert input data (host -> user) */
            if( bp->inputChannelCount == 0 )
            {
                /* no input */
                userInput = 0;
            }
            else /* there are input channels */
            {
                /*
                    could use more elaborate logic here and sometimes process
                    buffers in-place.
                */
            
                destBytePtr = (unsigned char *)bp->tempInputBuffer;

                if( bp->userInputIsInterleaved )
                {
                    destSampleStrideSamples = bp->inputChannelCount;
                    destChannelStrideBytes = bp->bytesPerUserInputSample;
                    userInput = bp->tempInputBuffer;
                }
                else /* user input is not interleaved */
                {
                    destSampleStrideSamples = 1;
                    destChannelStrideBytes = frameCount * bp->bytesPerUserInputSample;

                    /* setup non-interleaved ptrs */
                    for( i=0; i<bp->inputChannelCount; ++i )
                    {
                        bp->tempInputBufferPtrs[i] = ((unsigned char*)bp->tempInputBuffer) +
                            i * bp->bytesPerUserInputSample * frameCount;
                    }
                
                    userInput = bp->tempInputBufferPtrs;
                }

                if( !bp->hostInputChannels[0][0].data )
                {
                    /* no input was supplied (see PaUtil_SetNoInput), so
                        zero the input buffer */

                    for( i=0; i<bp->inputChannelCount; ++i )
                    {
                        bp->inputZeroer( destBytePtr, destSampleStrideSamples, frameCount );
                        destBytePtr += destChannelStrideBytes;  /* skip to next destination channel */
                    }
                }
                else
                {
                    for( i=0; i<bp->inputChannelCount; ++i )
                    {
                        bp->inputConverter( destBytePtr, destSampleStrideSamples,
                                                hostInputChannels[i].data,
                                                hostInputChannels[i].stride,
                                                frameCount, &bp->ditherGenerator );

                        destBytePtr += destChannelStrideBytes;  /* skip to next destination channel */

                        /* advance src ptr for next iteration */
                        hostInputChannels[i].data = ((unsigned char*)hostInputChannels[i].data) +
                                frameCount * hostInputChannels[i].stride * bp->bytesPerHostInputSample;
                    }
                }
            }

            /* configure user output buffer */
            if( bp->outputChannelCount == 0 )
            {
                /* no output */
                userOutput = 0;
            }
            else /* there are output channels */
            {
                if( bp->userOutputIsInterleaved )
                {
                    userOutput = bp->tempOutputBuffer;
                }
                else /* user output is not interleaved */
                {
                    for( i = 0; i < bp->outputChannelCount; ++i )
                    {
                        bp->tempOutputBufferPtrs[i] = ((unsigned char*)bp->tempOutputBuffer) +
                            i * bp->bytesPerUserOutputSample * frameCount;
                    }

                    userOutput = bp->tempOutputBufferPtrs;
                }
            }
        
            *streamCallbackResult = bp->streamCallback( userInput, userOutput,
                    frameCount, bp->timeInfo, bp->callbackStatusFlags, bp->userData );

            if( *streamCallbackResult == paAbort )
            {
                /* callback returned paAbort, don't advance framesProcessed
                        and framesToGo, they will be handled below */
            }
            else
            {
                bp->timeInfo->inputBufferAdcTime += frameCount * bp->samplePeriod;
                bp->timeInfo->outputBufferDacTime += frameCount * bp->samplePeriod;

                /* convert output data (user -> host) */
                
                if( bp->outputChannelCount != 0 && bp->hostOutputChannels[0][0].data )
                {
                    /*
                        could use more elaborate logic here and sometimes process
                        buffers in-place.
                    */
            
                    srcBytePtr = (unsigned char *)bp->tempOutputBuffer;

                    if( bp->userOutputIsInterleaved )
                    {
                        srcSampleStrideSamples = bp->outputChannelCount;
                        srcChannelStrideBytes = bp->bytesPerUserOutputSample;
                    }
                    else /* user output is not interleaved */
                    {
                        srcSampleStrideSamples = 1;
                        srcChannelStrideBytes = frameCount * bp->bytesPerUserOutputSample;
                    }

                    for( i=0; i<bp->outputChannelCount; ++i )
                    {
                        bp->outputConverter(    hostOutputChannels[i].data,
                                                hostOutputChannels[i].stride,
                                                srcBytePtr, srcSampleStrideSamples,
                                                frameCount, &bp->ditherGenerator );

                        srcBytePtr += srcChannelStrideBytes;  /* skip to next source channel */

                        /* advance dest ptr for next iteration */
                        hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                                frameCount * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
                    }
                }
             
                framesProcessed += frameCount;

                framesToGo -= frameCount;
            }
        }
        while( framesToGo > 0  && *streamCallbackResult == paContinue );
    }

    if( framesToGo > 0 )
    {
        /* zero any remaining frames output. There will only be remaining frames
            if the callback has returned paComplete or paAbort */

        frameCount = framesToGo;

        if( bp->outputChannelCount != 0 && bp->hostOutputChannels[0][0].data )
        {
            for( i=0; i<bp->outputChannelCount; ++i )
            {
                bp->outputZeroer(   hostOutputChannels[i].data,
                                    hostOutputChannels[i].stride,
                                    frameCount );

                /* advance dest ptr for next iteration */
                hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                        frameCount * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
            }
        }

        framesProcessed += frameCount;
    }

    return framesProcessed;
}


/*
    AdaptingInputOnlyProcess() is a half duplex input buffer processor. It
    converts data from the input buffers into the temporary input buffer,
    when the temporary input buffer is full, it calls the streamCallback.
*/
static unsigned long AdaptingInputOnlyProcess( PaUtilBufferProcessor *bp,
        int *streamCallbackResult,
        PaUtilChannelDescriptor *hostInputChannels,
        unsigned long framesToProcess )
{
    void *userInput, *userOutput;
    unsigned char *destBytePtr;
    unsigned int destSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int destChannelStrideBytes; /* stride from one channel to the next, in bytes */
    unsigned int i;
    unsigned long frameCount;
    unsigned long framesToGo = framesToProcess;
    unsigned long framesProcessed = 0;
    
    userOutput = 0;

    do
    {
        frameCount = ( bp->framesInTempInputBuffer + framesToGo > bp->framesPerUserBuffer )
                ? ( bp->framesPerUserBuffer - bp->framesInTempInputBuffer )
                : framesToGo;

        /* convert frameCount samples into temp buffer */

        if( bp->userInputIsInterleaved )
        {
            destBytePtr = ((unsigned char*)bp->tempInputBuffer) +
                    bp->bytesPerUserInputSample * bp->inputChannelCount *
                    bp->framesInTempInputBuffer;
                      
            destSampleStrideSamples = bp->inputChannelCount;
            destChannelStrideBytes = bp->bytesPerUserInputSample;

            userInput = bp->tempInputBuffer;
        }
        else /* user input is not interleaved */
        {
            destBytePtr = ((unsigned char*)bp->tempInputBuffer) +
                    bp->bytesPerUserInputSample * bp->framesInTempInputBuffer;

            destSampleStrideSamples = 1;
            destChannelStrideBytes = bp->framesPerUserBuffer * bp->bytesPerUserInputSample;

            /* setup non-interleaved ptrs */
            for( i=0; i<bp->inputChannelCount; ++i )
            {
                bp->tempInputBufferPtrs[i] = ((unsigned char*)bp->tempInputBuffer) +
                    i * bp->bytesPerUserInputSample * bp->framesPerUserBuffer;
            }
                    
            userInput = bp->tempInputBufferPtrs;
        }

        for( i=0; i<bp->inputChannelCount; ++i )
        {
            bp->inputConverter( destBytePtr, destSampleStrideSamples,
                                    hostInputChannels[i].data,
                                    hostInputChannels[i].stride,
                                    frameCount, &bp->ditherGenerator );

            destBytePtr += destChannelStrideBytes;  /* skip to next destination channel */

            /* advance src ptr for next iteration */
            hostInputChannels[i].data = ((unsigned char*)hostInputChannels[i].data) +
                    frameCount * hostInputChannels[i].stride * bp->bytesPerHostInputSample;
        }

        bp->framesInTempInputBuffer += frameCount;

        if( bp->framesInTempInputBuffer == bp->framesPerUserBuffer )
        {
            /**
            @todo (non-critical optimisation)
            The conditional below implements the continue/complete/abort mechanism
            simply by continuing on iterating through the input buffer, but not
            passing the data to the callback. With care, the outer loop could be
            terminated earlier, thus some unneeded conversion cycles would be
            saved.
            */
            if( *streamCallbackResult == paContinue )
            {
                bp->timeInfo->outputBufferDacTime = 0;

                *streamCallbackResult = bp->streamCallback( userInput, userOutput,
                        bp->framesPerUserBuffer, bp->timeInfo,
                        bp->callbackStatusFlags, bp->userData );

                bp->timeInfo->inputBufferAdcTime += frameCount * bp->samplePeriod;
            }
            
            bp->framesInTempInputBuffer = 0;
        }

        framesProcessed += frameCount;

        framesToGo -= frameCount;
    }while( framesToGo > 0 );

    return framesProcessed;
}


/*
    AdaptingOutputOnlyProcess() is a half duplex output buffer processor.
    It converts data from the temporary output buffer, to the output buffers,
    when the temporary output buffer is empty, it calls the streamCallback.
*/
static unsigned long AdaptingOutputOnlyProcess( PaUtilBufferProcessor *bp,
        int *streamCallbackResult,
        PaUtilChannelDescriptor *hostOutputChannels,
        unsigned long framesToProcess )
{
    void *userInput, *userOutput;
    unsigned char *srcBytePtr;
    unsigned int srcSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int srcChannelStrideBytes;  /* stride from one channel to the next, in bytes */
    unsigned int i;
    unsigned long frameCount;
    unsigned long framesToGo = framesToProcess;
    unsigned long framesProcessed = 0;

    do
    {
        if( bp->framesInTempOutputBuffer == 0 && *streamCallbackResult == paContinue )
        {
            userInput = 0;

            /* setup userOutput */
            if( bp->userOutputIsInterleaved )
            {
                userOutput = bp->tempOutputBuffer;
            }
            else /* user output is not interleaved */
            {
                for( i = 0; i < bp->outputChannelCount; ++i )
                {
                    bp->tempOutputBufferPtrs[i] = ((unsigned char*)bp->tempOutputBuffer) +
                            i * bp->framesPerUserBuffer * bp->bytesPerUserOutputSample;
                }

                userOutput = bp->tempOutputBufferPtrs;
            }

            bp->timeInfo->inputBufferAdcTime = 0;
            
            *streamCallbackResult = bp->streamCallback( userInput, userOutput,
                    bp->framesPerUserBuffer, bp->timeInfo,
                    bp->callbackStatusFlags, bp->userData );

            if( *streamCallbackResult == paAbort )
            {
                /* if the callback returned paAbort, we disregard its output */
            }
            else
            {
                bp->timeInfo->outputBufferDacTime += bp->framesPerUserBuffer * bp->samplePeriod;

                bp->framesInTempOutputBuffer = bp->framesPerUserBuffer;
            }
        }

        if( bp->framesInTempOutputBuffer > 0 )
        {
            /* convert frameCount frames from user buffer to host buffer */

            frameCount = PA_MIN_( bp->framesInTempOutputBuffer, framesToGo );

            if( bp->userOutputIsInterleaved )
            {
                srcBytePtr = ((unsigned char*)bp->tempOutputBuffer) +
                        bp->bytesPerUserOutputSample * bp->outputChannelCount *
                        (bp->framesPerUserBuffer - bp->framesInTempOutputBuffer);

                srcSampleStrideSamples = bp->outputChannelCount;
                srcChannelStrideBytes = bp->bytesPerUserOutputSample;
            }
            else /* user output is not interleaved */
            {
                srcBytePtr = ((unsigned char*)bp->tempOutputBuffer) +
                        bp->bytesPerUserOutputSample *
                        (bp->framesPerUserBuffer - bp->framesInTempOutputBuffer);
                            
                srcSampleStrideSamples = 1;
                srcChannelStrideBytes = bp->framesPerUserBuffer * bp->bytesPerUserOutputSample;
            }

            for( i=0; i<bp->outputChannelCount; ++i )
            {
                bp->outputConverter(    hostOutputChannels[i].data,
                                        hostOutputChannels[i].stride,
                                        srcBytePtr, srcSampleStrideSamples,
                                        frameCount, &bp->ditherGenerator );

                srcBytePtr += srcChannelStrideBytes;  /* skip to next source channel */

                /* advance dest ptr for next iteration */
                hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                        frameCount * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
            }

            bp->framesInTempOutputBuffer -= frameCount;
        }
        else
        {
            /* no more user data is available because the callback has returned
                paComplete or paAbort. Fill the remainder of the host buffer
                with zeros.
            */

            frameCount = framesToGo;

            for( i=0; i<bp->outputChannelCount; ++i )
            {
                bp->outputZeroer(   hostOutputChannels[i].data,
                                    hostOutputChannels[i].stride,
                                    frameCount );

                /* advance dest ptr for next iteration */
                hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                        frameCount * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
            }
        }
        
        framesProcessed += frameCount;
        
        framesToGo -= frameCount;

    }while( framesToGo > 0 );

    return framesProcessed;
}

/* CopyTempOutputBuffersToHostOutputBuffers is called from AdaptingProcess to copy frames from
	tempOutputBuffer to hostOutputChannels. This includes data conversion
	and interleaving. 
*/
static void CopyTempOutputBuffersToHostOutputBuffers( PaUtilBufferProcessor *bp)
{
    unsigned long maxFramesToCopy;
    PaUtilChannelDescriptor *hostOutputChannels;
    unsigned int frameCount;
    unsigned char *srcBytePtr;
    unsigned int srcSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int srcChannelStrideBytes; /* stride from one channel to the next, in bytes */
    unsigned int i;

     /* copy frames from user to host output buffers */
     while( bp->framesInTempOutputBuffer > 0 &&
             ((bp->hostOutputFrameCount[0] + bp->hostOutputFrameCount[1]) > 0) )
     {
         maxFramesToCopy = bp->framesInTempOutputBuffer;

         /* select the output buffer set (1st or 2nd) */
         if( bp->hostOutputFrameCount[0] > 0 )
         {
             hostOutputChannels = bp->hostOutputChannels[0];
             frameCount = PA_MIN_( bp->hostOutputFrameCount[0], maxFramesToCopy );
         }
         else
         {
             hostOutputChannels = bp->hostOutputChannels[1];
             frameCount = PA_MIN_( bp->hostOutputFrameCount[1], maxFramesToCopy );
         }

         if( bp->userOutputIsInterleaved )
         {
             srcBytePtr = ((unsigned char*)bp->tempOutputBuffer) +
                     bp->bytesPerUserOutputSample * bp->outputChannelCount *
                     (bp->framesPerUserBuffer - bp->framesInTempOutputBuffer);
                         
             srcSampleStrideSamples = bp->outputChannelCount;
             srcChannelStrideBytes = bp->bytesPerUserOutputSample;
         }
         else /* user output is not interleaved */
         {
             srcBytePtr = ((unsigned char*)bp->tempOutputBuffer) +
                     bp->bytesPerUserOutputSample *
                     (bp->framesPerUserBuffer - bp->framesInTempOutputBuffer);

             srcSampleStrideSamples = 1;
             srcChannelStrideBytes = bp->framesPerUserBuffer * bp->bytesPerUserOutputSample;
         }

         for( i=0; i<bp->outputChannelCount; ++i )
         {
             assert( hostOutputChannels[i].data != NULL );
             bp->outputConverter(    hostOutputChannels[i].data,
                                     hostOutputChannels[i].stride,
                                     srcBytePtr, srcSampleStrideSamples,
                                     frameCount, &bp->ditherGenerator );

             srcBytePtr += srcChannelStrideBytes;  /* skip to next source channel */

             /* advance dest ptr for next iteration */
             hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                     frameCount * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
         }

         if( bp->hostOutputFrameCount[0] > 0 )
             bp->hostOutputFrameCount[0] -= frameCount;
         else
             bp->hostOutputFrameCount[1] -= frameCount;

         bp->framesInTempOutputBuffer -= frameCount;
     }
}

/*
    AdaptingProcess is a full duplex adapting buffer processor. It converts
    data from the temporary output buffer into the host output buffers, then
    from the host input buffers into the temporary input buffers. Calling the
    streamCallback when necessary.
    When processPartialUserBuffers is 0, all available input data will be
    consumed and all available output space will be filled. When
    processPartialUserBuffers is non-zero, as many full user buffers
    as possible will be processed, but partial buffers will not be consumed.
*/
static unsigned long AdaptingProcess( PaUtilBufferProcessor *bp,
        int *streamCallbackResult, int processPartialUserBuffers )
{
    void *userInput, *userOutput;
    unsigned long framesProcessed = 0;
    unsigned long framesAvailable;
    unsigned long endProcessingMinFrameCount;
    unsigned long maxFramesToCopy;
    PaUtilChannelDescriptor *hostInputChannels, *hostOutputChannels;
    unsigned int frameCount;
    unsigned char *destBytePtr;
    unsigned int destSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int destChannelStrideBytes; /* stride from one channel to the next, in bytes */
    unsigned int i, j;
 

    framesAvailable = bp->hostInputFrameCount[0] + bp->hostInputFrameCount[1];/* this is assumed to be the same as the output buffer's frame count */

    if( processPartialUserBuffers )
        endProcessingMinFrameCount = 0;
    else
        endProcessingMinFrameCount = (bp->framesPerUserBuffer - 1);

    /* Fill host output with remaining frames in user output (tempOutputBuffer) */
    CopyTempOutputBuffersToHostOutputBuffers( bp );		  	

    while( framesAvailable > endProcessingMinFrameCount ) 
    {

        if( bp->framesInTempOutputBuffer == 0 && *streamCallbackResult != paContinue )
        {
            /* the callback will not be called any more, so zero what remains
                of the host output buffers */

            for( i=0; i<2; ++i )
            {
                frameCount = bp->hostOutputFrameCount[i];
                if( frameCount > 0 )
                {
                    hostOutputChannels = bp->hostOutputChannels[i];
                    
                    for( j=0; j<bp->outputChannelCount; ++j )
                    {
                        bp->outputZeroer(   hostOutputChannels[j].data,
                                            hostOutputChannels[j].stride,
                                            frameCount );

                        /* advance dest ptr for next iteration  */
                        hostOutputChannels[j].data = ((unsigned char*)hostOutputChannels[j].data) +
                                frameCount * hostOutputChannels[j].stride * bp->bytesPerHostOutputSample;
                    }
                    bp->hostOutputFrameCount[i] = 0;
                }
            }
        }          


        /* copy frames from host to user input buffers */
        while( bp->framesInTempInputBuffer < bp->framesPerUserBuffer &&
                ((bp->hostInputFrameCount[0] + bp->hostInputFrameCount[1]) > 0) )
        {
            maxFramesToCopy = bp->framesPerUserBuffer - bp->framesInTempInputBuffer;

            /* select the input buffer set (1st or 2nd) */
            if( bp->hostInputFrameCount[0] > 0 )
            {
                hostInputChannels = bp->hostInputChannels[0];
                frameCount = PA_MIN_( bp->hostInputFrameCount[0], maxFramesToCopy );
            }
            else
            {
                hostInputChannels = bp->hostInputChannels[1];
                frameCount = PA_MIN_( bp->hostInputFrameCount[1], maxFramesToCopy );
            }

            /* configure conversion destination pointers */
            if( bp->userInputIsInterleaved )
            {
                destBytePtr = ((unsigned char*)bp->tempInputBuffer) +
                        bp->bytesPerUserInputSample * bp->inputChannelCount *
                        bp->framesInTempInputBuffer;

                destSampleStrideSamples = bp->inputChannelCount;
                destChannelStrideBytes = bp->bytesPerUserInputSample;
            }
            else /* user input is not interleaved */
            {
                destBytePtr = ((unsigned char*)bp->tempInputBuffer) +
                        bp->bytesPerUserInputSample * bp->framesInTempInputBuffer;

                destSampleStrideSamples = 1;
                destChannelStrideBytes = bp->framesPerUserBuffer * bp->bytesPerUserInputSample;
            }

            for( i=0; i<bp->inputChannelCount; ++i )
            {
                bp->inputConverter( destBytePtr, destSampleStrideSamples,
                                        hostInputChannels[i].data,
                                        hostInputChannels[i].stride,
                                        frameCount, &bp->ditherGenerator );

                destBytePtr += destChannelStrideBytes;  /* skip to next destination channel */

                /* advance src ptr for next iteration */
                hostInputChannels[i].data = ((unsigned char*)hostInputChannels[i].data) +
                        frameCount * hostInputChannels[i].stride * bp->bytesPerHostInputSample;
            }

            if( bp->hostInputFrameCount[0] > 0 )
                bp->hostInputFrameCount[0] -= frameCount;
            else
                bp->hostInputFrameCount[1] -= frameCount;
                
            bp->framesInTempInputBuffer += frameCount;

            /* update framesAvailable and framesProcessed based on input consumed
                unless something is very wrong this will also correspond to the
                amount of output generated */
            framesAvailable -= frameCount;
            framesProcessed += frameCount;
        }

        /* call streamCallback */
        if( bp->framesInTempInputBuffer == bp->framesPerUserBuffer &&
            bp->framesInTempOutputBuffer == 0 )
        {
            if( *streamCallbackResult == paContinue )
            {
                /* setup userInput */
                if( bp->userInputIsInterleaved )
                {
                    userInput = bp->tempInputBuffer;
                }
                else /* user input is not interleaved */
                {
                    for( i = 0; i < bp->inputChannelCount; ++i )
                    {
                        bp->tempInputBufferPtrs[i] = ((unsigned char*)bp->tempInputBuffer) +
                                i * bp->framesPerUserBuffer * bp->bytesPerUserInputSample;
                    }

                    userInput = bp->tempInputBufferPtrs;
                }

                /* setup userOutput */
                if( bp->userOutputIsInterleaved )
                {
                    userOutput = bp->tempOutputBuffer;
                }
                else /* user output is not interleaved */
                {
                    for( i = 0; i < bp->outputChannelCount; ++i )
                    {
                        bp->tempOutputBufferPtrs[i] = ((unsigned char*)bp->tempOutputBuffer) +
                                i * bp->framesPerUserBuffer * bp->bytesPerUserOutputSample;
                    }

                    userOutput = bp->tempOutputBufferPtrs;
                }

                /* call streamCallback */

                *streamCallbackResult = bp->streamCallback( userInput, userOutput,
                        bp->framesPerUserBuffer, bp->timeInfo,
                        bp->callbackStatusFlags, bp->userData );

                bp->timeInfo->inputBufferAdcTime += bp->framesPerUserBuffer * bp->samplePeriod;
                bp->timeInfo->outputBufferDacTime += bp->framesPerUserBuffer * bp->samplePeriod;

                bp->framesInTempInputBuffer = 0;

                if( *streamCallbackResult == paAbort )
                    bp->framesInTempOutputBuffer = 0;
                else
                    bp->framesInTempOutputBuffer = bp->framesPerUserBuffer;
            }
            else
            {
                /* paComplete or paAbort has already been called. */

                bp->framesInTempInputBuffer = 0;
            }
        }

        /* copy frames from user (tempOutputBuffer) to host output buffers (hostOutputChannels) 
           Means to process the user output provided by the callback. Has to be called after
            each callback. */
        CopyTempOutputBuffersToHostOutputBuffers( bp );		  	

    }
    
    return framesProcessed;
}


unsigned long PaUtil_EndBufferProcessing( PaUtilBufferProcessor* bp, int *streamCallbackResult )
{
    unsigned long framesToProcess, framesToGo;
    unsigned long framesProcessed = 0;
    
    if( bp->inputChannelCount != 0 && bp->outputChannelCount != 0
            && bp->hostInputChannels[0][0].data /* input was supplied (see PaUtil_SetNoInput) */
            && bp->hostOutputChannels[0][0].data /* output was supplied (see PaUtil_SetNoOutput) */ )
    {
        assert( (bp->hostInputFrameCount[0] + bp->hostInputFrameCount[1]) ==
                (bp->hostOutputFrameCount[0] + bp->hostOutputFrameCount[1]) );
    }

    assert( *streamCallbackResult == paContinue
            || *streamCallbackResult == paComplete
            || *streamCallbackResult == paAbort ); /* don't forget to pass in a valid callback result value */

    if( bp->useNonAdaptingProcess )
    {
        if( bp->inputChannelCount != 0 && bp->outputChannelCount != 0 )
        {
            /* full duplex non-adapting process, splice buffers if they are
                different lengths */

            framesToGo = bp->hostOutputFrameCount[0] + bp->hostOutputFrameCount[1]; /* relies on assert above for input/output equivalence */

            do{
                unsigned long noInputInputFrameCount;
                unsigned long *hostInputFrameCount;
                PaUtilChannelDescriptor *hostInputChannels;
                unsigned long noOutputOutputFrameCount;
                unsigned long *hostOutputFrameCount;
                PaUtilChannelDescriptor *hostOutputChannels;
                unsigned long framesProcessedThisIteration;

                if( !bp->hostInputChannels[0][0].data )
                {
                    /* no input was supplied (see PaUtil_SetNoInput)
                        NonAdaptingProcess knows how to deal with this
                    */
                    noInputInputFrameCount = framesToGo;
                    hostInputFrameCount = &noInputInputFrameCount;
                    hostInputChannels = 0;
                }
                else if( bp->hostInputFrameCount[0] != 0 )
                {
                    hostInputFrameCount = &bp->hostInputFrameCount[0];
                    hostInputChannels = bp->hostInputChannels[0];
                }
                else
                {
                    hostInputFrameCount = &bp->hostInputFrameCount[1];
                    hostInputChannels = bp->hostInputChannels[1];
                }

                if( !bp->hostOutputChannels[0][0].data )
                {
                    /* no output was supplied (see PaUtil_SetNoOutput)
                        NonAdaptingProcess knows how to deal with this
                    */
                    noOutputOutputFrameCount = framesToGo;
                    hostOutputFrameCount = &noOutputOutputFrameCount;
                    hostOutputChannels = 0;
                }
                if( bp->hostOutputFrameCount[0] != 0 )
                {
                    hostOutputFrameCount = &bp->hostOutputFrameCount[0];
                    hostOutputChannels = bp->hostOutputChannels[0];
                }
                else
                {
                    hostOutputFrameCount = &bp->hostOutputFrameCount[1];
                    hostOutputChannels = bp->hostOutputChannels[1];
                }

                framesToProcess = PA_MIN_( *hostInputFrameCount,
                                       *hostOutputFrameCount );

                assert( framesToProcess != 0 );
                
                framesProcessedThisIteration = NonAdaptingProcess( bp, streamCallbackResult,
                        hostInputChannels, hostOutputChannels,
                        framesToProcess );                                       

                *hostInputFrameCount -= framesProcessedThisIteration;
                *hostOutputFrameCount -= framesProcessedThisIteration;

                framesProcessed += framesProcessedThisIteration;
                framesToGo -= framesProcessedThisIteration;
                
            }while( framesToGo > 0 );
        }
        else
        {
            /* half duplex non-adapting process, just process 1st and 2nd buffer */
            /* process first buffer */

            framesToProcess = (bp->inputChannelCount != 0)
                            ? bp->hostInputFrameCount[0]
                            : bp->hostOutputFrameCount[0];

            framesProcessed = NonAdaptingProcess( bp, streamCallbackResult,
                        bp->hostInputChannels[0], bp->hostOutputChannels[0],
                        framesToProcess );

            /* process second buffer if provided */
    
            framesToProcess = (bp->inputChannelCount != 0)
                            ? bp->hostInputFrameCount[1]
                            : bp->hostOutputFrameCount[1];
            if( framesToProcess > 0 )
            {
                framesProcessed += NonAdaptingProcess( bp, streamCallbackResult,
                    bp->hostInputChannels[1], bp->hostOutputChannels[1],
                    framesToProcess );
            }
        }
    }
    else /* block adaption necessary*/
    {

        if( bp->inputChannelCount != 0 && bp->outputChannelCount != 0 )
        {
            /* full duplex */
            
            if( bp->hostBufferSizeMode == paUtilVariableHostBufferSizePartialUsageAllowed  )
            {
                framesProcessed = AdaptingProcess( bp, streamCallbackResult,
                        0 /* dont process partial user buffers */ );
            }
            else
            {
                framesProcessed = AdaptingProcess( bp, streamCallbackResult,
                        1 /* process partial user buffers */ );
            }
        }
        else if( bp->inputChannelCount != 0 )
        {
            /* input only */
            framesToProcess = bp->hostInputFrameCount[0];

            framesProcessed = AdaptingInputOnlyProcess( bp, streamCallbackResult,
                        bp->hostInputChannels[0], framesToProcess );

            framesToProcess = bp->hostInputFrameCount[1];
            if( framesToProcess > 0 )
            {
                framesProcessed += AdaptingInputOnlyProcess( bp, streamCallbackResult,
                        bp->hostInputChannels[1], framesToProcess );
            }
        }
        else
        {
            /* output only */
            framesToProcess = bp->hostOutputFrameCount[0];

            framesProcessed = AdaptingOutputOnlyProcess( bp, streamCallbackResult,
                        bp->hostOutputChannels[0], framesToProcess );

            framesToProcess = bp->hostOutputFrameCount[1];
            if( framesToProcess > 0 )
            {
                framesProcessed += AdaptingOutputOnlyProcess( bp, streamCallbackResult,
                        bp->hostOutputChannels[1], framesToProcess );
            }
        }
    }

    return framesProcessed;
}


int PaUtil_IsBufferProcessorOutputEmpty( PaUtilBufferProcessor* bp )
{
    return (bp->framesInTempOutputBuffer) ? 0 : 1;
} 


unsigned long PaUtil_CopyInput( PaUtilBufferProcessor* bp,
        void **buffer, unsigned long frameCount )
{
    PaUtilChannelDescriptor *hostInputChannels;
    unsigned int framesToCopy;
    unsigned char *destBytePtr;
    void **nonInterleavedDestPtrs;
    unsigned int destSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int destChannelStrideBytes; /* stride from one channel to the next, in bytes */
    unsigned int i;

    hostInputChannels = bp->hostInputChannels[0];
    framesToCopy = PA_MIN_( bp->hostInputFrameCount[0], frameCount );

    if( bp->userInputIsInterleaved )
    {
        destBytePtr = (unsigned char*)*buffer;
        
        destSampleStrideSamples = bp->inputChannelCount;
        destChannelStrideBytes = bp->bytesPerUserInputSample;

        for( i=0; i<bp->inputChannelCount; ++i )
        {
            bp->inputConverter( destBytePtr, destSampleStrideSamples,
                                hostInputChannels[i].data,
                                hostInputChannels[i].stride,
                                framesToCopy, &bp->ditherGenerator );

            destBytePtr += destChannelStrideBytes;  /* skip to next source channel */

            /* advance dest ptr for next iteration */
            hostInputChannels[i].data = ((unsigned char*)hostInputChannels[i].data) +
                    framesToCopy * hostInputChannels[i].stride * bp->bytesPerHostInputSample;
        }

        /* advance callers dest pointer (buffer) */
        *buffer = ((unsigned char *)*buffer) +
                framesToCopy * bp->inputChannelCount * bp->bytesPerUserInputSample;
    }
    else
    {
        /* user input is not interleaved */
        
        nonInterleavedDestPtrs = (void**)*buffer;

        destSampleStrideSamples = 1;
        
        for( i=0; i<bp->inputChannelCount; ++i )
        {
            destBytePtr = (unsigned char*)nonInterleavedDestPtrs[i];

            bp->inputConverter( destBytePtr, destSampleStrideSamples,
                                hostInputChannels[i].data,
                                hostInputChannels[i].stride,
                                framesToCopy, &bp->ditherGenerator );

            /* advance callers dest pointer (nonInterleavedDestPtrs[i]) */
            destBytePtr += bp->bytesPerUserInputSample * framesToCopy;
            nonInterleavedDestPtrs[i] = destBytePtr;
            
            /* advance dest ptr for next iteration */
            hostInputChannels[i].data = ((unsigned char*)hostInputChannels[i].data) +
                    framesToCopy * hostInputChannels[i].stride * bp->bytesPerHostInputSample;
        }
    }

    bp->hostInputFrameCount[0] -= framesToCopy;
    
    return framesToCopy;
}

unsigned long PaUtil_CopyOutput( PaUtilBufferProcessor* bp,
        const void ** buffer, unsigned long frameCount )
{
    PaUtilChannelDescriptor *hostOutputChannels;
    unsigned int framesToCopy;
    unsigned char *srcBytePtr;
    void **nonInterleavedSrcPtrs;
    unsigned int srcSampleStrideSamples; /* stride from one sample to the next within a channel, in samples */
    unsigned int srcChannelStrideBytes; /* stride from one channel to the next, in bytes */
    unsigned int i;

    hostOutputChannels = bp->hostOutputChannels[0];
    framesToCopy = PA_MIN_( bp->hostOutputFrameCount[0], frameCount );

    if( bp->userOutputIsInterleaved )
    {
        srcBytePtr = (unsigned char*)*buffer;
        
        srcSampleStrideSamples = bp->outputChannelCount;
        srcChannelStrideBytes = bp->bytesPerUserOutputSample;

        for( i=0; i<bp->outputChannelCount; ++i )
        {
            bp->outputConverter(    hostOutputChannels[i].data,
                                    hostOutputChannels[i].stride,
                                    srcBytePtr, srcSampleStrideSamples,
                                    framesToCopy, &bp->ditherGenerator );

            srcBytePtr += srcChannelStrideBytes;  /* skip to next source channel */

            /* advance dest ptr for next iteration */
            hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                    framesToCopy * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
        }

        /* advance callers source pointer (buffer) */
        *buffer = ((unsigned char *)*buffer) +
                framesToCopy * bp->outputChannelCount * bp->bytesPerUserOutputSample;

    }
    else
    {
        /* user output is not interleaved */
        
        nonInterleavedSrcPtrs = (void**)*buffer;

        srcSampleStrideSamples = 1;
        
        for( i=0; i<bp->outputChannelCount; ++i )
        {
            srcBytePtr = (unsigned char*)nonInterleavedSrcPtrs[i];
            
            bp->outputConverter(    hostOutputChannels[i].data,
                                    hostOutputChannels[i].stride,
                                    srcBytePtr, srcSampleStrideSamples,
                                    framesToCopy, &bp->ditherGenerator );


            /* advance callers source pointer (nonInterleavedSrcPtrs[i]) */
            srcBytePtr += bp->bytesPerUserOutputSample * framesToCopy;
            nonInterleavedSrcPtrs[i] = srcBytePtr;
            
            /* advance dest ptr for next iteration */
            hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                    framesToCopy * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
        }
    }

    bp->hostOutputFrameCount[0] += framesToCopy;
    
    return framesToCopy;
}


unsigned long PaUtil_ZeroOutput( PaUtilBufferProcessor* bp, unsigned long frameCount )
{
    PaUtilChannelDescriptor *hostOutputChannels;
    unsigned int framesToZero;
    unsigned int i;

    hostOutputChannels = bp->hostOutputChannels[0];
    framesToZero = PA_MIN_( bp->hostOutputFrameCount[0], frameCount );

    for( i=0; i<bp->outputChannelCount; ++i )
    {
        bp->outputZeroer(   hostOutputChannels[i].data,
                            hostOutputChannels[i].stride,
                            framesToZero );


        /* advance dest ptr for next iteration */
        hostOutputChannels[i].data = ((unsigned char*)hostOutputChannels[i].data) +
                framesToZero * hostOutputChannels[i].stride * bp->bytesPerHostOutputSample;
    }

    bp->hostOutputFrameCount[0] += framesToZero;
    
    return framesToZero;
}
