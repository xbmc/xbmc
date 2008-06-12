/*
 * $Id: pa_jack.c 1306 2007-11-27 19:33:53Z aknudsen $
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 * JACK Implementation by Joshua Haberman
 *
 * Copyright (c) 2004 Stefan Westerfeld <stefan@space.twc.de>
 * Copyright (c) 2004 Arve Knudsen <aknuds-1@broadpark.no>
 * Copyright (c) 2002 Joshua Haberman <joshua@haberman.com>
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

#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>  /* EBUSY */
#include <signal.h> /* sig_atomic_t */
#include <math.h>
#include <semaphore.h>

#include <jack/types.h>
#include <jack/jack.h>

#include "pa_util.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_process.h"
#include "pa_allocation.h"
#include "pa_cpuload.h"
#include "pa_ringbuffer.h"
#include "pa_debugprint.h"

static int aErr_;
static PaError paErr_;     /* For use with ENSURE_PA */
static pthread_t mainThread_;
static char *jackErr_ = NULL;
static const char* clientName_ = "PortAudio";

#define STRINGIZE_HELPER(expr) #expr
#define STRINGIZE(expr) STRINGIZE_HELPER(expr)

/* Check PaError */
#define ENSURE_PA(expr) \
    do { \
        if( (paErr_ = (expr)) < paNoError ) \
        { \
            if( (paErr_) == paUnanticipatedHostError && pthread_self() == mainThread_ ) \
            { \
                if (! jackErr_ ) jackErr_ = "unknown error";\
                PaUtil_SetLastHostErrorInfo( paJACK, -1, jackErr_ ); \
            } \
            PaUtil_DebugPrint(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            result = paErr_; \
            goto error; \
        } \
    } while( 0 )

#define UNLESS(expr, code) \
    do { \
        if( (expr) == 0 ) \
        { \
            if( (code) == paUnanticipatedHostError && pthread_self() == mainThread_ ) \
            { \
                if (!jackErr_) jackErr_ = "unknown error";\
                PaUtil_SetLastHostErrorInfo( paJACK, -1, jackErr_ ); \
            } \
            PaUtil_DebugPrint(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            result = (code); \
            goto error; \
        } \
    } while( 0 )

#define ASSERT_CALL(expr, success) \
    aErr_ = (expr); \
    assert( aErr_ == success );

/*
 * Functions that directly map to the PortAudio stream interface
 */

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
/*static PaTime GetStreamInputLatency( PaStream *stream );*/
/*static PaTime GetStreamOutputLatency( PaStream *stream );*/
static PaTime GetStreamTime( PaStream *stream );
static double GetStreamCpuLoad( PaStream* stream );


/*
 * Data specific to this API
 */

struct PaJackStream;

typedef struct
{
    PaUtilHostApiRepresentation commonHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;

    PaUtilAllocationGroup *deviceInfoMemory;

    jack_client_t *jack_client;
    int jack_buffer_size;
    PaHostApiIndex hostApiIndex;

    pthread_mutex_t mtx;
    pthread_cond_t cond;
    unsigned long inputBase, outputBase;

    /* For dealing with the process thread */
    volatile int xrun;     /* Received xrun notification from JACK? */
    struct PaJackStream * volatile toAdd, * volatile toRemove;
    struct PaJackStream *processQueue;
    volatile sig_atomic_t jackIsDown;
}
PaJackHostApiRepresentation;

/* PaJackStream - a stream data structure specifically for this implementation */

typedef struct PaJackStream
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilBufferProcessor bufferProcessor;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaJackHostApiRepresentation *hostApi;

    /* our input and output ports */
    jack_port_t **local_input_ports;
    jack_port_t **local_output_ports;

    /* the input and output ports of the client we are connecting to */
    jack_port_t **remote_input_ports;
    jack_port_t **remote_output_ports;

    int num_incoming_connections;
    int num_outgoing_connections;

    jack_client_t *jack_client;

    /* The stream is running if it's still producing samples.
     * The stream is active if samples it produced are still being heard.
     */
    volatile sig_atomic_t is_running;
    volatile sig_atomic_t is_active;
    /* Used to signal processing thread that stream should start or stop, respectively */
    volatile sig_atomic_t doStart, doStop, doAbort;

    jack_nframes_t t0;

    PaUtilAllocationGroup *stream_memory;

    /* These are useful in the process callback */

    int callbackResult;
    int isSilenced;
    int xrun;

    /* These are useful for the blocking API */

    int                     isBlockingStream;
    PaUtilRingBuffer        inFIFO;
    PaUtilRingBuffer        outFIFO;
    volatile sig_atomic_t   data_available;
    sem_t                   data_semaphore;
    int                     bytesPerFrame;
    int                     samplesPerFrame;

    struct PaJackStream *next;
}
PaJackStream;

#define TRUE 1
#define FALSE 0

/*
 * Functions specific to this API
 */

static int JackCallback( jack_nframes_t frames, void *userData );


/*
 *
 * Implementation
 *
 */

/* ---- blocking emulation layer ---- */

/* Allocate buffer. */
static PaError BlockingInitFIFO( PaUtilRingBuffer *rbuf, long numFrames, long bytesPerFrame )
{
    long numBytes = numFrames * bytesPerFrame;
    char *buffer = (char *) malloc( numBytes );
    if( buffer == NULL ) return paInsufficientMemory;
    memset( buffer, 0, numBytes );
    return (PaError) PaUtil_InitializeRingBuffer( rbuf, numBytes, buffer );
}

/* Free buffer. */
static PaError BlockingTermFIFO( PaUtilRingBuffer *rbuf )
{
    if( rbuf->buffer ) free( rbuf->buffer );
    rbuf->buffer = NULL;
    return paNoError;
}

static int
BlockingCallback( const void                      *inputBuffer,
                  void                            *outputBuffer,
		  unsigned long                    framesPerBuffer,
		  const PaStreamCallbackTimeInfo*  timeInfo,
		  PaStreamCallbackFlags            statusFlags,
		  void                             *userData )
{
    struct PaJackStream *stream = (PaJackStream *)userData;
    long numBytes = stream->bytesPerFrame * framesPerBuffer;

    /* This may get called with NULL inputBuffer during initial setup. */
    if( inputBuffer != NULL )
    {
        PaUtil_WriteRingBuffer( &stream->inFIFO, inputBuffer, numBytes );
    }
    if( outputBuffer != NULL )
    {
        int numRead = PaUtil_ReadRingBuffer( &stream->outFIFO, outputBuffer, numBytes );
        /* Zero out remainder of buffer if we run out of data. */
        memset( (char *)outputBuffer + numRead, 0, numBytes - numRead );
    }

    if( !stream->data_available )
    {
        stream->data_available = 1;
        sem_post( &stream->data_semaphore );
    }
    return paContinue;
}

static PaError
BlockingBegin( PaJackStream *stream, int minimum_buffer_size )
{
    long    doRead = 0;
    long    doWrite = 0;
    PaError result = paNoError;
    long    numFrames;

    doRead = stream->local_input_ports != NULL;
    doWrite = stream->local_output_ports != NULL;
    /* <FIXME> */
    stream->samplesPerFrame = 2;
    stream->bytesPerFrame = sizeof(float) * stream->samplesPerFrame;
    /* </FIXME> */
    numFrames = 32;
    while (numFrames < minimum_buffer_size)
        numFrames *= 2;

    if( doRead )
    {
        ENSURE_PA( BlockingInitFIFO( &stream->inFIFO, numFrames, stream->bytesPerFrame ) );
    }
    if( doWrite )
    {
        long numBytes;

        ENSURE_PA( BlockingInitFIFO( &stream->outFIFO, numFrames, stream->bytesPerFrame ) );

        /* Make Write FIFO appear full initially. */
        numBytes = PaUtil_GetRingBufferWriteAvailable( &stream->outFIFO );
        PaUtil_AdvanceRingBufferWriteIndex( &stream->outFIFO, numBytes );
    }

    stream->data_available = 0;
    sem_init( &stream->data_semaphore, 0, 0 );

error:
    return result;
}

static void
BlockingEnd( PaJackStream *stream )
{
    BlockingTermFIFO( &stream->inFIFO );
    BlockingTermFIFO( &stream->outFIFO );

    sem_destroy( &stream->data_semaphore );
}

static PaError BlockingReadStream( PaStream* s, void *data, unsigned long numFrames )
{
    PaError result = paNoError;
    PaJackStream *stream = (PaJackStream *)s;

    long bytesRead;
    char *p = (char *) data;
    long numBytes = stream->bytesPerFrame * numFrames;
    while( numBytes > 0 )
    {
        bytesRead = PaUtil_ReadRingBuffer( &stream->inFIFO, p, numBytes );
        numBytes -= bytesRead;
        p += bytesRead;
        if( numBytes > 0 )
        {
            /* see write for an explanation */
            if( stream->data_available )
                stream->data_available = 0;
            else
                sem_wait( &stream->data_semaphore );
        }
    }

    return result;
}

static PaError BlockingWriteStream( PaStream* s, const void *data, unsigned long numFrames )
{
    PaError result = paNoError;
    PaJackStream *stream = (PaJackStream *)s;
    long bytesWritten;
    char *p = (char *) data;
    long numBytes = stream->bytesPerFrame * numFrames;
    while( numBytes > 0 )
    {
        bytesWritten = PaUtil_WriteRingBuffer( &stream->outFIFO, p, numBytes );
        numBytes -= bytesWritten;
        p += bytesWritten;
        if( numBytes > 0 )
        {
            /* we use the following algorithm:
             *   (1) write data
             *   (2) if some data didn't fit into the ringbuffer, set data_available to 0
             *       to indicate to the audio that if space becomes available, we want to know
             *   (3) retry to write data (because it might be that between (1) and (2)
             *       new space in the buffer became available)
             *   (4) if this failed, we are sure that the buffer is really empty and
             *       we will definitely receive a notification when it becomes available
             *       thus we can safely sleep
             *
             * if the algorithm bailed out in step (3) before, it leaks a count of 1
             * on the semaphore; however, it doesn't matter, because if we block in (4),
             * we also do it in a loop
             */
            if( stream->data_available )
                stream->data_available = 0;
            else
                sem_wait( &stream->data_semaphore );
        }
    }

    return result;
}

static signed long
BlockingGetStreamReadAvailable( PaStream* s )
{
    PaJackStream *stream = (PaJackStream *)s;

    int bytesFull = PaUtil_GetRingBufferReadAvailable( &stream->inFIFO );
    return bytesFull / stream->bytesPerFrame;
}

static signed long
BlockingGetStreamWriteAvailable( PaStream* s )
{
    PaJackStream *stream = (PaJackStream *)s;

    int bytesEmpty = PaUtil_GetRingBufferWriteAvailable( &stream->outFIFO );
    return bytesEmpty / stream->bytesPerFrame;
}

static PaError
BlockingWaitEmpty( PaStream *s )
{
    PaJackStream *stream = (PaJackStream *)s;

    while( PaUtil_GetRingBufferReadAvailable( &stream->outFIFO ) > 0 )
    {
        stream->data_available = 0;
        sem_wait( &stream->data_semaphore );
    }
    return 0;
}

/* ---- jack driver ---- */

/* BuildDeviceList():
 *
 * The process of determining a list of PortAudio "devices" from
 * JACK's client/port system is fairly involved, so it is separated
 * into its own routine.
 */

static PaError BuildDeviceList( PaJackHostApiRepresentation *jackApi )
{
    /* Utility macros for the repetitive process of allocating memory */

    /* JACK has no concept of a device.  To JACK, there are clients
     * which have an arbitrary number of ports.  To make this
     * intelligible to PortAudio clients, we will group each JACK client
     * into a device, and make each port of that client a channel */

    PaError result = paNoError;
    PaUtilHostApiRepresentation *commonApi = &jackApi->commonHostApiRep;

    const char **jack_ports = NULL;
    char **client_names = NULL;
    char *regex_pattern = NULL;
    int port_index, client_index, i;
    double globalSampleRate;
    regex_t port_regex;
    unsigned long numClients = 0, numPorts = 0;
    char *tmp_client_name = NULL;

    commonApi->info.defaultInputDevice = paNoDevice;
    commonApi->info.defaultOutputDevice = paNoDevice;
    commonApi->info.deviceCount = 0;

    /* Parse the list of ports, using a regex to grab the client names */
    ASSERT_CALL( regcomp( &port_regex, "^[^:]*", REG_EXTENDED ), 0 );

    /* since we are rebuilding the list of devices, free all memory
     * associated with the previous list */
    PaUtil_FreeAllAllocations( jackApi->deviceInfoMemory );

    regex_pattern = PaUtil_GroupAllocateMemory( jackApi->deviceInfoMemory, jack_client_name_size() + 3 );
    tmp_client_name = PaUtil_GroupAllocateMemory( jackApi->deviceInfoMemory, jack_client_name_size() );

    /* We can only retrieve the list of clients indirectly, by first
     * asking for a list of all ports, then parsing the port names
     * according to the client_name:port_name convention (which is
     * enforced by jackd)
     * A: If jack_get_ports returns NULL, there's nothing for us to do */
    UNLESS( (jack_ports = jack_get_ports( jackApi->jack_client, "", "", 0 )) && jack_ports[0], paNoError );
    /* Find number of ports */
    while( jack_ports[numPorts] )
        ++numPorts;
    /* At least there will be one port per client :) */
    UNLESS( client_names = PaUtil_GroupAllocateMemory( jackApi->deviceInfoMemory, numPorts *
                sizeof (char *) ), paInsufficientMemory );

    /* Build a list of clients from the list of ports */
    for( numClients = 0, port_index = 0; jack_ports[port_index] != NULL; port_index++ )
    {
        int client_seen = FALSE;
        regmatch_t match_info;
        const char *port = jack_ports[port_index];

        /* extract the client name from the port name, using a regex
         * that parses the clientname:portname syntax */
        UNLESS( !regexec( &port_regex, port, 1, &match_info, 0 ), paInternalError );
        assert(match_info.rm_eo - match_info.rm_so < jack_client_name_size());
        memcpy( tmp_client_name, port + match_info.rm_so,
                match_info.rm_eo - match_info.rm_so );
        tmp_client_name[match_info.rm_eo - match_info.rm_so] = '\0';

        /* do we know about this port's client yet? */
        for( i = 0; i < numClients; i++ )
        {
            if( strcmp( tmp_client_name, client_names[i] ) == 0 )
                client_seen = TRUE;
        }

        if (client_seen)
            continue;   /* A: Nothing to see here, move along */

        UNLESS( client_names[numClients] = (char*)PaUtil_GroupAllocateMemory( jackApi->deviceInfoMemory,
                    strlen(tmp_client_name) + 1), paInsufficientMemory );

        /* The alsa_pcm client should go in spot 0.  If this
         * is the alsa_pcm client AND we are NOT about to put
         * it in spot 0 put it in spot 0 and move whatever
         * was already in spot 0 to the end. */
        if( strcmp( "alsa_pcm", tmp_client_name ) == 0 && numClients > 0 )
        {
            /* alsa_pcm goes in spot 0 */
            strcpy( client_names[ numClients ], client_names[0] );
            strcpy( client_names[0], tmp_client_name );
        }
        else
        {
            /* put the new client at the end of the client list */
            strcpy( client_names[ numClients ], tmp_client_name );
        }
        ++numClients;
    }

    /* Now we have a list of clients, which will become the list of
     * PortAudio devices. */

    /* there is one global sample rate all clients must conform to */

    globalSampleRate = jack_get_sample_rate( jackApi->jack_client );
    UNLESS( commonApi->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory( jackApi->deviceInfoMemory,
                sizeof(PaDeviceInfo*) * numClients ), paInsufficientMemory );

    assert( commonApi->info.deviceCount == 0 );

    /* Create a PaDeviceInfo structure for every client */
    for( client_index = 0; client_index < numClients; client_index++ )
    {
        PaDeviceInfo *curDevInfo;
        const char **clientPorts = NULL;

        UNLESS( curDevInfo = (PaDeviceInfo*)PaUtil_GroupAllocateMemory( jackApi->deviceInfoMemory,
                    sizeof(PaDeviceInfo) ), paInsufficientMemory );
        UNLESS( curDevInfo->name = (char*)PaUtil_GroupAllocateMemory( jackApi->deviceInfoMemory,
                    strlen(client_names[client_index]) + 1 ), paInsufficientMemory );
        strcpy( (char *)curDevInfo->name, client_names[client_index] );

        curDevInfo->structVersion = 2;
        curDevInfo->hostApi = jackApi->hostApiIndex;

        /* JACK is very inflexible: there is one sample rate the whole
         * system must run at, and all clients must speak IEEE float. */
        curDevInfo->defaultSampleRate = globalSampleRate;

        /* To determine how many input and output channels are available,
         * we re-query jackd with more specific parameters. */

        sprintf( regex_pattern, "%s:.*", client_names[client_index] );

        /* ... what are your output ports (that we could input from)? */
        clientPorts = jack_get_ports( jackApi->jack_client, regex_pattern,
                                     NULL, JackPortIsOutput);
        curDevInfo->maxInputChannels = 0;
        curDevInfo->defaultLowInputLatency = 0.;
        curDevInfo->defaultHighInputLatency = 0.;
        if( clientPorts )
        {
            jack_port_t *p = jack_port_by_name( jackApi->jack_client, clientPorts[0] );
            curDevInfo->defaultLowInputLatency = curDevInfo->defaultHighInputLatency =
                jack_port_get_latency( p ) / globalSampleRate;

            for( i = 0; clientPorts[i] != NULL; i++)
            {
                /* The number of ports returned is the number of output channels.
                 * We don't care what they are, we just care how many */
                curDevInfo->maxInputChannels++;
            }
            free(clientPorts);
        }

        /* ... what are your input ports (that we could output to)? */
        clientPorts = jack_get_ports( jackApi->jack_client, regex_pattern,
                                     NULL, JackPortIsInput);
        curDevInfo->maxOutputChannels = 0;
        curDevInfo->defaultLowOutputLatency = 0.;
        curDevInfo->defaultHighOutputLatency = 0.;
        if( clientPorts )
        {
            jack_port_t *p = jack_port_by_name( jackApi->jack_client, clientPorts[0] );
            curDevInfo->defaultLowOutputLatency = curDevInfo->defaultHighOutputLatency =
                jack_port_get_latency( p ) / globalSampleRate;

            for( i = 0; clientPorts[i] != NULL; i++)
            {
                /* The number of ports returned is the number of input channels.
                 * We don't care what they are, we just care how many */
                curDevInfo->maxOutputChannels++;
            }
            free(clientPorts);
        }

        /* Add this client to the list of devices */
        commonApi->deviceInfos[client_index] = curDevInfo;
        ++commonApi->info.deviceCount;
        if( commonApi->info.defaultInputDevice == paNoDevice && curDevInfo->maxInputChannels > 0 )
            commonApi->info.defaultInputDevice = client_index;
        if( commonApi->info.defaultOutputDevice == paNoDevice && curDevInfo->maxOutputChannels > 0 )
            commonApi->info.defaultOutputDevice = client_index;
    }

error:
    regfree( &port_regex );
    free( jack_ports );
    return result;
}

static void UpdateSampleRate( PaJackStream *stream, double sampleRate )
{
    /* XXX: Maybe not the cleanest way of going about this? */
    stream->cpuLoadMeasurer.samplingPeriod = stream->bufferProcessor.samplePeriod = 1. / sampleRate;
    stream->streamRepresentation.streamInfo.sampleRate = sampleRate;
}

static void JackErrorCallback( const char *msg )
{
    if( pthread_self() == mainThread_ )
    {
        assert( msg );
        jackErr_ = realloc( jackErr_, strlen( msg ) + 1 );
        strcpy( jackErr_, msg );
    }
}

static void JackOnShutdown( void *arg )
{
    PaJackHostApiRepresentation *jackApi = (PaJackHostApiRepresentation *)arg;
    PaJackStream *stream = jackApi->processQueue;

    PA_DEBUG(( "%s: JACK server is shutting down\n", __FUNCTION__ ));
    for( ; stream; stream = stream->next )
    {
        stream->is_active = 0;
    }

    /* Make sure that the main thread doesn't get stuck waiting on the condition */
    ASSERT_CALL( pthread_mutex_lock( &jackApi->mtx ), 0 );
    jackApi->jackIsDown = 1;
    ASSERT_CALL( pthread_cond_signal( &jackApi->cond ), 0 );
    ASSERT_CALL( pthread_mutex_unlock( &jackApi->mtx ), 0 );

}

static int JackSrCb( jack_nframes_t nframes, void *arg )
{
    PaJackHostApiRepresentation *jackApi = (PaJackHostApiRepresentation *)arg;
    double sampleRate = (double)nframes;
    PaJackStream *stream = jackApi->processQueue;

    /* Update all streams in process queue */
    PA_DEBUG(( "%s: Acting on change in JACK samplerate: %f\n", __FUNCTION__, sampleRate ));
    for( ; stream; stream = stream->next )
    {
        if( stream->streamRepresentation.streamInfo.sampleRate != sampleRate )
        {
            PA_DEBUG(( "%s: Updating samplerate\n", __FUNCTION__ ));
            UpdateSampleRate( stream, sampleRate );
        }
    }

    return 0;
}

static int JackXRunCb(void *arg) {
    PaJackHostApiRepresentation *hostApi = (PaJackHostApiRepresentation *)arg;
    assert( hostApi );
    hostApi->xrun = TRUE;
    PA_DEBUG(( "%s: JACK signalled xrun\n", __FUNCTION__ ));
    return 0;
}

PaError PaJack_Initialize( PaUtilHostApiRepresentation **hostApi,
                           PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    PaJackHostApiRepresentation *jackHostApi;
    int activated = 0;
    jack_status_t jackStatus = 0;
    *hostApi = NULL;    /* Initialize to NULL */

    UNLESS( jackHostApi = (PaJackHostApiRepresentation*)
        PaUtil_AllocateMemory( sizeof(PaJackHostApiRepresentation) ), paInsufficientMemory );
    UNLESS( jackHostApi->deviceInfoMemory = PaUtil_CreateAllocationGroup(), paInsufficientMemory );

    mainThread_ = pthread_self();
    ASSERT_CALL( pthread_mutex_init( &jackHostApi->mtx, NULL ), 0 );
    ASSERT_CALL( pthread_cond_init( &jackHostApi->cond, NULL ), 0 );

    /* Try to become a client of the JACK server.  If we cannot do
     * this, then this API cannot be used.
     *
     * Without the JackNoStartServer option, the jackd server is started
     * automatically which we do not want.
     */

    jackHostApi->jack_client = jack_client_open( clientName_, JackNoStartServer, &jackStatus );
    if( !jackHostApi->jack_client )
    {
        /* the V19 development docs say that if an implementation
         * detects that it cannot be used, it should return a NULL
         * interface and paNoError */
        PA_DEBUG(( "%s: Couldn't connect to JACK, status: %d\n", __FUNCTION__, jackStatus ));
        result = paNoError;
        goto error;
    }

    jackHostApi->hostApiIndex = hostApiIndex;

    *hostApi = &jackHostApi->commonHostApiRep;
    (*hostApi)->info.structVersion = 1;
    (*hostApi)->info.type = paJACK;
    (*hostApi)->info.name = "JACK Audio Connection Kit";

    /* Build a device list by querying the JACK server */
    ENSURE_PA( BuildDeviceList( jackHostApi ) );

    /* Register functions */

    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &jackHostApi->callbackStreamInterface,
                                      CloseStream, StartStream,
                                      StopStream, AbortStream,
                                      IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable,
                                      PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &jackHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      BlockingReadStream, BlockingWriteStream,
                                      BlockingGetStreamReadAvailable, BlockingGetStreamWriteAvailable );

    jackHostApi->inputBase = jackHostApi->outputBase = 0;
    jackHostApi->xrun = 0;
    jackHostApi->toAdd = jackHostApi->toRemove = NULL;
    jackHostApi->processQueue = NULL;
    jackHostApi->jackIsDown = 0;

    jack_on_shutdown( jackHostApi->jack_client, JackOnShutdown, jackHostApi );
    jack_set_error_function( JackErrorCallback );
    jackHostApi->jack_buffer_size = jack_get_buffer_size ( jackHostApi->jack_client );
    /* Don't check for error, may not be supported (deprecated in at least jackdmp) */
    jack_set_sample_rate_callback( jackHostApi->jack_client, JackSrCb, jackHostApi );
    UNLESS( !jack_set_xrun_callback( jackHostApi->jack_client, JackXRunCb, jackHostApi ), paUnanticipatedHostError );
    UNLESS( !jack_set_process_callback( jackHostApi->jack_client, JackCallback, jackHostApi ), paUnanticipatedHostError );
    UNLESS( !jack_activate( jackHostApi->jack_client ), paUnanticipatedHostError );
    activated = 1;

    return result;

error:
    if( activated )
        ASSERT_CALL( jack_deactivate( jackHostApi->jack_client ), 0 );

    if( jackHostApi )
    {
        if( jackHostApi->jack_client )
            ASSERT_CALL( jack_client_close( jackHostApi->jack_client ), 0 );

        if( jackHostApi->deviceInfoMemory )
        {
            PaUtil_FreeAllAllocations( jackHostApi->deviceInfoMemory );
            PaUtil_DestroyAllocationGroup( jackHostApi->deviceInfoMemory );
        }

        PaUtil_FreeMemory( jackHostApi );
    }
    return result;
}


