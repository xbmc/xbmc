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
 * Windows implementation for generic mutex functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
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
	HANDLE os_mutex;
	
	/// The condition variable for signaling availability 
	HANDLE os_cond_lock;
	
	/// The condition variable for signaling a wake request
	HANDLE os_cond_wake;

	/// Number of threads currently waiting on this mutex
	int num_wait_threads;

	/// The locking thread if currently locked 
	DWORD os_thread;
	
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
	if ((mutex->os_mutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
		return NULL;
	} else if ((mutex->os_cond_lock = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
		int ec;
		
		ec = CloseHandle(mutex->os_mutex);
		assert(ec);
		return NULL;
	} else if ((mutex->os_cond_wake = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
		int ec;
		
		ec = CloseHandle(mutex->os_mutex);
		assert(ec);
		ec = CloseHandle(mutex->os_cond_lock);
		assert(ec);
		return NULL;
	}
	return mutex;
}

CP_HIDDEN void cpi_destroy_mutex(cpi_mutex_t *mutex) {
	int ec;
	
	assert(mutex != NULL);
	assert(mutex->lock_count == 0);
	ec = CloseHandle(mutex->os_mutex);
	assert(ec);
	ec = CloseHandle(mutex->os_cond_lock);
	assert(ec);
	ec = CloseHandle(mutex->os_cond_wake);
	assert(ec);
	free(mutex);
}

static char *get_win_errormsg(DWORD error, char *buffer, size_t size) {
	if (!FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS
		| FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		0,
		buffer,
		size / sizeof(char),
		NULL)) {
		strncpy(buffer, _("unknown error"), size);
	}
	buffer[size/sizeof(char) - 1] = '\0';
	return buffer;
}

static void lock_mutex(HANDLE mutex) {
	DWORD ec;
	
	if ((ec = WaitForSingleObject(mutex, INFINITE)) != WAIT_OBJECT_0) {
		char buffer[256];
		ec = GetLastError();
		cpi_fatalf(_("Could not lock a mutex due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void unlock_mutex(HANDLE mutex) {
	if (!ReleaseMutex(mutex)) {
		char buffer[256];
		DWORD ec = GetLastError();
		cpi_fatalf(_("Could not release a mutex due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void wait_for_event(HANDLE event) {
	if (WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0) {
		char buffer[256];
		DWORD ec = GetLastError();
		cpi_fatalf(_("Could not wait for an event due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void set_event(HANDLE event) {
	if (!SetEvent(event)) {
		char buffer[256];
		DWORD ec = GetLastError();
		cpi_fatalf(_("Could not set an event due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void reset_event(HANDLE event) {
	if (!ResetEvent(event)) {
		char buffer[256];
		DWORD ec = GetLastError();
		cpi_fatalf(_("Could not reset an event due to error %ld: %s"),
			(long) ec, get_win_errormsg(ec, buffer, sizeof(buffer)));
	}
}

static void lock_mutex_holding(cpi_mutex_t *mutex) {
	DWORD self = GetCurrentThreadId();
	
	while (mutex->lock_count != 0
			&& self != mutex->os_thread) {
		unlock_mutex(mutex->os_mutex);
		wait_for_event(mutex->os_cond_lock);
		lock_mutex(mutex->os_mutex);
	}
	mutex->os_thread = self;
	mutex->lock_count++;
}

CP_HIDDEN void cpi_lock_mutex(cpi_mutex_t *mutex) {
	assert(mutex != NULL);
	lock_mutex(mutex->os_mutex);
	lock_mutex_holding(mutex);
	unlock_mutex(mutex->os_mutex);
}

CP_HIDDEN void cpi_unlock_mutex(cpi_mutex_t *mutex) {
	DWORD self = GetCurrentThreadId();
	
	assert(mutex != NULL);
	lock_mutex(mutex->os_mutex);
	if (mutex->lock_count > 0
		&& self == mutex->os_thread) {
		if (--mutex->lock_count == 0) {
			set_event(mutex->os_cond_lock);
		}
	} else {
		cpi_fatalf(_("Internal C-Pluff error: Unauthorized attempt at unlocking a mutex."));
	}
	unlock_mutex(mutex->os_mutex);
}

CP_HIDDEN void cpi_wait_mutex(cpi_mutex_t *mutex) {
	DWORD self = GetCurrentThreadId();
	
	assert(mutex != NULL);
	lock_mutex(mutex->os_mutex);
	if (mutex->lock_count > 0
		&& self == mutex->os_thread) {
		int lc = mutex->lock_count;
		
		// Release mutex
		mutex->lock_count = 0;
		mutex->num_wait_threads++;
		set_event(mutex->os_cond_lock);
		unlock_mutex(mutex->os_mutex);
		
		// Wait for signal
		wait_for_event(mutex->os_cond_wake);
		
		// Reset wake signal if last one waking up
		lock_mutex(mutex->os_mutex);
		if (--mutex->num_wait_threads == 0) {
			reset_event(mutex->os_cond_wake);
		}
		
		// Re-acquire mutex and restore lock count for this thread
		lock_mutex_holding(mutex);
		mutex->lock_count = lc;		
		
	} else {
		cpi_fatalf(_("Internal C-Pluff error: Unauthorized attempt at waiting on a mutex."));
	}
	unlock_mutex(mutex->os_mutex);
}

CP_HIDDEN void cpi_signal_mutex(cpi_mutex_t *mutex) {
	DWORD self = GetCurrentThreadId();
	
	assert(mutex != NULL);
	lock_mutex(mutex->os_mutex);
	if (mutex->lock_count > 0
		&& self == mutex->os_thread) {
		set_event(mutex->os_cond_wake);
	} else {
		cpi_fatalf(_("Internal C-Pluff error: Unauthorized attempt at signaling a mutex."));
	}
	unlock_mutex(mutex->os_mutex);	
}

#if !defined(NDEBUG)
CP_HIDDEN int cpi_is_mutex_locked(cpi_mutex_t *mutex) {
	int locked;
	
	lock_mutex(mutex->os_mutex);
	locked = (mutex->lock_count != 0);
	unlock_mutex(mutex->os_mutex);
	return locked;
}
#endif
