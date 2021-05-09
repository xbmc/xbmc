/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Stopwatch.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <map>
#include <string>

struct SAlarmClockEvent
{
  CStopWatch watch;
  double m_fSecs;
  std::string m_strCommand;
  bool m_loop;
};

class CAlarmClock : public CThread
{
public:
  CAlarmClock();
  ~CAlarmClock() override;
  void Start(const std::string& strName, float n_secs, const std::string& strCommand, bool bSilent = false, bool bLoop = false);
  inline bool IsRunning() const
  {
    return m_bIsRunning;
  }

  inline bool HasAlarm(const std::string& strName)
  {
    // note: strName should be lower case only here
    //       No point checking it at the moment due to it only being called
    //       from GUIInfoManager (which is always lowercase)
    //    CLog::Log(LOGDEBUG,"checking for {}",strName);
    return (m_event.find(strName) != m_event.end());
  }

  double GetRemaining(const std::string& strName)
  {
    std::map<std::string,SAlarmClockEvent>::iterator iter;
    if ((iter=m_event.find(strName)) != m_event.end())
    {
      return iter->second.m_fSecs - static_cast<double>(iter->second.watch.IsRunning()
                                                            ? iter->second.watch.GetElapsedSeconds()
                                                            : 0.f);
    }

    return 0.0;
  }

  void Stop(const std::string& strName, bool bSilent = false);
  void Process() override;
private:
  std::map<std::string,SAlarmClockEvent> m_event;
  CCriticalSection m_events;

  bool m_bIsRunning = false;
};

extern CAlarmClock g_alarmClock;

