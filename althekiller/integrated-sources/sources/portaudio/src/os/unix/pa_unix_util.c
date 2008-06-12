/*
 * $Id: pa_unix_util.c 1232 2007-06-16 14:49:43Z rossb $
 * Portable Audio I/O Library
 * UNIX platform-specific support functions
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2000 Ross Bencina
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
 @ingroup unix_src
*/
 
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h> /* For memset */
#include <math.h>
#include <errno.h>

#include "pa_util.h"
#include "pa_unix_util.h"
#include "pa_debugprint.h"

/*
   Track memory allocations to avoid leaks.
 */

#if PA_TRACK_MEMORY
static int numAllocations_ = 0;
#endif


void *PaUtil_AllocateMemory( long size )
{
    void *result = malloc( size );

#if PA_TRACK_MEMORY
    if( result != NULL ) numAllocations_ += 1;
#endif
    return result;
}


void PaUtil_FreeMemory( void *block )
{
    if( block != NULL )
    {
        free( block );
#if PA_TRACK_MEMORY
        numAllocations_ -= 1;
#endif

    }
}


int PaUtil_CountCurrentlyAllocatedBlocks( void )
{
#if PA_TRACK_MEMORY
    return numAllocations_;
#else
    return 0;
#endif
}


void Pa_Sleep( long msec )
{
#ifdef HAVE_NANOSLEEP
    struct timespec req = {0}, rem = {0};
    PaTime time = msec / 1.e3;
    req.tv_sec = (time_t)time;
    assert(time - req.tv_sec < 1.0);
    req.tv_nsec = (long)((time - req.tv_sec) * 1.e9);
    nanosleep(&req, &rem);
    /* XXX: Try sleeping the remaining time (contained in rem) if interrupted by a signal? */
#else
    while( msec > 999 )     /* For OpenBSD and IRIX, argument */
        {                   /* to usleep must be < 1000000.   */
        usleep( 999000 );
        msec -= 999;
        }
    usleep( msec * 1000 );
#endif
}

/*            *** NOT USED YET: ***
static int usePerformanceCounter_;
static double microsecondsPerTick_;
*/

void PaUtil_InitializeClock( void )
{
    /* TODO */
}


PaTime PaUtil_GetTime( void )
{
#ifdef HAVE_CLOCK_GETTIME
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (PaTime)(tp.tv_sec + tp.tv_nsec / 1.e9);
#else
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return (PaTime) tv.tv_usec / 1000000. + tv.tv_sec;
#endif
}

PaError PaUtil_InitializeThreading( PaUtilThreading *threading )
{
    (void) paUtilErr_;
    return paNoError;
}

void PaUtil_TerminateThreading( PaUtilThreading *threading )
{
}

PaError PaUtil_StartThreading( PaUtilThreading *threading, void *(*threadRoutine)(void *), void *data )
{
    pthread_create( &threading->callbackThread, NULL, threadRoutine, data );
    return paNoError;
}

PaError PaUtil_CancelThreading( PaUtilThreading *threading, int wait, PaError *exitResult )
{
    PaError result = paNoError;
    void *pret;

    if( exitResult )
        *exitResult = paNoError;

    /* Only kill the thread if it isn't in the process of stopping (flushing adaptation buffers) */
    if( !wait )
        pthread_cancel( threading->callbackThread );   /* XXX: Safe to call this if the thread has exited on its own? */
    pthread_join( threading->callbackThread, &pret );

#ifdef PTHREAD_CANCELED
    if( pret && PTHREAD_CANCELED != pret )
#else
    /* !wait means the thread may have been canceled */
    if( pret && wait )
#endif
    {
        if( exitResult )
            *exitResult = *(PaError *) pret;
        free( pret );
    }

    return result;
}

/* Threading */
/* paUnixMainThread 
 * We have to be a bit careful with defining this global variable,
 * as explained below. */
