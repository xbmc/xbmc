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

#include "AlarmClock.h"
#include "ApplicationMessenger.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "log.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/StringUtils.h"

using namespace std;

CAlarmClock::CAlarmClock() : CThread("AlarmClock"), m_bIsRunning(false)
{
}

CAlarmClock::~CAlarmClock()
{
}

void CAlarmClock::Start(const std::string& strName, float n_secs, const std::string& strCommand, bool bSilent /* false */, bool bLoop /* false */)
{
  // make lower case so that lookups are case-insensitive
  std::string lowerName(strName);
  StringUtils::ToLower(lowerName);
  Stop(lowerName);
  SAlarmClockEvent event;
  event.m_fSecs = n_secs;
  event.m_strCommand = strCommand;
  event.m_loop = bLoop;
  if (!m_bIsRunning)
  {
    StopThread();
    Create();
    m_bIsRunning = true;
  }

  std::string strAlarmClock;
  std::string strStarted;
  if (StringUtils::EqualsNoCase(strName, "shutdowntimer"))
  {
    strAlarmClock = g_localizeStrings.Get(20144);
    strStarted = g_localizeStrings.Get(20146);
  }
  else
  {
    strAlarmClock = g_localizeStrings.Get(13208);
    strStarted = g_localizeStrings.Get(13210);
  }

  std::string strMessage = StringUtils::Format(strStarted.c_str(),
                                              static_cast<int>(event.m_fSecs)/60,
                                              static_cast<int>(event.m_fSecs)%60);

  if(!bSilent)
     CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strAlarmClock, strMessage);

  event.watch.StartZero();
  CSingleLock lock(m_events);
  m_event.insert(make_pair(lowerName,event));
  CLog::Log(LOGDEBUG,"started alarm with name: %s",lowerName.c_str());
}

void CAlarmClock::Stop(const std::string& strName, bool bSilent /* false */)
{
  CSingleLock lock(m_events);

  std::string lowerName(strName);
  StringUtils::ToLower(lowerName);          // lookup as lowercase only
  map<std::string,SAlarmClockEvent>::iterator iter = m_event.find(lowerName);

  if (iter == m_event.end())
    return;

  std::string strAlarmClock;
  if (StringUtils::EqualsNoCase(strName, "shutdowntimer"))
    strAlarmClock = g_localizeStrings.Get(20144);
  else
    strAlarmClock = g_localizeStrings.Get(13208);

  std::string strMessage;
  float       elapsed     = 0.f;

  if (iter->second.watch.IsRunning())
    elapsed = iter->second.watch.GetElapsedSeconds();

  if( elapsed > iter->second.m_fSecs )
    strMessage = g_localizeStrings.Get(13211);
  else
  {
    float remaining = static_cast<float>(iter->second.m_fSecs-elapsed);
    std::string strStarted = g_localizeStrings.Get(13212);
    strMessage = StringUtils::Format(strStarted.c_str(),
                                     static_cast<int>(remaining)/60,
                                     static_cast<int>(remaining)%60);
  }
  if (iter->second.m_strCommand.empty() || iter->second.m_fSecs > elapsed)
  {
    if(!bSilent)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strAlarmClock, strMessage);
  }
  else
  {
    CApplicationMessenger::Get().ExecBuiltIn(iter->second.m_strCommand);
    if (iter->second.m_loop)
    {
      iter->second.watch.Reset();
      return;
    }
  }

  iter->second.watch.Stop();
  m_event.erase(iter);
}

void CAlarmClock::Process()
{
  while( !m_bStop)
  {
    std::string strLast;
    {
      CSingleLock lock(m_events);
      for (map<std::string,SAlarmClockEvent>::iterator iter=m_event.begin();iter != m_event.end(); ++iter)
        if ( iter->second.watch.IsRunning()
          && iter->second.watch.GetElapsedSeconds() >= iter->second.m_fSecs)
        {
          Stop(iter->first);
          if ((iter = m_event.find(strLast)) == m_event.end())
            break;
        }
        else
          strLast = iter->first;
    }
    Sleep(100);
  }
}