static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaJackHostApiRepresentation *jackHostApi = (PaJackHostApiRepresentation*)hostApi;

    /* note: this automatically disconnects all ports, since a deactivated
     * client is not allowed to have any ports connected */
    ASSERT_CALL( jack_deactivate( jackHostApi->jack_client ), 0 );

    ASSERT_CALL( pthread_mutex_destroy( &jackHostApi->mtx ), 0 );
    ASSERT_CALL( pthread_cond_destroy( &jackHostApi->cond ), 0 );

    ASSERT_CALL( jack_client_close( jackHostApi->jack_client ), 0 );

    if( jackHostApi->deviceInfoMemory )
    {
        PaUtil_FreeAllAllocations( jackHostApi->deviceInfoMemory );
        PaUtil_DestroyAllocationGroup( jackHostApi->deviceInfoMemory );
    }

    PaUtil_FreeMemory( jackHostApi );

    free( jackErr_ );
}

static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    int inputChannelCount = 0, outputChannelCount = 0;
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

    /*
        The following check is not necessary for JACK.

            - if a full duplex stream is requested, check that the combination
                of input and output parameters is supported


        Because the buffer adapter handles conversion between all standard
        sample formats, the following checks are only required if paCustomFormat
        is implemented, or under some other unusual conditions.

            - check that input device can support inputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format

            - check that output device can support outputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format
    */

    /* check that the device supports sampleRate */