#ifdef __apple__
/* apple/gcc has a "problem" with global vars and dynamic libs.
   Initializing it seems to fix the problem.
   Described a bit in this thread:
   http://gcc.gnu.org/ml/gcc/2005-06/msg00179.html
*/
pthread_t paUnixMainThread = 0;
#else
/*pthreads are opaque. We don't know that asigning it an int value
  always makes sense, so we don't initialize it unless we have to.*/
pthread_t paUnixMainThread = 0;
#endif

PaError PaUnixThreading_Initialize()
{
    paUnixMainThread = pthread_self();
    return paNoError;
}

static PaError BoostPriority( PaUnixThread* self )
{
    PaError result = paNoError;
    struct sched_param spm = { 0 };
    /* Priority should only matter between contending FIFO threads? */
    spm.sched_priority = 1;

    assert( self );

    if( pthread_setschedparam( self->thread, SCHED_FIFO, &spm ) != 0 )
    {
        PA_UNLESS( errno == EPERM, paInternalError );  /* Lack permission to raise priority */
        PA_DEBUG(( "Failed bumping priority\n" ));
        result = 0;
    }
    else
    {
        result = 1; /* Success */
    }
error:
    return result;
}

PaError PaUnixThread_New( PaUnixThread* self, void* (*threadFunc)( void* ), void* threadArg, PaTime waitForChild,
        int rtSched )
{
    PaError result = paNoError;
    pthread_attr_t attr;
    int started = 0;

    memset( self, 0, sizeof (PaUnixThread) );
    PaUnixMutex_Initialize( &self->mtx );
    PA_ASSERT_CALL( pthread_cond_init( &self->cond, NULL ), 0 );

    self->parentWaiting = 0 != waitForChild;

    /* Spawn thread */

/* Temporarily disabled since we should test during configuration for presence of required mman.h header */
#if 0
#if defined _POSIX_MEMLOCK && (_POSIX_MEMLOCK != -1)
    if( rtSched )
    {
        if( mlockall( MCL_CURRENT | MCL_FUTURE ) < 0 )
        {
            int savedErrno = errno;             /* In case errno gets overwritten */
            assert( savedErrno != EINVAL );     /* Most likely a programmer error */
            PA_UNLESS( (savedErrno == EPERM), paInternalError );
            PA_DEBUG(( "%s: Failed locking memory\n", __FUNCTION__ ));
        }
        else
            PA_DEBUG(( "%s: Successfully locked memory\n", __FUNCTION__ ));
    }
#endif
#endif

    PA_UNLESS( !pthread_attr_init( &attr ), paInternalError );
    /* Priority relative to other processes */
    PA_UNLESS( !pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM ), paInternalError );   

    PA_UNLESS( !pthread_create( &self->thread, &attr, threadFunc, threadArg ), paInternalError );
    started = 1;

    if( rtSched )
    {
#if 0
        if( self->useWatchdog )
        {
            int err;
            struct sched_param wdSpm = { 0 };
            /* Launch watchdog, watchdog sets callback thread priority */
            int prio = PA_MIN( self->rtPrio + 4, sched_get_priority_max( SCHED_FIFO ) );
            wdSpm.sched_priority = prio;

            PA_UNLESS( !pthread_attr_init( &attr ), paInternalError );
            PA_UNLESS( !pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED ), paInternalError );
            PA_UNLESS( !pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM ), paInternalError );
            PA_UNLESS( !pthread_attr_setschedpolicy( &attr, SCHED_FIFO ), paInternalError );
            PA_UNLESS( !pthread_attr_setschedparam( &attr, &wdSpm ), paInternalError );
            if( (err = pthread_create( &self->watchdogThread, &attr, &WatchdogFunc, self )) )
            {
                PA_UNLESS( err == EPERM, paInternalError );
                /* Permission error, go on without realtime privileges */
                PA_DEBUG(( "Failed bumping priority\n" ));
            }
            else
            {
                int policy;
                self->watchdogRunning = 1;
                PA_ENSURE_SYSTEM( pthread_getschedparam( self->watchdogThread, &policy, &wdSpm ), 0 );
                /* Check if priority is right, policy could potentially differ from SCHED_FIFO (but that's alright) */
                if( wdSpm.sched_priority != prio )
                {
                    PA_DEBUG(( "Watchdog priority not set correctly (%d)\n", wdSpm.sched_priority ));
                    PA_ENSURE( paInternalError );
                }
            }
        }
        else
