/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#if defined(__APPLE__) && defined(__arm__)
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "XBMC_cond.h"
#include "log.h"

/* Create a condition variable */
SDL_cond * SDL_CreateCond(void)
{
	SDL_cond *cond;

	cond = (SDL_cond *) malloc(sizeof(SDL_cond));
	if ( cond ) {
		if ( pthread_cond_init(&cond->cond, NULL) < 0 ) {
      CLog::Log(LOGERROR, "pthread_cond_init() failed");
			free(cond);
			cond = NULL;
		}
	}
	return(cond);
}

/* Destroy a condition variable */
void SDL_DestroyCond(SDL_cond *cond)
{
	if ( cond ) {
		pthread_cond_destroy(&cond->cond);
		free(cond);
	}
}

/* Restart one of the threads that are waiting on the condition variable */
int SDL_CondSignal(SDL_cond *cond)
{
	int retval;

	if ( ! cond ) {
    CLog::Log(LOGERROR, "Passed a NULL condition variable");
		return -1;
	}

	retval = 0;
	if ( pthread_cond_signal(&cond->cond) != 0 ) {
    CLog::Log(LOGERROR, "pthread_cond_signal() failed");
		retval = -1;
	}
	return retval;
}

/* Restart all threads that are waiting on the condition variable */
int SDL_CondBroadcast(SDL_cond *cond)
{
	int retval;

	if ( ! cond ) {
    CLog::Log(LOGERROR, "Passed a NULL condition variable");
		return -1;
	}

	retval = 0;
	if ( pthread_cond_broadcast(&cond->cond) != 0 ) {
    CLog::Log(LOGERROR, "pthread_cond_broadcast() failed");
		retval = -1;
	}
	return retval;
}

int SDL_CondWaitTimeout(SDL_cond *cond, SDL_mutex *mutex, uint32_t ms)
{
	int retval;
	struct timeval delta;
	struct timespec abstime;

	if ( ! cond ) {
    CLog::Log(LOGERROR, "Passed a NULL condition variable");
		return -1;
	}

	gettimeofday(&delta, NULL);

	abstime.tv_sec = delta.tv_sec + (ms/1000);
	abstime.tv_nsec = (delta.tv_usec + (ms%1000) * 1000) * 1000;
        if ( abstime.tv_nsec > 1000000000 ) {
          abstime.tv_sec += 1;
          abstime.tv_nsec -= 1000000000;
        }

  tryagain:
	retval = pthread_cond_timedwait(&cond->cond, &mutex->id, &abstime);
	switch (retval) {
	    case EINTR:
		goto tryagain;
		break;
	    case ETIMEDOUT:
		retval = SDL_MUTEX_TIMEDOUT;
		break;
	    case 0:
		break;
	    default:
    CLog::Log(LOGERROR, "pthread_cond_timedwait() failed");
		retval = -1;
		break;
	}
	return retval;
}

/* Wait on the condition variable, unlocking the provided mutex.
   The mutex must be locked before entering this function!
 */
int SDL_CondWait(SDL_cond *cond, SDL_mutex *mutex)
{
	int retval;

	if ( ! cond ) {
    CLog::Log(LOGERROR, "Passed a NULL condition variable");
		return -1;
	}

	retval = 0;
	if ( pthread_cond_wait(&cond->cond, &mutex->id) != 0 ) {
    CLog::Log(LOGERROR, "pthread_cond_wait() failed");
		retval = -1;
	}
	return retval;
}
#endif