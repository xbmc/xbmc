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

#include "system.h"
#if defined(HAS_SDL)
#include <SDL/SDL_mutex.h>
#else
#if defined(_WIN32)
  #include <windows.h>
#else
  #include <sys/time.h>
  #include <unistd.h>
  #include <errno.h>
  #include <stdlib.h>
  #include <pthread.h>
#endif

#include "XBMC_mutex.h"
#include "log.h"

SDL_mutex *SDL_CreateMutex (void)
{
#if defined(_WIN32)
	SDL_mutex *mutex;

	/* Allocate mutex memory */
	mutex = (SDL_mutex *)malloc(sizeof(*mutex));
	if ( mutex ) {
		/* Create the mutex, with initial value signaled */
		mutex->id = CreateMutex(NULL, FALSE, NULL);
		if ( ! mutex->id ) {
      CLog::Log(LOGERROR, "Couldn't create mutex");
      free(mutex);
			mutex = NULL;
		}
	} else {
    CLog::Log(LOGERROR, "OutOfMemory");
	}
	return(mutex);
#else
	SDL_mutex *mutex;
	pthread_mutexattr_t attr;

	/* Allocate the structure */
	mutex = (SDL_mutex *)calloc(1, sizeof(*mutex));
	if ( mutex ) {
		pthread_mutexattr_init(&attr);
    #if defined(PTHREAD_MUTEX_RECURSIVE)
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    #elif defined(PTHREAD_MUTEX_RECURSIVE_NP)
        pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    #else
        /* No extra attributes necessary */
    #endif
		if ( pthread_mutex_init(&mutex->id, &attr) != 0 ) {
      CLog::Log(LOGERROR, "pthread_mutex_init() failed");
			free(mutex);
			mutex = NULL;
		}
	} else {
    CLog::Log(LOGERROR, "OutOfMemory");
	}
	return(mutex);
#endif
}

void SDL_DestroyMutex(SDL_mutex *mutex)
{
#if defined(_WIN32)
	if ( mutex ) {
		if ( mutex->id ) {
			CloseHandle(mutex->id);
			mutex->id = 0;
		}
    free(mutex);
	}
#else

	if ( mutex ) {
		pthread_mutex_destroy(&mutex->id);
		free(mutex);
	}
#endif
}

/* Lock the mutex */
int SDL_mutexP(SDL_mutex *mutex)
{
#if defined(_WIN32)
	if ( mutex == NULL ) {
    CLog::Log(LOGERROR, "Passed a NULL mutex");
		return -1;
	}
	if ( WaitForSingleObject(mutex->id, INFINITE) == WAIT_FAILED ) {
    CLog::Log(LOGERROR, "Couldn't wait on mutex");
		return -1;
	}
	return(0);
#else
	int retval;
  #if defined(FAKE_RECURSIVE_MUTEX)
    pthread_t this_thread;
  #endif

	if ( mutex == NULL ) {
    CLog::Log(LOGERROR, "Passed a NULL mutex");
		return -1;
	}

	retval = 0;
  #if defined(FAKE_RECURSIVE_MUTEX)
    this_thread = pthread_self();
    if (pthread_equal(mutex->owner, this_thread)) {
    //if ( mutex->owner == this_thread ) {
      ++mutex->recursive;
    } else {
      /* The order of operations is important.
         We set the locking thread id after we obtain the lock
         so unlocks from other threads will fail.
      */
      if ( pthread_mutex_lock(&mutex->id) == 0 ) {
        mutex->owner = this_thread;
        mutex->recursive = 0;
      } else {
        CLog::Log(LOGERROR, "pthread_mutex_lock() failed");
        retval = -1;
      }
    }
  #else
    if ( pthread_mutex_lock(&mutex->id) < 0 ) {
      CLog::Log(LOGERROR, "pthread_mutex_lock() failed");
      retval = -1;
    }
  #endif
	return retval;
#endif
}

int SDL_mutexV(SDL_mutex *mutex)
{
#if defined(_WIN32)
	if ( mutex == NULL ) {
    CLog::Log(LOGERROR, "Passed a NULL mutex");
		return -1;
	}
	if ( ReleaseMutex(mutex->id) == FALSE ) {
    CLog::Log(LOGERROR, "Couldn't release mutex");
		return -1;
	}
	return(0);
#else
	int retval;

	if ( mutex == NULL ) {
    CLog::Log(LOGERROR, "Passed a NULL mutex");
		return -1;
	}

	retval = 0;
  #if defined(FAKE_RECURSIVE_MUTEX)
    /* We can only unlock the mutex if we own it */
    if (pthread_equal(pthread_self(), mutex->owner)) {
    //if ( pthread_self() == mutex->owner ) {
      if ( mutex->recursive ) {
        --mutex->recursive;
      } else {
        /* The order of operations is important.
           First reset the owner so another thread doesn't lock
           the mutex and set the ownership before we reset it,
           then release the lock semaphore.
         */
        mutex->owner = 0;
        pthread_mutex_unlock(&mutex->id);
      }
    } else {
      CLog::Log(LOGERROR, "mutex not owned by this thread");
      retval = -1;
    }

  #else
    if ( pthread_mutex_unlock(&mutex->id) < 0 ) {
      CLog::Log(LOGERROR, "pthread_mutex_unlock() failed");
      retval = -1;
    }
  #endif /* FAKE_RECURSIVE_MUTEX */

	return retval;
#endif
}

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
