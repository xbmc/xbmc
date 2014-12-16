/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#pragma once

#include <windows.h>
#include "commons/Exception.h"

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
    inline ThreadLocal()
    {
       if ((key = TlsAlloc()) == TLS_OUT_OF_INDEXES)
          throw XbmcCommons::UncheckedException("Ran out of Windows TLS Indexes. Windows Error Code %d",(int)GetLastError());
    }

    inline ~ThreadLocal() 
    {
       if (!TlsFree(key))
          throw XbmcCommons::UncheckedException("Failed to free Tls %d, Windows Error Code %d",(int)key, (int)GetLastError());
    }

    inline void set(T* val)
    {
       if (!TlsSetValue(key,(LPVOID)val))
          throw XbmcCommons::UncheckedException("Failed to set Tls %d, Windows Error Code %d",(int)key, (int)GetLastError());
    }

    inline T* get() { return (T*)TlsGetValue(key); }
  };
}

