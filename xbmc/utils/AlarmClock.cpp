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

#include "AlarmClock.h"
#include "Application.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "log.h"
#include "dialogs/GUIDialogKaiToast.h"

using namespace std;

CAlarmClock::CAlarmClock() : m_bIsRunning(false)
{
}

CAlarmClock::~CAlarmClock()
{
}

void CAlarmClock::Start(const CStdString& strName, float n_secs, const CStdString& strCommand, bool bSilent /* false */, bool bLoop /* false */)
{
  // make lower case so that lookups are case-insensitive
  CStdString lowerName(strName);
  lowerName.ToLower();
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

  CStdString strAlarmClock;
  CStdString strStarted;
  if (strName.CompareNoCase("shutdowntimer") == 0)
  {
    strAlarmClock = g_localizeStrings.Get(20144);
    strStarted = g_localizeStrings.Get(20146);
  }
  else
  {
    strAlarmClock = g_localizeStrings.Get(13208);
    strStarted = g_localizeStrings.Get(13210);
  }

  CStdString strMessage;

  strMessage.Format(strStarted.c_str(),static_cast<int>(event.m_fSecs)/60,static_cast<int>(event.m_fSecs)%60);

  if(!bSilent)
     CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strAlarmClock, strMessage);

  event.watch.StartZero();
  CSingleLock lock(m_events);
  m_event.insert(make_pair(lowerName,event));
  CLog::Log(LOGDEBUG,"started alarm with name: %s",lowerName.c_str());
}

void CAlarmClock::Stop(const CStdString& strName, bool bSilent /* false */)
{
  CSingleLock lock(m_events);

  CStdString lowerName(strName);
  lowerName.ToLower();          // lookup as lowercase only
  map<CStdString,SAlarmClockEvent>::iterator iter = m_event.find(lowerName);

  if (iter == m_event.end())
    return;

  SAlarmClockEvent& event = iter->second;

  CStdString strAlarmClock;
  if (event.m_strCommand.Equals("xbmc.shutdown") || event.m_strCommand.Equals("xbmc.shutdown()"))
    strAlarmClock = g_localizeStrings.Get(20144);
  else
    strAlarmClock = g_localizeStrings.Get(13208);

  CStdString strMessage;
  if( iter->second.watch.GetElapsedSeconds() > iter->second.m_fSecs )
    strMessage = g_localizeStrings.Get(13211);
  else
  {
    float remaining = static_cast<float>(iter->second.m_fSecs-iter->second.watch.GetElapsedSeconds());
    CStdString strStarted = g_localizeStrings.Get(13212);
    strMessage.Format(strStarted.c_str(),static_cast<int>(remaining)/60,static_cast<int>(remaining)%60);
  }
  if (iter->second.m_strCommand.IsEmpty() || iter->second.m_fSecs > iter->second.watch.GetElapsedSeconds())
  {
    if(!bSilent)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strAlarmClock, strMessage);
  }
  else
  {
    g_application.getApplicationMessenger().ExecBuiltIn(iter->second.m_strCommand);
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
    CStdString strLast = "";
    {
      CSingleLock lock(m_events);
      for (map<CStdString,SAlarmClockEvent>::iterator iter=m_event.begin();iter != m_event.end(); ++iter)
        if (iter->second.watch.GetElapsedSeconds() >= iter->second.m_fSecs)
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

