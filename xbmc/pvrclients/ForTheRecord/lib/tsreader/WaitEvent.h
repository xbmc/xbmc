#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef TSREADER

#include "libPlatform/os-dependent.h"
#if defined(TARGET_WINDOWS)
typedef HANDLE wait_event_t;
#else
typedef sem_t wait_event_t;
#include "PlatformDefs.h"
#endif

class CWaitEvent
{
  public:
    CWaitEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, int bManualReset, int bInitialState, const char* lpName);
    virtual ~CWaitEvent(void);
    bool Wait();
    void SetEvent();
    void ResetEvent();

  protected:
    wait_event_t m_waitevent;
};

#endif //TSREADER
