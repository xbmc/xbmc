/* threadlib.c - jonclegg@yahoo.com
 * really bad threading library from hell
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

#if WIN32
#include <process.h>
#include <windows.h>
#elif __UNIX__
#include <pthread.h>
#elif __BEOS__
#include <be/kernel/OS.h>
#endif
#include <stdio.h>
#include <time.h>
#include "srtypes.h"
#include "threadlib.h"
#include "debug.h"

#if defined (commentout)
#if WIN32
	#define beginthread(thread, callback) \
		_beginthread((void *)callback, 0, (void *)NULL)
#elif __UNIX__
    #include <unistd.h>
	#define beginthread(thread, callback) \
		pthread_create(thread, NULL, \
                          (void *)callback, (void *)NULL)
#endif
#endif

/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
threadlib_beginthread (THREAD_HANDLE *thread, void (*callback)(void *), void* arg)
{
    BeginThread (thread->thread_handle, callback, arg);
    //if (thread->thread_handle == NULL)	// don't feel like porting this
    //	return SR_ERROR_CANT_CREATE_THREAD;

    return SR_SUCCESS;
}

void
threadlib_waitforclose(THREAD_HANDLE *thread)
{
    WaitForThread(thread->thread_handle);
}

HSEM threadlib_create_sem()
{
	HSEM s;
	SemInit(s);
	return s;
}

error_code threadlib_waitfor_sem(HSEM *e)
{
	if (!e)
		return SR_ERROR_INVALID_PARAM;
	SemWait(*e);
	return SR_SUCCESS;
}

error_code
threadlib_signal_sem(HSEM *e)
{
    if (!e)
	return SR_ERROR_INVALID_PARAM;
    SemPost(*e);
    return SR_SUCCESS;
}

void threadlib_destroy_sem(HSEM *e)
{
	DestroyThread(*e);
}
