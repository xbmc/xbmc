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
 *  Copyright (C) 2004-2006 bear
 *  Copyright (C) 2005      nate
 *
 *  bear and nate can be reached on the forums at
 *    http://forums.dvbowners.com/
 */

//TODO: code below is Windows specific. Make platform independent (use pthreads under Linux/OSX)

#ifdef TSREADER

#include "Thread.h"

CThread::CThread(const char* ThreadName)
{
  m_hStopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
  m_hDoneEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
  m_ThreadHandle = INVALID_HANDLE_VALUE;
  m_bThreadRunning=FALSE;

  if (ThreadName)
    m_ThreadName = ThreadName;
}

CThread::~CThread()
{
  StopThread();
  CloseHandle(m_hStopEvent);
  CloseHandle(m_hDoneEvent);
}

bool CThread::IsThreadRunning()
{
  return m_bThreadRunning;
}

long CThread::StartThread()
{
  ResetEvent(m_hStopEvent);
  m_ThreadHandle = (HANDLE) _beginthread(&CThread::staticThread, 0, (void *) this);
  if (m_ThreadHandle == INVALID_HANDLE_VALUE)
    return E_FAIL;

  return S_OK;
}

long CThread::StopThread(unsigned long dwTimeoutMilliseconds)
{
  long hr = S_OK;

  SetEvent(m_hStopEvent);
  long result = WaitForSingleObject(m_hDoneEvent, dwTimeoutMilliseconds);

  if ((result == WAIT_TIMEOUT) && (m_ThreadHandle != INVALID_HANDLE_VALUE))
  {
    TerminateThread(m_ThreadHandle, -1);
    CloseHandle(m_ThreadHandle);
    hr = S_FALSE;
  }
  else if (result != WAIT_OBJECT_0)
  {
    DWORD err = GetLastError();
    return HRESULT_FROM_WIN32(err);
  }

  m_ThreadHandle = INVALID_HANDLE_VALUE;

  return hr;
}

bool CThread::ThreadIsStopping(unsigned long dwTimeoutMilliseconds)
{
  DWORD result = WaitForSingleObject(m_hStopEvent, dwTimeoutMilliseconds);
  return (result != WAIT_TIMEOUT);
}

void CThread::Process()
{
  ResetEvent(m_hDoneEvent);
  m_bThreadRunning=TRUE;
  try
  {
    Run();
  }
  catch (LPWSTR pStr)
  {
    pStr = NULL;
  }
  SetEvent(m_hDoneEvent);
  m_bThreadRunning=FALSE;
}

THREADFUNC CThread::staticThread(void* data)
{
  CThread *thread = reinterpret_cast<CThread *>(data);
  thread->Process();
}

tThreadId CThread::ThreadId(void)
{
#ifdef __APPLE__
    return (int)pthread_self();
#else
#ifdef TARGET_WINDOWS
  return GetCurrentThreadId();
#else
  return syscall(__NR_gettid);
#endif
#endif
}

bool CThread::SetPriority(const int iPriority)
// Set thread priority
// Return true for success
{
  bool rtn = false;

#ifdef TARGET_WINDOWS
  if (m_ThreadHandle)
  {
    rtn = SetThreadPriority( m_ThreadHandle, iPriority ) == TRUE;
  }
#else
#warning TODO: implement me
#endif

  return(rtn);
}

#endif //TSREADER