#endif
            PA_ENSURE( BoostPriority( self ) );

        {
            int policy;
            struct sched_param spm;
            pthread_getschedparam(self->thread, &policy, &spm);
        }
    }
    
    if( self->parentWaiting )
    {
        PaTime till;
        struct timespec ts;
        int res = 0;
        PaTime now;

        PA_ENSURE( PaUnixMutex_Lock( &self->mtx ) );

        /* Wait for stream to be started */
        now = PaUtil_GetTime();
        till = now + waitForChild;

        while( self->parentWaiting && !res )
        {
            if( waitForChild > 0 )
            {
                ts.tv_sec = (time_t) floor( till );
                ts.tv_nsec = (long) ((till - floor( till )) * 1e9);
                res = pthread_cond_timedwait( &self->cond, &self->mtx.mtx, &ts );
            }
            else
            {
                res = pthread_cond_wait( &self->cond, &self->mtx.mtx );
            }
        }

        PA_ENSURE( PaUnixMutex_Unlock( &self->mtx ) );

        PA_UNLESS( !res || ETIMEDOUT == res, paInternalError );
        PA_DEBUG(( "%s: Waited for %g seconds for stream to start\n", __FUNCTION__, PaUtil_GetTime() - now ));
        if( ETIMEDOUT == res )
        {
            PA_ENSURE( paTimedOut );
        }
    }

end:
    return result;
error:
    if( started )
    {
        PaUnixThread_Terminate( self, 0, NULL );
    }

    goto end;
}

PaError PaUnixThread_Terminate( PaUnixThread* self, int wait, PaError* exitResult )
{
    PaError result = paNoError;
    void* pret;

    if( exitResult )
    {
        *exitResult = paNoError;
    }
#if 0
    if( watchdogExitResult )
        *watchdogExitResult = paNoError;

    if( th->watchdogRunning )
    {
        pthread_cancel( th->watchdogThread );
        PA_ENSURE_SYSTEM( pthread_join( th->watchdogThread, &pret ), 0 );

        if( pret && pret != PTHREAD_CANCELED )
        {
            if( watchdogExitResult )
                *watchdogExitResult = *(PaError *) pret;
            free( pret );
        }
    }
#endif

    /* Only kill the thread if it isn't in the process of stopping (flushing adaptation buffers) */
    /* TODO: Make join time out */
    self->stopRequested = wait;
    if( !wait )
    {
        PA_DEBUG(( "%s: Canceling thread %d\n", __FUNCTION__, self->thread ));
        /* XXX: Safe to call this if the thread has exited on its own? */
        pthread_cancel( self->thread );
    }
    PA_DEBUG(( "%s: Joining thread %d\n", __FUNCTION__, self->thread ));
    PA_ENSURE_SYSTEM( pthread_join( self->thread, &pret ), 0 );

    if( pret && PTHREAD_CANCELED != pret )
    {
        if( exitResult )
        {
            *exitResult = *(PaError*)pret;
        }
        free( pret );
    }

error:
    PA_ASSERT_CALL( PaUnixMutex_Terminate( &self->mtx ), paNoError );
    PA_ASSERT_CALL( pthread_cond_destroy( &self->cond ), 0 );

    return result;
}

PaError PaUnixThread_PrepareNotify( PaUnixThread* self )
{
    PaError result = paNoError;
    PA_UNLESS( self->parentWaiting, paInternalError );

    PA_ENSURE( PaUnixMutex_Lock( &self->mtx ) );
    self->locked = 1;

error:
    return result;
}

