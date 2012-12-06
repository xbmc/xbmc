/*
* Copyright (C) 2005-2012 Team XBMC
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
* <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include <windows.h>

namespace XbmcThreads
{
  /**
   * A thin wrapper around windows thread specific storage
   * functionality.
   */
  template <typename T> class ThreadLocal
  {
    DWORD key;
  public:
    inline ThreadLocal() { key = TlsAlloc(); }

    inline ~ThreadLocal() { TlsFree(key);  }

    inline void set(T* val) {  TlsSetValue(key,(LPVOID)val);  }

    inline T* get() { return (T*)TlsGetValue(key); }
  };
}

