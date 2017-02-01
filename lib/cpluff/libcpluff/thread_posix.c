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
 * Posix implementation for generic mutex functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#include "internal.h"
#include "thread.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

// A generic recursive mutex implementation 
struct cpi_mutex_t {

	/// The current lock count 
	int lock_count;
	
	/// The underlying operating system mutex 
	pthread_mutex_t os_mutex;
	
	/// The condition variable for signaling availability 
	pthread_cond_t os_cond_lock;
	
	/// The condition variable for broadcasting a wake request
	pthread_cond_t os_cond_wake;

	/// The locking thread if currently locked 
	pthread_t os_thread;
	
};


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

CP_HIDDEN cpi_mutex_t * cpi_create_mutex(void) {
	cpi_mutex_t *mutex;
	
	if ((mutex = malloc(sizeof(cpi_mutex_t))) == NULL) {
		return NULL;
	}
	memset(mutex, 0, sizeof(cpi_mutex_t));
	if (pthread_mutex_init(&(mutex->os_mutex), NULL)) {
		return NULL;
	} else if (pthread_cond_init(&(mutex->os_cond_lock), NULL)) {
		int ec;
		
		ec = pthread_mutex_destroy(&(mutex->os_mutex));
		assert(!ec);
		return NULL;
	} else if (pthread_cond_init(&(mutex->os_cond_wake), NULL)) {
		int ec;
		
		ec = pthread_mutex_destroy(&(mutex->os_mutex));
		assert(!ec);
		ec = pthread_cond_destroy(&(mutex->os_cond_wake));
		assert(!ec);
		return NULL;
	}
	return mutex;
}

CP_HIDDEN void cpi_destroy_mutex(cpi_mutex_t *mutex) {
	int ec;
	
	assert(mutex != NULL);
	assert(mutex->lock_count == 0);
	ec = pthread_mutex_destroy(&(mutex->os_mutex));
	assert(!ec);
	ec = pthread_cond_destroy(&(mutex->os_cond_lock));
	assert(!ec);
	ec = pthread_cond_destroy(&(mutex->os_cond_wake));
	assert(!ec);
	free(mutex);
}

static void lock_mutex(pthread_mutex_t *mutex) {
	int ec;
	
	if ((ec = pthread_mutex_lock(mutex))) {
		cpi_fatalf(_("Could not lock a mutex due to error %d."), ec);
	}
}

static void unlock_mutex(pthread_mutex_t *mutex) {
	int ec;
	
	if ((ec = pthread_mutex_unlock(mutex))) {
		cpi_fatalf(_("Could not unlock a mutex due to error %d."), ec);
	}
}

static void lock_mutex_holding(cpi_mutex_t *mutex) {
	pthread_t self = pthread_self();
	
	while (mutex->lock_count != 0
			&& !pthread_equal(self, mutex->os_thread)) {
		int ec;
		
		if ((ec = pthread_cond_wait(&(mutex->os_cond_lock), &(mutex->os_mutex)))) {
			cpi_fatalf(_("Could not wait for a condition variable due to error %d."), ec);
		}
	}
	mutex->os_thread = self;
	mutex->lock_count++;
}

CP_HIDDEN void cpi_lock_mutex(cpi_mutex_t *mutex) {
	assert(mutex != NULL);
	lock_mutex(&(mutex->os_mutex));
	lock_mutex_holding(mutex);
	unlock_mutex(&(mutex->os_mutex));
}

CP_HIDDEN void cpi_unlock_mutex(cpi_mutex_t *mutex) {
	pthread_t self = pthread_self();
	
	assert(mutex != NULL);
	lock_mutex(&(mutex->os_mutex));
	if (mutex->lock_count > 0
		&& pthread_equal(self, mutex->os_thread)) {
		if (--mutex->lock_count == 0) {
			int ec;
			
			if ((ec = pthread_cond_signal(&(mutex->os_cond_lock)))) {
				cpi_fatalf(_("Could not signal a condition variable due to error %d."), ec);
			}
		}
	} else {
		cpi_fatalf(_("Internal C-Pluff error: Unauthorized attempt at unlocking a mutex."));
	}
	unlock_mutex(&(mutex->os_mutex));
}

CP_HIDDEN void cpi_wait_mutex(cpi_mutex_t *mutex) {
	pthread_t self = pthread_self();
	
	assert(mutex != NULL);
	lock_mutex(&(mutex->os_mutex));
	if (mutex->lock_count > 0
		&& pthread_equal(self, mutex->os_thread)) {
		int ec;
		int lc = mutex->lock_count;
		
		// Release mutex
		mutex->lock_count = 0;
		if ((ec = pthread_cond_signal(&(mutex->os_cond_lock)))) {
			cpi_fatalf(_("Could not signal a condition variable due to error %d."), ec);
		}
		
		// Wait for signal
		if ((ec = pthread_cond_wait(&(mutex->os_cond_wake), &(mutex->os_mutex)))) {
			cpi_fatalf(_("Could not wait for a condition variable due to error %d."), ec);
		}
		
		// Re-acquire mutex and restore lock count for this thread
		lock_mutex_holding(mutex);
		mutex->lock_count = lc;
		
	} else {
		cpi_fatalf(_("Internal C-Pluff error: Unauthorized attempt at waiting on a mutex."));
	}
	unlock_mutex(&(mutex->os_mutex));
}

CP_HIDDEN void cpi_signal_mutex(cpi_mutex_t *mutex) {
	pthread_t self = pthread_self();
	
	assert(mutex != NULL);
	lock_mutex(&(mutex->os_mutex));
	if (mutex->lock_count > 0
		&& pthread_equal(self, mutex->os_thread)) {
		int ec;
		
		// Signal the mutex
		if ((ec = pthread_cond_broadcast(&(mutex->os_cond_wake)))) {
			cpi_fatalf(_("Could not broadcast a condition variable due to error %d."), ec);
		}
		
	} else {
		cpi_fatalf(_("Internal C-Pluff error: Unauthorized attempt at signaling a mutex."));
	}
	unlock_mutex(&(mutex->os_mutex));
}

#if !defined(NDEBUG)
CP_HIDDEN int cpi_is_mutex_locked(cpi_mutex_t *mutex) {
	int locked;
	
	lock_mutex(&(mutex->os_mutex));
	locked = (mutex->lock_count != 0);
	unlock_mutex(&(mutex->os_mutex));
	return locked;
}
#endif
