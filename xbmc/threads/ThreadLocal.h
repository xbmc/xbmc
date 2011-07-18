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

#include <boost/thread/tss.hpp>

namespace XbmcThreads
{
  extern void ThreadLocalNoCleanup(void*);

  /**
   * A thin wrapper around boost::thread_specific_ptr
   */
  template <typename T> class ThreadLocal
  {
    boost::thread_specific_ptr<T> value;

    typedef void (*cleanupFunction)(T*);
  public:
    inline ThreadLocal() : value((cleanupFunction)ThreadLocalNoCleanup) {}
    inline T* replace(T* val) { void* ret = value.get(); value.reset(val); return (T*)ret; }
    inline void set(T* val) { value.reset(val); }
    inline T* get() { return value.get(); }
  };
}