#define ABS(x) ( (x) > 0 ? (x) : -(x) )
    if( ABS(sampleRate - jack_get_sample_rate(((PaJackHostApiRepresentation *) hostApi)->jack_client )) > 1 )
       return paInvalidSampleRate;
#undef ABS

    return paFormatIsSupported;
}

/* Basic stream initialization */
static PaError InitializeStream( PaJackStream *stream, PaJackHostApiRepresentation *hostApi, int numInputChannels,
        int numOutputChannels )
{
    PaError result = paNoError;
    assert( stream );

    memset( stream, 0, sizeof (PaJackStream) );
    UNLESS( stream->stream_memory = PaUtil_CreateAllocationGroup(), paInsufficientMemory );
    stream->jack_client = hostApi->jack_client;
    stream->hostApi = hostApi;

    if( numInputChannels > 0 )
    {
        UNLESS( stream->local_input_ports =
                (jack_port_t**) PaUtil_GroupAllocateMemory( stream->stream_memory, sizeof(jack_port_t*) * numInputChannels ),
                paInsufficientMemory );
        memset( stream->local_input_ports, 0, sizeof(jack_port_t*) * numInputChannels );
        UNLESS( stream->remote_output_ports =
                (jack_port_t**) PaUtil_GroupAllocateMemory( stream->stream_memory, sizeof(jack_port_t*) * numInputChannels ),
                paInsufficientMemory );
        memset( stream->remote_output_ports, 0, sizeof(jack_port_t*) * numInputChannels );
    }
    if( numOutputChannels > 0 )
    {
        UNLESS( stream->local_output_ports =
                (jack_port_t**) PaUtil_GroupAllocateMemory( stream->stream_memory, sizeof(jack_port_t*) * numOutputChannels ),
                paInsufficientMemory );
        memset( stream->local_output_ports, 0, sizeof(jack_port_t*) * numOutputChannels );
        UNLESS( stream->remote_input_ports =
                (jack_port_t**) PaUtil_GroupAllocateMemory( stream->stream_memory, sizeof(jack_port_t*) * numOutputChannels ),
                paInsufficientMemory );
        memset( stream->remote_input_ports, 0, sizeof(jack_port_t*) * numOutputChannels );
    }

    stream->num_incoming_connections = numInputChannels;
    stream->num_outgoing_connections = numOutputChannels;

error:
    return result;
}

