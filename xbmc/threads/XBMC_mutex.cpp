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
#if defined(_WIN32)
  #include <windows.h>
#else
  #include <stdlib.h>
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
#endif