PaError PaUnixThread_NotifyParent( PaUnixThread* self )
{
    PaError result = paNoError;
    PA_UNLESS( self->parentWaiting, paInternalError );

    if( !self->locked )
    {
        PA_ENSURE( PaUnixMutex_Lock( &self->mtx ) );
        self->locked = 1;
    }
    self->parentWaiting = 0;
    pthread_cond_signal( &self->cond );
    PA_ENSURE( PaUnixMutex_Unlock( &self->mtx ) );
    self->locked = 0;

error:
    return result;
}

int PaUnixThread_StopRequested( PaUnixThread* self )
{
    return self->stopRequested;
}

PaError PaUnixMutex_Initialize( PaUnixMutex* self )
{
    PaError result = paNoError;
    PA_ASSERT_CALL( pthread_mutex_init( &self->mtx, NULL ), 0 );
    return result;
}

PaError PaUnixMutex_Terminate( PaUnixMutex* self )
{
    PaError result = paNoError;
    PA_ASSERT_CALL( pthread_mutex_destroy( &self->mtx ), 0 );
    return result;
}

/** Lock mutex.
 *
 * We're disabling thread cancellation while the thread is holding a lock, so mutexes are 
 * properly unlocked at termination time.
 */
PaError PaUnixMutex_Lock( PaUnixMutex* self )
{
    PaError result = paNoError;
    int oldState;
    
    PA_ENSURE_SYSTEM( pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &oldState ), 0 );
    PA_ENSURE_SYSTEM( pthread_mutex_lock( &self->mtx ), 0 );

error:
    return result;
}

/** Unlock mutex.
 *
 * Thread cancellation is enabled again after the mutex is properly unlocked.
 */
PaError PaUnixMutex_Unlock( PaUnixMutex* self )
{
    PaError result = paNoError;
    int oldState;

    PA_ENSURE_SYSTEM( pthread_mutex_unlock( &self->mtx ), 0 );
    PA_ENSURE_SYSTEM( pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &oldState ), 0 );

error:
    return result;
}


#if 0
static void OnWatchdogExit( void *userData )
{
    PaAlsaThreading *th = (PaAlsaThreading *) userData;
    struct sched_param spm = { 0 };
    assert( th );

    PA_ASSERT_CALL( pthread_setschedparam( th->callbackThread, SCHED_OTHER, &spm ), 0 );    /* Lower before exiting */
    PA_DEBUG(( "Watchdog exiting\n" ));
}

