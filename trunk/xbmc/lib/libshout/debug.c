/* debug.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "srtypes.h"
#include "threadlib.h"
#include "rip_manager.h"
#include "mchar.h"
#include "debug.h"

#if WIN32
    #define vsnprintf _vsnprintf
    #define vswprintf _vsnwprintf
#else
    /* This prototype is missing in some systems */
    int vswprintf (wchar_t * ws, size_t n, const wchar_t * format, va_list arg);
#endif

/*****************************************************************************
 * Public functions
 *****************************************************************************/
#define DEBUG_BUF_LEN 2048
int debug_on = 0;
FILE* gcsfp = 0;
static HSEM m_debug_lock;
static char* debug_filename = 0;
static char filename_buf[SR_MAX_PATH];
static char* default_filename = "gcs.txt";
static int debug_initialized = 0;

void
debug_set_filename (char* filename)
{
    sr_strncpy (filename_buf, filename, SR_MAX_PATH);
    debug_filename = filename_buf;
}

void
debug_enable (void)
{
    debug_on = 1;
    if (!debug_filename) {
	debug_filename = default_filename;
    }
}

void
debug_open (void)
{
    if (!debug_on) return;
    if (!gcsfp) {
	gcsfp = fopen(debug_filename, "a");
	if (!gcsfp) {
	    debug_on = 0;
	}
    }
}

void
debug_close (void)
{
    if (!debug_on) return;
    if (gcsfp) {
	fclose(gcsfp);
	gcsfp = 0;
    }
}

void
#ifdef XBMC
debug_printf2 (char* fmt, ...)
#else
debug_printf (char* fmt, ...)
#endif
{
    int was_open = 1;
    va_list argptr;

    if (!debug_on) {
	/* Uncomment to debug debug_mprintf() */
#if defined (commentout)
	va_start (argptr, fmt);
        vprintf (fmt, argptr);
	va_end (argptr);
#endif
	return;
    }

    if (!debug_initialized) {
        m_debug_lock = threadlib_create_sem();
        threadlib_signal_sem(&m_debug_lock);
    }
    threadlib_waitfor_sem (&m_debug_lock);

    va_start (argptr, fmt);
    if (!gcsfp) {
	was_open = 0;
	debug_open();
	if (!gcsfp) return;
    }
    if (!debug_initialized) {
	debug_initialized = 1;
	fprintf (gcsfp, "=========================\n");
	fprintf (gcsfp, "STREAMRIPPER " SRPLATFORM " " SRVERSION "\n");
    }

    vfprintf (gcsfp, fmt, argptr);
    fflush (gcsfp);

    va_end (argptr);
    if (!was_open) {
	debug_close ();
    }
    threadlib_signal_sem (&m_debug_lock);
}

void
debug_mprintf (mchar* fmt, ...)
{
    int was_open = 1;
    va_list argptr;
    int rc;
#if defined HAVE_WCHAR_SUPPORT    
    mchar mbuf[DEBUG_BUF_LEN];
#endif    
    char cbuf[DEBUG_BUF_LEN];

    if (!debug_on) return;

    if (!debug_initialized) {
        m_debug_lock = threadlib_create_sem();
        threadlib_signal_sem(&m_debug_lock);
    }
    threadlib_waitfor_sem (&m_debug_lock);

    va_start (argptr, fmt);
    if (!gcsfp) {
	was_open = 0;
	debug_open();
	if (!gcsfp) return;
    }
    if (!debug_initialized) {
	debug_initialized = 1;
	fprintf (gcsfp, "=========================\n");
	fprintf (gcsfp, "STREAMRIPPER " SRPLATFORM " " SRVERSION "\n");
    }

#if defined HAVE_WCHAR_SUPPORT
    rc = vswprintf (mbuf, DEBUG_BUF_LEN, fmt, argptr);
    debug_on = 0;   /* Avoid recursive call which hangs on semaphore */
    rc = string_from_mstring (cbuf, DEBUG_BUF_LEN, mbuf, CODESET_LOCALE);
    debug_on = 1;
#else
    rc = vsnprintf (cbuf, DEBUG_BUF_LEN, fmt, argptr);
#endif

    fwrite (cbuf, 1, rc, gcsfp);

    fflush (gcsfp);

    va_end (argptr);
    if (!was_open) {
	debug_close ();
    }
    threadlib_signal_sem (&m_debug_lock);
}

void
debug_print_error (void)
{
#if defined (WIN32)
    LPVOID lpMsgBuf;
    FormatMessage (
	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	FORMAT_MESSAGE_FROM_SYSTEM | 
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	GetLastError(),
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	(LPTSTR) &lpMsgBuf,
	0,
	NULL 
    );
    debug_printf ("%s", lpMsgBuf);
    LocalFree (lpMsgBuf);
#endif
}
