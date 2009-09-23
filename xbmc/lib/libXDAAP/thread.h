/* thread abstraction
 *
 * Copyright (C) 2004 David Hammerton
 * crazney@crazney.net
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "portability.h"

#if defined(SYSTEM_POSIX)   /* POSIX */

#define THREADS_POSIX

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#define ts_thread     pthread_t
#define ts_mutex      pthread_mutex_t
#define ts_condition  pthread_cond_t

#ifdef __APPLE__
	#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE 
#endif 
	
#define ts_mutex_create(m)            pthread_mutex_init(& m, NULL)
#define ts_mutex_create_recursive(m)  do { \
        pthread_mutexattr_t attr; \
        pthread_mutexattr_init(&attr) ; \
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP); \
        pthread_mutex_init(& m, &attr); \
        pthread_mutexattr_destroy(&attr) ; \
    } while (0)
#define ts_mutex_lock(m)              pthread_mutex_lock(& m)
#define ts_mutex_unlock(m)            pthread_mutex_unlock(& m)
#define ts_mutex_destroy(m)           pthread_mutex_destroy(& m)

#define ts_condition_create(c)     pthread_cond_init(& c, NULL)
#define ts_condition_wait(c, m)    pthread_cond_wait(& c, & m)
#define ts_condition_signal(c)     pthread_cond_signal(& c)
#define ts_condition_signal_all(c) pthread_cond_broadcast(& c)
#define ts_condition_destroy(c)    pthread_cond_destroy(& c)

#define ts_thread_create(t, cb, data) pthread_create(& t, NULL, \
                                                     (void*)cb, (void*)data)
#define ts_thread_join(t)             pthread_join(t, NULL)
#define ts_exit()                     pthread_exit(NULL)
#define ts_thread_close(t)            do {} while (0)

#define ts_thread_cb(name)            static void* name(void *arg)
#define ts_thread_defaultret          NULL

#elif defined(_WIN32) /* win32 */

#define THREADS_WIN32
#include <windows.h>

#define ts_thread     HANDLE
#define ts_mutex      HANDLE
#define ts_condition  HANDLE

#define ts_mutex_create(m)        do { m = CreateMutex(NULL, FALSE, NULL); } while(0)
#define ts_mutex_lock(m)          WaitForSingleObject(m, INFINITE)
#define ts_mutex_unlock(m)       ReleaseMutex(m)
#define ts_mutex_destroy(m)      CloseHandle(m)

/* threadpool.c isn't used
#define ts_condition_create(c)    do { c = CreateEvent(NULL, FALSE, FALSE, NULL) } while(0)
*/
#define ts_thread_create(t, cb, data) do { t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cb, \
                                                        (LPVOID)data, 0, NULL); \
									  } while(0)
#define ts_thread_close(t)            CloseHandle(t)

#define ts_thread_cb(name)            static DWORD WINAPI name(LPVOID arg)
#define ts_thread_defaultret          0;

#else

#error IMPLEMENT ME

#endif




