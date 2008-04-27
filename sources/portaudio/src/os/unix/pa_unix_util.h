/*
 * $Id: pa_unix_util.h 1241 2007-07-23 20:08:31Z aknudsen $
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

#ifndef PA_UNIX_UTIL_H
#define PA_UNIX_UTIL_H

#include "pa_cpuload.h"
#include <assert.h>
#include <pthread.h>
#include <signal.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define PA_MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define PA_MAX(x,y) ( (x) > (y) ? (x) : (y) )

/* Utilize GCC branch prediction for error tests */
#if defined __GNUC__ && __GNUC__ >= 3
#define UNLIKELY(expr) __builtin_expect( (expr), 0 )
#else
#define UNLIKELY(expr) (expr)
#endif

#define STRINGIZE_HELPER(expr) #expr
#define STRINGIZE(expr) STRINGIZE_HELPER(expr)

#define PA_UNLESS(expr, code) \
    do { \
        if( UNLIKELY( (expr) == 0 ) ) \
        { \
            PaUtil_DebugPrint(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            result = (code); \
            goto error; \
        } \
    } while (0);

static PaError paUtilErr_;          /* Used with PA_ENSURE */

/* Check PaError */
#define PA_ENSURE(expr) \
    do { \
        if( UNLIKELY( (paUtilErr_ = (expr)) < paNoError ) ) \
        { \
            PaUtil_DebugPrint(( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" )); \
            result = paUtilErr_; \
            goto error; \
        } \
    } while (0);

#define PA_ASSERT_CALL(expr, success) \
    paUtilErr_ = (expr); \
    assert( success == paUtilErr_ );

#define PA_ENSURE_SYSTEM(expr, success) \
    do { \
        if( UNLIKELY( (paUtilErr_ = (expr)) != success ) ) \
        { \
            /* PaUtil_SetLastHostErrorInfo should only be used in the main thread */ \
            if( pthread_equal(pthread_self(), paUnixMainThread) ) \
            { \
                PaUtil_SetLastHostErrorInfo( paALSA, paUtilErr_, strerror( paUtilErr_ ) ); \
            } \
            PaUtil_DebugPrint( "Expression '" #expr "' failed in '" __FILE__ "', line: " STRINGIZE( __LINE__ ) "\n" ); \
            result = paUnanticipatedHostError; \
            goto error; \
        } \
    } while( 0 );

typedef struct {
    pthread_t callbackThread;
} PaUtilThreading;

PaError PaUtil_InitializeThreading( PaUtilThreading *threading );
void PaUtil_TerminateThreading( PaUtilThreading *threading );
PaError PaUtil_StartThreading( PaUtilThreading *threading, void *(*threadRoutine)(void *), void *data );
PaError PaUtil_CancelThreading( PaUtilThreading *threading, int wait, PaError *exitResult );

/* State accessed by utility functions */

/*
void PaUnix_SetRealtimeScheduling( int rt );

void PaUtil_InitializeThreading( PaUtilThreading *th, PaUtilCpuLoadMeasurer *clm );

PaError PaUtil_CreateCallbackThread( PaUtilThreading *th, void *(*CallbackThreadFunc)( void * ), PaStream *s );

PaError PaUtil_KillCallbackThread( PaUtilThreading *th, PaError *exitResult );

void PaUtil_CallbackUpdate( PaUtilThreading *th );
*/

extern pthread_t paUnixMainThread;

typedef struct
{
    pthread_mutex_t mtx;
} PaUnixMutex;

PaError PaUnixMutex_Initialize( PaUnixMutex* self );
PaError PaUnixMutex_Terminate( PaUnixMutex* self );
PaError PaUnixMutex_Lock( PaUnixMutex* self );
PaError PaUnixMutex_Unlock( PaUnixMutex* self );

typedef struct
{
    pthread_t thread;
    int parentWaiting;
    int stopRequested;
    int locked;
    PaUnixMutex mtx;
    pthread_cond_t cond;
    volatile sig_atomic_t stopRequest;
} PaUnixThread;

/** Initialize global threading state.
 */
PaError PaUnixThreading_Initialize();

/** Perish, passing on eventual error code.
 *
 * A thin wrapper around pthread_exit, will automatically pass on any error code to the joining thread.
 * If the result indicates an error, i.e. it is not equal to paNoError, this function will automatically
 * allocate a pointer so the error is passed on with pthread_exit. If the result indicates that all is
 * well however, only a NULL pointer will be handed to pthread_exit. Thus, the joining thread should
 * check whether a non-NULL result pointer is obtained from pthread_join and make sure to free it.
 * @param result: The error code to pass on to the joining thread.
 */
#define PaUnixThreading_EXIT(result) \
    do { \
        PaError* pres = NULL; \
        if( paNoError != (result) ) \
        { \
            pres = malloc( sizeof (PaError) ); \
            *pres = (result); \
        } \
        pthread_exit( pres ); \
    } while (0);

/** Spawn a thread.
 *
 * Intended for spawning the callback thread from the main thread. This function can even block (for a certain
 * time or indefinitely) untill notified by the callback thread (using PaUnixThread_NotifyParent), which can be
 * useful in order to make sure that callback has commenced before returning from Pa_StartStream.
 * @param threadFunc: The function to be executed in the child thread.
 * @param waitForChild: If not 0, wait for child thread to call PaUnixThread_NotifyParent. Less than 0 means
 * wait for ever, greater than 0 wait for the specified time.
 * @param rtSched: Enable realtime scheduling?
 * @return: If timed out waiting on child, paTimedOut.
 */
PaError PaUnixThread_New( PaUnixThread* self, void* (*threadFunc)( void* ), void* threadArg, PaTime waitForChild,
        int rtSched );

/** Terminate thread.
 *
 * @param wait: If true, request that background thread stop and wait untill it does, else cancel it.
 * @param exitResult: If non-null this will upon return contain the exit status of the thread.
 */
PaError PaUnixThread_Terminate( PaUnixThread* self, int wait, PaError* exitResult );

/** Prepare to notify waiting parent thread.
 *
 * An internal lock must be held before the parent is notified in PaUnixThread_NotifyParent, call this to
 * acquire it beforehand.
 * @return: If parent is not waiting, paInternalError.
 */
PaError PaUnixThread_PrepareNotify( PaUnixThread* self );

/** Notify waiting parent thread.
 *
 * @return: If parent timed out waiting, paTimedOut. If parent was never waiting, paInternalError.
 */
PaError PaUnixThread_NotifyParent( PaUnixThread* self );

/** Has the parent thread requested this thread to stop?
 */
int PaUnixThread_StopRequested( PaUnixThread* self );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
