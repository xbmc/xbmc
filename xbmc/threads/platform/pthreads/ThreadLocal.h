/*
* Copyright (C) 2005-2011 Team XBMC
* http://www.xbmc.org
*
* This Program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This Program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with XBMC; see the file COPYING. If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
* http://www.gnu.org/copyleft/gpl.html
*
*/

#pragma once

#include <pthread.h>
#include <assert.h>

namespace XbmcThreads
{
  /**
   * A thin wrapper around pthreads thread specific storage
   * functionality.
   */
  template <typename T> class ThreadLocal
  {
    pthread_key_t key;
  public:
    inline ThreadLocal()
    { 
      int pthread_key_create_result = pthread_key_create(&key,NULL);
      assert(pthread_key_create_result == 0);
    }

    inline ~ThreadLocal()
    {
      int pthread_key_delete_result = pthread_key_delete(key);
      assert(pthread_key_delete_result == 0);
    }

    inline void set(T* val) 
    { 
      int pthread_setspecific_result = pthread_setspecific(key,(void*)val);
      assert(pthread_setspecific_result == 0);
    }

    inline T* get() 
    {
      return (T*)pthread_getspecific(key);
    }
  };
}