/*!
 * Free resources associated with stream, and eventually stream itself.
 *
 * Frees allocated memory, and closes opened pcms.
 */
static void CleanUpStream( PaJackStream *stream, int terminateStreamRepresentation, int terminateBufferProcessor )
{
    int i;
    assert( stream );

    if( stream->isBlockingStream )
        BlockingEnd( stream );

    for( i = 0; i < stream->num_incoming_connections; ++i )
    {
        if( stream->local_input_ports[i] )
            ASSERT_CALL( jack_port_unregister( stream->jack_client, stream->local_input_ports[i] ), 0 );
    }
    for( i = 0; i < stream->num_outgoing_connections; ++i )
    {
        if( stream->local_output_ports[i] )
            ASSERT_CALL( jack_port_unregister( stream->jack_client, stream->local_output_ports[i] ), 0 );
    }

    if( terminateStreamRepresentation )
        PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );
    if( terminateBufferProcessor )
        PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );

    if( stream->stream_memory )
    {
        PaUtil_FreeAllAllocations( stream->stream_memory );
        PaUtil_DestroyAllocationGroup( stream->stream_memory );
    }
    PaUtil_FreeMemory( stream );
}

static PaError WaitCondition( PaJackHostApiRepresentation *hostApi )
{
    PaError result = paNoError;
    int err = 0;
    PaTime pt = PaUtil_GetTime();
    struct timespec ts;

    ts.tv_sec = (time_t) floor( pt + 1 );
    ts.tv_nsec = (long) ((pt - floor( pt )) * 1000000000);
    /* XXX: Best enclose in loop, in case of spurious wakeups? */
    err = pthread_cond_timedwait( &hostApi->cond, &hostApi->mtx, &ts );

    /* Make sure we didn't time out */
    UNLESS( err != ETIMEDOUT, paTimedOut );
    UNLESS( !err, paInternalError );

error:
    return result;
}

