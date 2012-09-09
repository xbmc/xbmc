/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/Lockables.h"
#include "threads/platform/pthreads/CriticalSection.h"
#include "threads/Helpers.h"

#include <pthread.h>

namespace XbmcThreads
{
  namespace pthreads
  {
    // ==========================================================
    static pthread_mutexattr_t recursiveAttr;

    static bool setRecursiveAttr() 
    {
      static bool alreadyCalled = false; // initialized to 0 in the data segment prior to startup init code running
      if (!alreadyCalled)
      {
        pthread_mutexattr_init(&recursiveAttr);
        pthread_mutexattr_settype(&recursiveAttr,PTHREAD_MUTEX_RECURSIVE);
        alreadyCalled = true;
      }
      return true; // note, we never call destroy.
    }

    static bool recursiveAttrSet = setRecursiveAttr();

    pthread_mutexattr_t* RecursiveMutex::getRecursiveAttr()
    {
      if (!recursiveAttrSet) // this is only possible in the single threaded startup code
        recursiveAttrSet = setRecursiveAttr();
      return &recursiveAttr;
    }
    // ==========================================================
  }
}

