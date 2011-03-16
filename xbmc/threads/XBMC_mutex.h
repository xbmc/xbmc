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
#ifndef _SDL_mutex_h
#define _SDL_mutex_h

#include <pthread.h>

/* Functions to provide thread synchronization primitives

	These are independent of the other SDL routines.
*/

/* Synchronization functions which can time out return this value
   if they time out.
*/
#define SDL_MUTEX_TIMEDOUT	1

/* This is the timeout value which corresponds to never time out */
#define SDL_MUTEX_MAXWAIT	(~(Uint32)0)


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Mutex functions                                               */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* The SDL mutex structure, defined in SDL_mutex.c */
#if !defined(_WIN32)
  #if !PTHREAD_MUTEX_RECURSIVE && \
      !PTHREAD_MUTEX_RECURSIVE_NP
  #define FAKE_RECURSIVE_MUTEX
  #endif
#endif

typedef struct SDL_mutex {
#if defined(_WIN32)
	HANDLE id;
#else
    pthread_mutex_t id;
    #if defined(FAKE_RECURSIVE_MUTEX)
      int recursive;
      pthread_t owner;
    #endif
#endif
} SDL_mutex;

/* Create a mutex, initialized unlocked */
SDL_mutex *SDL_CreateMutex(void);

/* Lock the mutex  (Returns 0, or -1 on error) */
#define SDL_LockMutex(m)	SDL_mutexP(m)
int SDL_mutexP(SDL_mutex *mutex);

/* Unlock the mutex  (Returns 0, or -1 on error)
   It is an error to unlock a mutex that has not been locked by
   the current thread, and doing so results in undefined behavior.
 */
#define SDL_UnlockMutex(m)	SDL_mutexV(m)
int SDL_mutexV(SDL_mutex *mutex);

/* Destroy a mutex */
void SDL_DestroyMutex(SDL_mutex *mutex);

#endif /* _SDL_mutex_h */
#endif
