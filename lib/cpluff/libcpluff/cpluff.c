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
 * Core framework functions
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#ifdef DLOPEN_LIBTOOL
#include <ltdl.h>
#endif
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#ifdef CP_THREADS
#include "thread.h"
#endif
#include "internal.h"


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/// Number of initializations 
static int initialized = 0;

#ifdef CP_THREADS

/// Framework mutex
static cpi_mutex_t *framework_mutex = NULL;

#elif !defined(NDEBUG)

/// Framework locking count
static int framework_locked = 0;

#endif

/// Fatal error handler, or NULL for default 
static cp_fatal_error_func_t fatal_error_handler = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

CP_C_API const char *cp_get_version(void) {
	return CP_VERSION;
}

CP_C_API const char *cp_get_host_type(void) {
	return CP_HOST;
}

CP_HIDDEN void cpi_lock_framework(void) {
#if defined(CP_THREADS)
	cpi_lock_mutex(framework_mutex);
#elif !defined(NDEBUG)
	framework_locked++;
#endif
}

CP_HIDDEN void cpi_unlock_framework(void) {
#if defined(CP_THREADS)
	cpi_unlock_mutex(framework_mutex);
#elif !defined(NDEBUG)
	assert(framework_locked > 0);
	framework_locked--;
#endif
}

static void reset(void) {
#ifdef CP_THREADS
	if (framework_mutex != NULL) {
		cpi_destroy_mutex(framework_mutex);
		framework_mutex = NULL;
	}
#endif
}

CP_C_API cp_status_t cp_init(void) {
	cp_status_t status = CP_OK;
	
	// Initialize if necessary
	do {
		if (!initialized) {
			bindtextdomain(PACKAGE, CP_DATADIR CP_FNAMESEP_STR "locale");
#ifdef CP_THREADS
			if ((framework_mutex = cpi_create_mutex()) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
#endif
#ifdef DLOPEN_LIBTOOL
			if (lt_dlinit()) {
				status = CP_ERR_RESOURCE;
				break;
			}
#endif
		}
		initialized++;
	} while (0);
	
	// Rollback on failure
	if (status != CP_OK) {
		reset();
	}
	
	return status;
}

CP_C_API void cp_destroy(void) {
	assert(initialized > 0);
	initialized--;
	if (!initialized) {
#ifdef CP_THREADS
		assert(framework_mutex == NULL || !cpi_is_mutex_locked(framework_mutex));
#else
		assert(!framework_locked);
#endif
		cpi_destroy_all_contexts();
#ifdef DLOPEN_LIBTOOL
		lt_dlexit();
#endif
		reset();
	}
}

CP_C_API void cp_set_fatal_error_handler(cp_fatal_error_func_t error_handler) {
	fatal_error_handler = error_handler;
}

CP_HIDDEN void cpi_fatalf(const char *msg, ...) {
	va_list params;
	char fmsg[256];
		
	// Format message 
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';

	// Call error handler or print the error message
	if (fatal_error_handler != NULL) {
		fatal_error_handler(fmsg);
	} else {
		fprintf(stderr, _("C-Pluff: FATAL ERROR: %s\n"), fmsg);
	}
	
	// Abort if still alive 
	abort();
}

CP_HIDDEN void cpi_fatal_null_arg(const char *arg, const char *func) {
	cpi_fatalf(_("Argument %s has illegal NULL value in call to function %s."), arg, func);
}
