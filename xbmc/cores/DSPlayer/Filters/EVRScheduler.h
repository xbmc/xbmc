/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _EVRSCHEDULER_H
#define _EVRSCHEDULER_H
#pragma once


#include "streams.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>

#include "EvrHelper.h"
#include "DShowUtil/smartptr.h"
#include "Thread.h"

struct SchedulerCallback;

enum ScheduleEvent
{
  eTerminate =    WM_USER,
  eSchedule =     WM_USER + 1,
  eFlush =        WM_USER + 2
};

class CEvrScheduler
{
public:
  CEvrScheduler();
  virtual ~CEvrScheduler();

  void SetCallback(SchedulerCallback *pCB)
  {
    m_pCB = pCB;
  }

  void SetFrameRate(MFTIME TimePerFrame);
  void SetClockRate(float fRate) { m_fRate = fRate; }

  const LONGLONG& LastSampleTime() const { return m_LastSampleTime; }
  const LONGLONG& FrameDuration() const { return m_PerFrameInterval; }

  HRESULT StartScheduler(IMFClock *pClock);
  HRESULT StopScheduler();

  HRESULT ScheduleSample(IMFSample *pSample, BOOL bPresentNow);
  HRESULT ProcessSamplesInQueue(LONG *plNextSleep);
  float   GetFps();
  HRESULT ProcessSample(IMFSample *pSample, LONG *plNextSleep);
  HRESULT Flush();

  // ThreadProc for the scheduler thread.
  static DWORD WINAPI SchedulerThreadProc(LPVOID lpParameter);

  unsigned int uiFrameDropped;

private: 
  // non-static version of SchedulerThreadProc.
  DWORD SchedulerThreadProcPrivate();
  CCriticalSection m_SampleQueueLock;
  
private:
  ThreadSafeQueue<IMFSample>  m_ScheduledSamples;    // Samples waiting to be presented.
  Com::SmartPtr<IMFClock>           m_pClock;  // Presentation clock. Can be NULL.
  SchedulerCallback   *m_pCB;     // Weak reference; do not delete.

  DWORD               m_dwThreadID;
  HANDLE              m_hSchedulerThread;
  HANDLE              m_hThreadReadyEvent;
  HANDLE              m_hFlushEvent;

  float               m_fRate;                // Playback rate.
  MFTIME              m_PerFrameInterval;     // Duration of each frame.
  LONGLONG            m_PerFrame_1_4th;       // 1/4th of the frame duration.
  MFTIME              m_LastSampleTime;       // Most recent sample time.
  ScheduleEvent       m_currentEvent; 
};


//-----------------------------------------------------------------------------
// SchedulerCallback
//
// Defines the callback method to present samples. 
//-----------------------------------------------------------------------------

struct SchedulerCallback
{
  virtual HRESULT PresentSample(IMFSample *pSample, LONGLONG llTarget) = 0;
};

#endif