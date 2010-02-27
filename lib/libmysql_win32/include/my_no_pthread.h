/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


#if !defined(_my_no_pthread_h) && !defined(THREAD)
#define _my_no_pthread_h


/*
  This block is to access some thread-related type definitions
  even in builds which do not need thread functions,
  as some variables (based on these types) are declared
  even in non-threaded builds.
  Case in point: 'mf_keycache.c'
*/
#if defined(__WIN__)
#else /* Normal threads */
#include <pthread.h>

#endif /* defined(__WIN__) */


/*
  This undefs some pthread mutex locks when one isn't using threads
  to make thread safe code, that should also work in single thread
  environment, easier to use.
*/
#define pthread_mutex_init(A,B)
#define pthread_mutex_lock(A)
#define pthread_mutex_unlock(A)
#define pthread_mutex_destroy(A)
#define my_rwlock_init(A,B)
#define rw_rdlock(A)
#define rw_wrlock(A)
#define rw_unlock(A)
#define rwlock_destroy(A)

typedef int my_pthread_once_t;
#define MY_PTHREAD_ONCE_INIT 0
#define MY_PTHREAD_ONCE_DONE 1

#define my_pthread_once(C,F) do { \
    if (*(C) != MY_PTHREAD_ONCE_DONE) { F(); *(C)= MY_PTHREAD_ONCE_DONE; } \
  } while(0)

#endif