static PaError AddStream( PaJackStream *stream )
{
    PaError result = paNoError;
    PaJackHostApiRepresentation *hostApi = stream->hostApi;
    /* Add to queue of streams that should be processed */
    ASSERT_CALL( pthread_mutex_lock( &hostApi->mtx ), 0 );
    if( !hostApi->jackIsDown )
    {
        hostApi->toAdd = stream;
        /* Unlock mutex and await signal from processing thread */
        result = WaitCondition( stream->hostApi );
    }
    ASSERT_CALL( pthread_mutex_unlock( &hostApi->mtx ), 0 );
    ENSURE_PA( result );

    UNLESS( !hostApi->jackIsDown, paDeviceUnavailable );

error:
    return result;
}

/* Remove stream from processing queue */
static PaError RemoveStream( PaJackStream *stream )
{
    PaError result = paNoError;
    PaJackHostApiRepresentation *hostApi = stream->hostApi;

    /* Add to queue over streams that should be processed */
    ASSERT_CALL( pthread_mutex_lock( &hostApi->mtx ), 0 );
    if( !hostApi->jackIsDown )
    {
        hostApi->toRemove = stream;
        /* Unlock mutex and await signal from processing thread */
        result = WaitCondition( stream->hostApi );
    }
    ASSERT_CALL( pthread_mutex_unlock( &hostApi->mtx ), 0 );
    ENSURE_PA( result );

error:
    return result;
}

/* Add stream to JACK callback processing queue */
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
    PaJackHostApiRepresentation *jackHostApi = (PaJackHostApiRepresentation*)hostApi;
    PaJackStream *stream = NULL;
    char *port_string = PaUtil_GroupAllocateMemory( jackHostApi->deviceInfoMemory, jack_port_name_size() );
    unsigned long regexSz = jack_client_name_size() + 3;
    char *regex_pattern = PaUtil_GroupAllocateMemory( jackHostApi->deviceInfoMemory, regexSz );
    const char **jack_ports = NULL;
    /* int jack_max_buffer_size = jack_get_buffer_size( jackHostApi->jack_client ); */
    int i;
    int inputChannelCount, outputChannelCount;
    const double jackSr = jack_get_sample_rate( jackHostApi->jack_client );
    PaSampleFormat inputSampleFormat = 0, outputSampleFormat = 0;
    int bpInitialized = 0, srInitialized = 0;   /* Initialized buffer processor and stream representation? */
    unsigned long ofs;

    /* validate platform specific flags */
    if( (streamFlags & paPlatformSpecificFlags) != 0 )
        return paInvalidFlag; /* unexpected platform specific flag */
    if( (streamFlags & paPrimeOutputBuffersUsingStreamCallback) != 0 )
    {
        streamFlags &= ~paPrimeOutputBuffersUsingStreamCallback;
        /*return paInvalidFlag;*/   /* This implementation does not support buffer priming */
    }

    if( framesPerBuffer != paFramesPerBufferUnspecified )
    {
        /* Jack operates with power of two buffers, and we don't support non-integer buffer adaption (yet) */
        /*UNLESS( !(framesPerBuffer & (framesPerBuffer - 1)), paBufferTooBig );*/  /* TODO: Add descriptive error code? */
    }

    /* Preliminary checks */

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

    /* ... check that the sample rate exactly matches the ONE acceptable rate
     * A: This rate isn't necessarily constant though? */

