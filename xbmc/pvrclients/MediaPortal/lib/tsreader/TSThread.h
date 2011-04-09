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

/*
 *  This file originates from TSFileSource, a GPL directshow push
 *  source filter that provides an MPEG transport stream output.
 *  Copyright (C) 2005      nate
 *  Copyright (C) 2006      bear
 *
 *  nate can be reached on the forums at
 *    http://forums.dvbowners.com/
 */

#ifdef TSREADER

#include "os-dependent.h"

class TSThread  
{
  public:
    TSThread();
    virtual ~TSThread();

    virtual void ThreadProc() = 0;
    long StartThread();
    long StopThread(unsigned long dwTimeoutMilliseconds = 1000);

    bool ThreadIsStopping(unsigned long dwTimeoutMilliseconds = 10);
    bool IsThreadRunning();
  protected:
    virtual void InternalThreadProc();
    wait_event_t m_hDoneEvent;
    wait_event_t m_hStopEvent;

  private:
    bool   m_bThreadRunning;
    HANDLE m_threadHandle;
    static void __cdecl thread_function(void* p);
};

#endif //TSREADER