static void *WatchdogFunc( void *userData )
{
    PaError result = paNoError, *pres = NULL;
    int err;
    PaAlsaThreading *th = (PaAlsaThreading *) userData;
    unsigned intervalMsec = 500;
    const PaTime maxSeconds = 3.;   /* Max seconds between callbacks */
    PaTime timeThen = PaUtil_GetTime(), timeNow, timeElapsed, cpuTimeThen, cpuTimeNow, cpuTimeElapsed;
    double cpuLoad, avgCpuLoad = 0.;
    int throttled = 0;

    assert( th );

    /* Execute OnWatchdogExit when exiting */
    pthread_cleanup_push( &OnWatchdogExit, th );

    /* Boost priority of callback thread */
    PA_ENSURE( result = BoostPriority( th ) );
    if( !result )
    {
        /* Boost failed, might as well exit */
        pthread_exit( NULL );
    }

    cpuTimeThen = th->callbackCpuTime;
    {
        int policy;
        struct sched_param spm = { 0 };
        pthread_getschedparam( pthread_self(), &policy, &spm );
        PA_DEBUG(( "%s: Watchdog priority is %d\n", __FUNCTION__, spm.sched_priority ));
    }

    while( 1 )
    {
        double lowpassCoeff = 0.9, lowpassCoeff1 = 0.99999 - lowpassCoeff;
        
        /* Test before and after in case whatever underlying sleep call isn't interrupted by pthread_cancel */
        pthread_testcancel();
        Pa_Sleep( intervalMsec );
        pthread_testcancel();

        if( PaUtil_GetTime() - th->callbackTime > maxSeconds )
        {
            PA_DEBUG(( "Watchdog: Terminating callback thread\n" ));
            /* Tell thread to terminate */
            err = pthread_kill( th->callbackThread, SIGKILL );
            pthread_exit( NULL );
        }

        PA_DEBUG(( "%s: PortAudio reports CPU load: %g\n", __FUNCTION__, PaUtil_GetCpuLoad( th->cpuLoadMeasurer ) ));

        /* Check if we should throttle, or unthrottle :P */
        cpuTimeNow = th->callbackCpuTime;
        cpuTimeElapsed = cpuTimeNow - cpuTimeThen;
        cpuTimeThen = cpuTimeNow;

        timeNow = PaUtil_GetTime();
        timeElapsed = timeNow - timeThen;
        timeThen = timeNow;
        cpuLoad = cpuTimeElapsed / timeElapsed;
        avgCpuLoad = avgCpuLoad * lowpassCoeff + cpuLoad * lowpassCoeff1;
        /*
        if( throttled )
            PA_DEBUG(( "Watchdog: CPU load: %g, %g\n", avgCpuLoad, cpuTimeElapsed ));
            */
        if( PaUtil_GetCpuLoad( th->cpuLoadMeasurer ) > .925 )
        {
            static int policy;
            static struct sched_param spm = { 0 };
            static const struct sched_param defaultSpm = { 0 };
            PA_DEBUG(( "%s: Throttling audio thread, priority %d\n", __FUNCTION__, spm.sched_priority ));

            pthread_getschedparam( th->callbackThread, &policy, &spm );
            if( !pthread_setschedparam( th->callbackThread, SCHED_OTHER, &defaultSpm ) )
            {
                throttled = 1;
            }
            else
                PA_DEBUG(( "Watchdog: Couldn't lower priority of audio thread: %s\n", strerror( errno ) ));

            /* Give other processes a go, before raising priority again */
            PA_DEBUG(( "%s: Watchdog sleeping for %lu msecs before unthrottling\n", __FUNCTION__, th->throttledSleepTime ));
            Pa_Sleep( th->throttledSleepTime );

            /* Reset callback priority */
            if( pthread_setschedparam( th->callbackThread, SCHED_FIFO, &spm ) != 0 )
            {
                PA_DEBUG(( "%s: Couldn't raise priority of audio thread: %s\n", __FUNCTION__, strerror( errno ) ));
            }

            if( PaUtil_GetCpuLoad( th->cpuLoadMeasurer ) >= .99 )
                intervalMsec = 50;
            else
                intervalMsec = 100;

            /*
            lowpassCoeff = .97;
            lowpassCoeff1 = .99999 - lowpassCoeff;
            */
        }
        else if( throttled && avgCpuLoad < .8 )
        {
            intervalMsec = 500;
            throttled = 0;

            /*
            lowpassCoeff = .9;
            lowpassCoeff1 = .99999 - lowpassCoeff;
            */
        }
    }

    pthread_cleanup_pop( 1 );   /* Execute cleanup on exit */

error:
    /* Shouldn't get here in the normal case */

    /* Pass on error code */
    pres = malloc( sizeof (PaError) );
    *pres = result;
    
    pthread_exit( pres );
}

static void CallbackUpdate( PaAlsaThreading *th )
{
    th->callbackTime = PaUtil_GetTime();
    th->callbackCpuTime = PaUtil_GetCpuLoad( th->cpuLoadMeasurer );
}

/*
static void *CanaryFunc( void *userData )
{
    const unsigned intervalMsec = 1000;
    PaUtilThreading *th = (PaUtilThreading *) userData;

    while( 1 )
    {
        th->canaryTime = PaUtil_GetTime();

        pthread_testcancel();
        Pa_Sleep( intervalMsec );
    }

    pthread_exit( NULL );
}
*/
#endif
