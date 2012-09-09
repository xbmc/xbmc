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

#include "DVDPerformanceCounter.h"
#include "DVDMessageQueue.h"
#include "utils/TimeUtils.h"

#include "dvd_config.h"

#ifdef DVDDEBUG_WITH_PERFORMANCE_COUNTER
#include <xbdm.h>
#endif

HRESULT __stdcall DVDPerformanceCounterAudioQueue(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  numerator->QuadPart = 0LL;
  //g_dvdPerformanceCounter.Lock();
  if (g_dvdPerformanceCounter.m_pAudioQueue)
  {
    int iSize     = g_dvdPerformanceCounter.m_pAudioQueue->GetDataSize();
    int iMaxSize  = g_dvdPerformanceCounter.m_pAudioQueue->GetMaxDataSize();
    if (iMaxSize > 0)
    {
      int iPercent  = (iSize * 100) / iMaxSize;
      if (iPercent > 100) iPercent = 100;
      numerator->QuadPart = iPercent;
    }
  }
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

HRESULT __stdcall DVDPerformanceCounterVideoQueue(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  numerator->QuadPart = 0LL;
  //g_dvdPerformanceCounter.Lock();
  if (g_dvdPerformanceCounter.m_pVideoQueue)
  {
    int iSize     = g_dvdPerformanceCounter.m_pVideoQueue->GetDataSize();
    int iMaxSize  = g_dvdPerformanceCounter.m_pVideoQueue->GetMaxDataSize();
    if (iMaxSize > 0)
    {
      int iPercent  = (iSize * 100) / iMaxSize;
      if (iPercent > 100) iPercent = 100;
      numerator->QuadPart = iPercent;
    }
  }
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

inline int64_t get_thread_cpu_usage(ProcessPerformance* p)
{
  if (p->thread)
  {
    ULARGE_INTEGER old_time_thread;
    ULARGE_INTEGER old_time_system;

    old_time_thread.QuadPart = p->timer_thread.QuadPart;
    old_time_system.QuadPart = p->timer_system.QuadPart;

    p->timer_thread.QuadPart = p->thread->GetAbsoluteUsage();
    p->timer_system.QuadPart = CurrentHostCounter();

    int64_t threadTime = (p->timer_thread.QuadPart - old_time_thread.QuadPart);
    int64_t systemTime = (p->timer_system.QuadPart - old_time_system.QuadPart);

    if (systemTime > 0 && threadTime > 0) return ((threadTime * 100) / systemTime);
  }
  return 0LL;
}

HRESULT __stdcall DVDPerformanceCounterVideoDecodePerformance(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  //g_dvdPerformanceCounter.Lock();
  numerator->QuadPart = get_thread_cpu_usage(&g_dvdPerformanceCounter.m_videoDecodePerformance);
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

HRESULT __stdcall DVDPerformanceCounterAudioDecodePerformance(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  //g_dvdPerformanceCounter.Lock();
  numerator->QuadPart = get_thread_cpu_usage(&g_dvdPerformanceCounter.m_audioDecodePerformance);
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

HRESULT __stdcall DVDPerformanceCounterMainPerformance(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  //g_dvdPerformanceCounter.Lock();
  numerator->QuadPart = get_thread_cpu_usage(&g_dvdPerformanceCounter.m_mainPerformance);
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

CDVDPerformanceCounter g_dvdPerformanceCounter;

CDVDPerformanceCounter::CDVDPerformanceCounter()
{
  m_pAudioQueue = NULL;
  m_pVideoQueue = NULL;

  memset(&m_videoDecodePerformance, 0, sizeof(m_videoDecodePerformance)); // video decoding
  memset(&m_audioDecodePerformance, 0, sizeof(m_audioDecodePerformance)); // audio decoding + output to audio device
  memset(&m_mainPerformance,        0, sizeof(m_mainPerformance));        // reading files, demuxing, decoding of subtitles + menu overlays

  Initialize();
}

CDVDPerformanceCounter::~CDVDPerformanceCounter()
{
  DeInitialize();
}

bool CDVDPerformanceCounter::Initialize()
{
  CSingleLock lock(m_critSection);

#ifdef DVDDEBUG_WITH_PERFORMANCE_COUNTER

  DmRegisterPerformanceCounter("DVDAudioQueue",               DMCOUNT_SYNC, DVDPerformanceCounterAudioQueue);
  DmRegisterPerformanceCounter("DVDVideoQueue",               DMCOUNT_SYNC, DVDPerformanceCounterVideoQueue);
  DmRegisterPerformanceCounter("DVDVideoDecodePerformance",   DMCOUNT_SYNC, DVDPerformanceCounterVideoDecodePerformance);
  DmRegisterPerformanceCounter("DVDAudioDecodePerformance",   DMCOUNT_SYNC, DVDPerformanceCounterAudioDecodePerformance);
  DmRegisterPerformanceCounter("DVDMainPerformance",          DMCOUNT_SYNC, DVDPerformanceCounterMainPerformance);

#endif

  return true;
}

void CDVDPerformanceCounter::DeInitialize()
{

}

