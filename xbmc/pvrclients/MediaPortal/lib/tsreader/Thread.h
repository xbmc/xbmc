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
#include <string>

#ifdef TARGET_WINDOWS
#define THREADFUNC void __cdecl
#define THREADHANDLE HANDLE
#else
#define THREADFUNC int
#define THREADHANDLE pthread_t
#endif

class IRunnable
{
public:
  virtual void Run()=0;
  virtual ~IRunnable() {}
};

class CThread: public IRunnable
{
  public:
    CThread(const char* ThreadName);
    virtual ~CThread();

    long StartThread();
    long StopThread(unsigned long dwTimeoutMilliseconds = 1000);

    bool ThreadIsStopping(unsigned long dwTimeoutMilliseconds = 10);
    bool IsThreadRunning();
    tThreadId ThreadId(void);
  protected:
    bool SetPriority(const int iPriority);
    virtual void Process();
    wait_event_t m_hDoneEvent;
    wait_event_t m_hStopEvent;

  private:
    bool   m_bThreadRunning;
    IRunnable* m_pRunnable;
    static THREADFUNC staticThread(void *data);
    THREADHANDLE m_ThreadHandle;
    std::string m_ThreadName;
};

#endif //TSREADER