#define ABS(x) ( (x) > 0 ? (x) : -(x) )
    if( ABS(sampleRate - jackSr) > 1 )
       return paInvalidSampleRate;
#undef ABS

    UNLESS( stream = (PaJackStream*)PaUtil_AllocateMemory( sizeof(PaJackStream) ), paInsufficientMemory );
    ENSURE_PA( InitializeStream( stream, jackHostApi, inputChannelCount, outputChannelCount ) );

    /* the blocking emulation, if necessary */
    stream->isBlockingStream = !streamCallback;
    if( stream->isBlockingStream )
    {
        float latency = 0.001; /* 1ms is the absolute minimum we support */
        int   minimum_buffer_frames = 0;

        if( inputParameters && inputParameters->suggestedLatency > latency )
            latency = inputParameters->suggestedLatency;
        else if( outputParameters && outputParameters->suggestedLatency > latency )
            latency = outputParameters->suggestedLatency;

        /* the latency the user asked for indicates the minimum buffer size in frames */
        minimum_buffer_frames = (int) (latency * jack_get_sample_rate( jackHostApi->jack_client ));

        /* we also need to be able to store at least three full jack buffers to avoid dropouts */
        if( jackHostApi->jack_buffer_size * 3 > minimum_buffer_frames )
            minimum_buffer_frames = jackHostApi->jack_buffer_size * 3;

        /* setup blocking API data structures (FIXME: can fail) */
	BlockingBegin( stream, minimum_buffer_frames );

        /* install our own callback for the blocking API */
        streamCallback = BlockingCallback;
        userData = stream;

        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &jackHostApi->blockingStreamInterface, streamCallback, userData );
    }
    else
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &jackHostApi->callbackStreamInterface, streamCallback, userData );
    }
    srInitialized = 1;
    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, jackSr );

    /* create the JACK ports.  We cannot connect them until audio
     * processing begins */

    /* Register a unique set of ports for this stream
     * TODO: Robust allocation of new port names */

    ofs = jackHostApi->inputBase;
    for( i = 0; i < inputChannelCount; i++ )
    {
        snprintf( port_string, jack_port_name_size(), "in_%lu", ofs + i );
        UNLESS( stream->local_input_ports[i] = jack_port_register(
              jackHostApi->jack_client, port_string,
              JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 ), paInsufficientMemory );
    }
    jackHostApi->inputBase += inputChannelCount;

    ofs = jackHostApi->outputBase;
    for( i = 0; i < outputChannelCount; i++ )
    {
        snprintf( port_string, jack_port_name_size(), "out_%lu", ofs + i );
        UNLESS( stream->local_output_ports[i] = jack_port_register(
             jackHostApi->jack_client, port_string,
             JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 ), paInsufficientMemory );
    }
    jackHostApi->outputBase += outputChannelCount;

    /* look up the jack_port_t's for the remote ports.  We could do
     * this at stream start time, but doing it here ensures the
     * name lookup only happens once. */

    if( inputChannelCount > 0 )
    {
        int err = 0;

        /* Get output ports of our capture device */
        snprintf( regex_pattern, regexSz, "%s:.*", hostApi->deviceInfos[ inputParameters->device ]->name );
        UNLESS( jack_ports = jack_get_ports( jackHostApi->jack_client, regex_pattern,
                                     NULL, JackPortIsOutput ), paUnanticipatedHostError );
        for( i = 0; i < inputChannelCount && jack_ports[i]; i++ )
        {
            if( (stream->remote_output_ports[i] = jack_port_by_name(
                 jackHostApi->jack_client, jack_ports[i] )) == NULL )
            {
                err = 1;
                break;
            }
        }
        free( jack_ports );
        UNLESS( !err, paInsufficientMemory );

        /* Fewer ports than expected? */
        UNLESS( i == inputChannelCount, paInternalError );
    }

    if( outputChannelCount > 0 )
    {
        int err = 0;

        /* Get input ports of our playback device */
        snprintf( regex_pattern, regexSz, "%s:.*", hostApi->deviceInfos[ outputParameters->device ]->name );
        UNLESS( jack_ports = jack_get_ports( jackHostApi->jack_client, regex_pattern,
                                     NULL, JackPortIsInput ), paUnanticipatedHostError );
        for( i = 0; i < outputChannelCount && jack_ports[i]; i++ )
        {
            if( (stream->remote_input_ports[i] = jack_port_by_name(
                 jackHostApi->jack_client, jack_ports[i] )) == 0 )
            {
                err = 1;
                break;
            }
        }
        free( jack_ports );
        UNLESS( !err , paInsufficientMemory );

        /* Fewer ports than expected? */
        UNLESS( i == outputChannelCount, paInternalError );
    }

    ENSURE_PA( PaUtil_InitializeBufferProcessor(
                  &stream->bufferProcessor,
                  inputChannelCount,
                  inputSampleFormat,
                  paFloat32,            /* hostInputSampleFormat */
                  outputChannelCount,
                  outputSampleFormat,
                  paFloat32,            /* hostOutputSampleFormat */
                  jackSr,
                  streamFlags,
                  framesPerBuffer,
                  0,                            /* Ignored */
                  paUtilUnknownHostBufferSize,  /* Buffer size may vary on JACK's discretion */
                  streamCallback,
                  userData ) );
    bpInitialized = 1;

    if( stream->num_incoming_connections > 0 )
        stream->streamRepresentation.streamInfo.inputLatency = (jack_port_get_latency( stream->remote_output_ports[0] )
                - jack_get_buffer_size( jackHostApi->jack_client )  /* One buffer is not counted as latency */
            + PaUtil_GetBufferProcessorInputLatency( &stream->bufferProcessor )) / sampleRate;
    if( stream->num_outgoing_connections > 0 )
        stream->streamRepresentation.streamInfo.outputLatency = (jack_port_get_latency( stream->remote_input_ports[0] )
                - jack_get_buffer_size( jackHostApi->jack_client )  /* One buffer is not counted as latency */
            + PaUtil_GetBufferProcessorOutputLatency( &stream->bufferProcessor )) / sampleRate;

    stream->streamRepresentation.streamInfo.sampleRate = jackSr;
    stream->t0 = jack_frame_time( jackHostApi->jack_client );   /* A: Time should run from Pa_OpenStream */

    /* Add to queue of opened streams */
    ENSURE_PA( AddStream( stream ) );

    *s = (PaStream*)stream;

    return result;

error:
    if( stream )
        CleanUpStream( stream, srInitialized, bpInitialized );

    return result;
}

