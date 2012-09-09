#pragma once

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

#include "StdString.h"
#include "Stopwatch.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <map>

struct SAlarmClockEvent
{
  CStopWatch watch;
  double m_fSecs;
  CStdString m_strCommand;
  bool m_loop;
};

class CAlarmClock : public CThread
{
public:
  CAlarmClock();
  ~CAlarmClock();
  void Start(const CStdString& strName, float n_secs, const CStdString& strCommand, bool bSilent = false, bool bLoop = false);
  inline bool IsRunning() const
  {
    return m_bIsRunning;
  }

  inline bool HasAlarm(const CStdString& strName)
  {
    // note: strName should be lower case only here
    //       No point checking it at the moment due to it only being called
    //       from GUIInfoManager (which is always lowercase)
    //    CLog::Log(LOGDEBUG,"checking for %s",strName.c_str());
    return (m_event.find(strName) != m_event.end());
  }

  double GetRemaining(const CStdString& strName)
  {
    std::map<CStdString,SAlarmClockEvent>::iterator iter;
    if ((iter=m_event.find(strName)) != m_event.end())
    {
      return iter->second.m_fSecs-iter->second.watch.GetElapsedSeconds();
    }

    return 0.f;
  }

  void Stop(const CStdString& strName, bool bSilent = false);
  virtual void Process();
private:
  std::map<CStdString,SAlarmClockEvent> m_event;
  CCriticalSection m_events;

  bool m_bIsRunning;
};

extern CAlarmClock g_alarmClock;

