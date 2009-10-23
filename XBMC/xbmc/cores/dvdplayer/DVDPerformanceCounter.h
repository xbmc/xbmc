#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#define FILETIME_TO_ULARGE_INTEGER(ularge, filetime) { ularge.u.HighPart = filetime.dwHighDateTime; ularge.u.LowPart = filetime.dwLowDateTime; }

#include "system.h"

class CDVDMessageQueue;

typedef struct stProcessPerformance
{
  ULARGE_INTEGER  timer_thread;
  ULARGE_INTEGER  timer_system;
  HANDLE          hThread;
} ProcessPerformance;

class CDVDPerformanceCounter
{
public:
  CDVDPerformanceCounter();
  ~CDVDPerformanceCounter();

  bool Initialize();
  void DeInitialize();

  void Lock()                                       { EnterCriticalSection(&m_critSection); }
  void Unlock()                                     { LeaveCriticalSection(&m_critSection); }

  void EnableAudioQueue(CDVDMessageQueue* pQueue)   { Lock(); m_pAudioQueue = pQueue; Unlock(); }
  void DisableAudioQueue()                          { Lock(); m_pAudioQueue = NULL; Unlock(); }

  void EnableVideoQueue(CDVDMessageQueue* pQueue)   { Lock(); m_pVideoQueue = pQueue; Unlock(); }
  void DisableVideoQueue()                          { Lock(); m_pVideoQueue = NULL; Unlock(); }

  void EnableVideoDecodePerformance(HANDLE hThread) { Lock(); m_videoDecodePerformance.hThread = hThread; Unlock(); }
  void DisableVideoDecodePerformance()              { Lock(); m_videoDecodePerformance.hThread = NULL; Unlock(); }

  void EnableAudioDecodePerformance(HANDLE hThread) { Lock(); m_audioDecodePerformance.hThread = hThread; Unlock(); }
  void DisableAudioDecodePerformance()              { Lock(); m_audioDecodePerformance.hThread = NULL; Unlock(); }

  void EnableMainPerformance(HANDLE hThread)        { Lock(); m_mainPerformance.hThread = hThread; Unlock(); }
  void DisableMainPerformance()                     { Lock(); m_mainPerformance.hThread = NULL; Unlock(); }

  CDVDMessageQueue*         m_pAudioQueue;
  CDVDMessageQueue*         m_pVideoQueue;

  ProcessPerformance        m_videoDecodePerformance;
  ProcessPerformance        m_audioDecodePerformance;
  ProcessPerformance        m_mainPerformance;

private:
  CRITICAL_SECTION m_critSection;
};

extern CDVDPerformanceCounter g_dvdPerformanceCounter;