/*
    When CloseStream() is called, the multi-api layer ensures that
    the stream has already been stopped or aborted.
*/
static PaError CloseStream( PaStream* s )
{
    PaError result = paNoError;
    PaJackStream *stream = (PaJackStream*)s;

    /* Remove this stream from the processing queue */
    ENSURE_PA( RemoveStream( stream ) );

error:
    CleanUpStream( stream, 1, 1 );
    return result;
}

static PaError RealProcess( PaJackStream *stream, jack_nframes_t frames )
{
    PaError result = paNoError;
    PaStreamCallbackTimeInfo timeInfo = {0,0,0};
    int chn;
    int framesProcessed;
    const double sr = jack_get_sample_rate( stream->jack_client );    /* Shouldn't change during the process callback */
    PaStreamCallbackFlags cbFlags = 0;

    /* If the user has returned !paContinue from the callback we'll want to flush the internal buffers,
     * when these are empty we can finally mark the stream as inactive */
    if( stream->callbackResult != paContinue &&
            PaUtil_IsBufferProcessorOutputEmpty( &stream->bufferProcessor ) )
    {
        stream->is_active = 0;
        if( stream->streamRepresentation.streamFinishedCallback )
            stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );
        PA_DEBUG(( "%s: Callback finished\n", __FUNCTION__ ));

        goto end;
    }

    timeInfo.currentTime = (jack_frame_time( stream->jack_client ) - stream->t0) / sr;
    if( stream->num_incoming_connections > 0 )
        timeInfo.inputBufferAdcTime = timeInfo.currentTime - jack_port_get_latency( stream->remote_output_ports[0] )
            / sr;
    if( stream->num_outgoing_connections > 0 )
        timeInfo.outputBufferDacTime = timeInfo.currentTime + jack_port_get_latency( stream->remote_input_ports[0] )
            / sr;

    PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );

    if( stream->xrun )
    {
        /* XXX: Any way to tell which of these occurred? */
        cbFlags = paOutputUnderflow | paInputOverflow;
        stream->xrun = FALSE;
    }
    PaUtil_BeginBufferProcessing( &stream->bufferProcessor, &timeInfo,
            cbFlags );

    if( stream->num_incoming_connections > 0 )
        PaUtil_SetInputFrameCount( &stream->bufferProcessor, frames );
    if( stream->num_outgoing_connections > 0 )
        PaUtil_SetOutputFrameCount( &stream->bufferProcessor, frames );

    for( chn = 0; chn < stream->num_incoming_connections; chn++ )
    {
        jack_default_audio_sample_t *channel_buf = (jack_default_audio_sample_t*)
            jack_port_get_buffer( stream->local_input_ports[chn],
                    frames );

        PaUtil_SetNonInterleavedInputChannel( &stream->bufferProcessor,
                chn,
                channel_buf );
    }

    for( chn = 0; chn < stream->num_outgoing_connections; chn++ )
    {
        jack_default_audio_sample_t *channel_buf = (jack_default_audio_sample_t*)
            jack_port_get_buffer( stream->local_output_ports[chn],
                    frames );

        PaUtil_SetNonInterleavedOutputChannel( &stream->bufferProcessor,
                chn,
                channel_buf );
    }

    framesProcessed = PaUtil_EndBufferProcessing( &stream->bufferProcessor,
            &stream->callbackResult );
    /* We've specified a host buffer size mode where every frame should be consumed by the buffer processor */
    assert( framesProcessed == frames );

    PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, framesProcessed );

end:
    return result;
}

/* Update the JACK callback's stream processing queue. */
static PaError UpdateQueue( PaJackHostApiRepresentation *hostApi )
{
    PaError result = paNoError;
    int queueModified = 0;
    const double jackSr = jack_get_sample_rate( hostApi->jack_client );
    int err;

    if( (err = pthread_mutex_trylock( &hostApi->mtx )) != 0 )
    {
        assert( err == EBUSY );
        return paNoError;
    }

    if( hostApi->toAdd )
    {
        if( hostApi->processQueue )
        {
            PaJackStream *node = hostApi->processQueue;
            /* Advance to end of queue */
            while( node->next )
                node = node->next;

            node->next = hostApi->toAdd;
        }
        else
        {
            /* The only queue entry. */
            hostApi->processQueue = (PaJackStream *)hostApi->toAdd;
        }

        /* If necessary, update stream state */
        if( hostApi->toAdd->streamRepresentation.streamInfo.sampleRate != jackSr )
            UpdateSampleRate( hostApi->toAdd, jackSr );

        hostApi->toAdd = NULL;
        queueModified = 1;
    }
    if( hostApi->toRemove )
    {
        int removed = 0;
        PaJackStream *node = hostApi->processQueue, *prev = NULL;
        assert( hostApi->processQueue );

        while( node )
        {
            if( node == hostApi->toRemove )
            {
                if( prev )
                    prev->next = node->next;
                else
                    hostApi->processQueue = (PaJackStream *)node->next;

                removed = 1;
                break;
            }

            prev = node;
            node = node->next;
        }
        UNLESS( removed, paInternalError );
        hostApi->toRemove = NULL;
        PA_DEBUG(( "%s: Removed stream from processing queue\n", __FUNCTION__ ));
        queueModified = 1;
    }

    if( queueModified )
    {
        /* Signal that we've done what was asked of us */
        ASSERT_CALL( pthread_cond_signal( &hostApi->cond ), 0 );
    }

error:
    ASSERT_CALL( pthread_mutex_unlock( &hostApi->mtx ), 0 );

    return result;
}

/* Audio processing callback invoked periodically from JACK. */
static int JackCallback( jack_nframes_t frames, void *userData )
{
    PaError result = paNoError;
    PaJackHostApiRepresentation *hostApi = (PaJackHostApiRepresentation *)userData;
    PaJackStream *stream = NULL;
    int xrun = hostApi->xrun;
    hostApi->xrun = 0;

    assert( hostApi );

    ENSURE_PA( UpdateQueue( hostApi ) );

    /* Process each stream */
    stream = hostApi->processQueue;
    for( ; stream; stream = stream->next )
    {
        if( xrun )  /* Don't override if already set */
            stream->xrun = 1;

        /* See if this stream is to be started */
        if( stream->doStart )
        {
            /* If we can't obtain a lock, we'll try next time */
            int err = pthread_mutex_trylock( &stream->hostApi->mtx );
            if( !err )
            {
                if( stream->doStart )   /* Could potentially change before obtaining the lock */
                {
                    stream->is_active = 1;
                    stream->doStart = 0;
                    PA_DEBUG(( "%s: Starting stream\n", __FUNCTION__ ));
                    ASSERT_CALL( pthread_cond_signal( &stream->hostApi->cond ), 0 );
                    stream->callbackResult = paContinue;
                    stream->isSilenced = 0;
                }

                ASSERT_CALL( pthread_mutex_unlock( &stream->hostApi->mtx ), 0 );
            }
            else
                assert( err == EBUSY );
        }
        else if( stream->doStop || stream->doAbort )    /* Should we stop/abort stream? */
        {
            if( stream->callbackResult == paContinue )     /* Ok, make it stop */
            {
                PA_DEBUG(( "%s: Stopping stream\n", __FUNCTION__ ));
                stream->callbackResult = stream->doStop ? paComplete : paAbort;
            }
        }

        if( stream->is_active )
            ENSURE_PA( RealProcess( stream, frames ) );
        /* If we have just entered inactive state, silence output */
        if( !stream->is_active && !stream->isSilenced )
        {
            int i;

            /* Silence buffer after entering inactive state */
            PA_DEBUG(( "Silencing the output\n" ));
            for( i = 0; i < stream->num_outgoing_connections; ++i )
            {
                jack_default_audio_sample_t *buffer = jack_port_get_buffer( stream->local_output_ports[i], frames );
                memset( buffer, 0, sizeof (jack_default_audio_sample_t) * frames );
            }

            stream->isSilenced = 1;
        }

        if( stream->doStop || stream->doAbort )
        {
            /* See if RealProcess has acted on the request */
            if( !stream->is_active )   /* Ok, signal to the main thread that we've carried out the operation */
            {
                /* If we can't obtain a lock, we'll try next time */
                int err = pthread_mutex_trylock( &stream->hostApi->mtx );
                if( !err )
                {
                    stream->doStop = stream->doAbort = 0;
                    ASSERT_CALL( pthread_cond_signal( &stream->hostApi->cond ), 0 );
                    ASSERT_CALL( pthread_mutex_unlock( &stream->hostApi->mtx ), 0 );
                }
                else
                    assert( err == EBUSY );
            }
        }
    }

    return 0;
error:
    return -1;
}

