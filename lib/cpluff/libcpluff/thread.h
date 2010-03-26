/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file
 * Declarations for generic mutex functions and types
 */ 

#ifndef THREAD_H_
#define THREAD_H_
#ifdef CP_THREADS

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

// A generic mutex implementation 
typedef struct cpi_mutex_t cpi_mutex_t;


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

// Mutex functions 

/**
 * Creates a mutex. The mutex is initially available.
 * 
 * @return the created mutex or NULL if no resources available
 */
CP_HIDDEN cpi_mutex_t * cpi_create_mutex(void);

/**
 * Destroys the specified mutex.
 * 
 * @param mutex the mutex
 */
CP_HIDDEN void cpi_destroy_mutex(cpi_mutex_t *mutex);

/**
 * Waits for the specified mutex to become available and locks it.
 * If the calling thread has already locked the mutex then the
 * lock count of the mutex is increased.
 * 
 * @param mutex the mutex
 */
CP_HIDDEN void cpi_lock_mutex(cpi_mutex_t *mutex);

/**
 * Unlocks the specified mutex which must have been previously locked
 * by this thread. If there has been several calls to cpi_lock_mutex
 * by the same thread then the mutex is unlocked only after corresponding
 * number of unlock requests.
 * 
 * @param mutex the mutex
 */
CP_HIDDEN void cpi_unlock_mutex(cpi_mutex_t *mutex);

/**
 * Waits on the specified mutex until it is signaled. The calling thread
 * must hold the mutex. The mutex is released on call to this function and
 * it is reacquired before the function returns.
 * 
 * @param mutex the mutex to wait on
 */
CP_HIDDEN void cpi_wait_mutex(cpi_mutex_t *mutex);

/**
 * Signals the specified mutex waking all the threads currently waiting on
 * the mutex. The calling thread must hold the mutex. The mutex is not
 * released.
 * 
 * @param mutex the mutex to be signaled
 */
CP_HIDDEN void cpi_signal_mutex(cpi_mutex_t *mutex);

#if !defined(NDEBUG)

/**
 * Returns whether the mutex is currently locked. This function
 * is only intended to be used for assertions. The returned state
 * reflects the state of the mutex only at the time of inspection.
 */
CP_HIDDEN int cpi_is_mutex_locked(cpi_mutex_t *mutex);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus 

#endif //CP_THREADS
#endif //THREAD_H_