static PaError StartStream( PaStream *s )
{
    PaError result = paNoError;
    PaJackStream *stream = (PaJackStream*)s;
    int i;

    /* Ready the processor */
    PaUtil_ResetBufferProcessor( &stream->bufferProcessor );

    /* Connect the ports. Note that the ports may already have been connected by someone else in
     * the meantime, in which case JACK returns EEXIST. */

    if( stream->num_incoming_connections > 0 )
    {
        for( i = 0; i < stream->num_incoming_connections; i++ )
        {
            int r = jack_connect( stream->jack_client, jack_port_name( stream->remote_output_ports[i] ),
                    jack_port_name( stream->local_input_ports[i] ) );
           UNLESS( 0 == r || EEXIST == r, paUnanticipatedHostError );
        }
    }

    if( stream->num_outgoing_connections > 0 )
    {
        for( i = 0; i < stream->num_outgoing_connections; i++ )
        {
            int r = jack_connect( stream->jack_client, jack_port_name( stream->local_output_ports[i] ),
                    jack_port_name( stream->remote_input_ports[i] ) );
           UNLESS( 0 == r || EEXIST == r, paUnanticipatedHostError );
        }
    }

    stream->xrun = FALSE;

    /* Enable processing */

    ASSERT_CALL( pthread_mutex_lock( &stream->hostApi->mtx ), 0 );
    stream->doStart = 1;

    /* Wait for stream to be started */
    result = WaitCondition( stream->hostApi );
    /*
    do
    {
        err = pthread_cond_timedwait( &stream->hostApi->cond, &stream->hostApi->mtx, &ts );
    } while( !stream->is_active && !err );
    */
    if( result != paNoError )   /* Something went wrong, call off the stream start */
    {
        stream->doStart = 0;
        stream->is_active = 0;  /* Cancel any processing */
    }
    ASSERT_CALL( pthread_mutex_unlock( &stream->hostApi->mtx ), 0 );

    ENSURE_PA( result );

    stream->is_running = TRUE;
    PA_DEBUG(( "%s: Stream started\n", __FUNCTION__ ));

error:
    return result;
}

static PaError RealStop( PaJackStream *stream, int abort )
{
    PaError result = paNoError;
    int i;

    if( stream->isBlockingStream )
        BlockingWaitEmpty ( stream );

    ASSERT_CALL( pthread_mutex_lock( &stream->hostApi->mtx ), 0 );
    if( abort )
        stream->doAbort = 1;
    else
        stream->doStop = 1;

    /* Wait for stream to be stopped */
    result = WaitCondition( stream->hostApi );
    ASSERT_CALL( pthread_mutex_unlock( &stream->hostApi->mtx ), 0 );
    ENSURE_PA( result );

    UNLESS( !stream->is_active, paInternalError );

    PA_DEBUG(( "%s: Stream stopped\n", __FUNCTION__ ));

error:
    stream->is_running = FALSE;

    /* Disconnect ports belonging to this stream */

    if( !stream->hostApi->jackIsDown )  /* XXX: Well? */
    {
        for( i = 0; i < stream->num_incoming_connections; i++ )
        {
            if( jack_port_connected( stream->local_input_ports[i] ) )
            {
                UNLESS( !jack_port_disconnect( stream->jack_client, stream->local_input_ports[i] ),
                        paUnanticipatedHostError );
            }
        }
        for( i = 0; i < stream->num_outgoing_connections; i++ )
        {
            if( jack_port_connected( stream->local_output_ports[i] ) )
            {
                UNLESS( !jack_port_disconnect( stream->jack_client, stream->local_output_ports[i] ),
                        paUnanticipatedHostError );
            }
        }
    }

    return result;
}

static PaError StopStream( PaStream *s )
{
    assert(s);
    return RealStop( (PaJackStream *)s, 0 );
}

static PaError AbortStream( PaStream *s )
{
    assert(s);
    return RealStop( (PaJackStream *)s, 1 );
}

static PaError IsStreamStopped( PaStream *s )
{
    PaJackStream *stream = (PaJackStream*)s;
    return !stream->is_running;
}


static PaError IsStreamActive( PaStream *s )
{
    PaJackStream *stream = (PaJackStream*)s;
    return stream->is_active;
}


static PaTime GetStreamTime( PaStream *s )
{
    PaJackStream *stream = (PaJackStream*)s;

    /* A: Is this relevant?? --> TODO: what if we're recording-only? */
    return (jack_frame_time( stream->jack_client ) - stream->t0) / (PaTime)jack_get_sample_rate( stream->jack_client );
}


static double GetStreamCpuLoad( PaStream* s )
{
    PaJackStream *stream = (PaJackStream*)s;
    return PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer );
}

PaError PaJack_SetClientName( const char* name )
{
    if( strlen( name ) > jack_client_name_size() )
    {
        /* OK, I don't know any better error code */
        return paInvalidFlag;
    }
    clientName_ = name;
    return paNoError;
}

PaError PaJack_GetClientName(const char** clientName)
{
    PaError result = paNoError;
    PaJackHostApiRepresentation* jackHostApi = NULL;
    PaJackHostApiRepresentation** ref = &jackHostApi;
    ENSURE_PA( PaUtil_GetHostApiRepresentation( (PaUtilHostApiRepresentation**)ref, paJACK ) );
    *clientName = jack_get_client_name( jackHostApi->jack_client );

error:
    return result;
}
