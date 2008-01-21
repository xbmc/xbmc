#include "stdafx.h"
#include "AlarmClock.h"
#include "../Application.h"
#include "../Util.h"

CAlarmClock g_alarmClock;
CAlarmClock::CAlarmClock() : m_bIsRunning(false)
{
}
CAlarmClock::~CAlarmClock()
{
}
void CAlarmClock::start(const CStdString& strName, float n_secs, const CStdString& strCommand)
{
  // make lower case so that lookups are case-insensitive
  CStdString lowerName(strName);
  lowerName.ToLower();
  stop(lowerName);
  SAlarmClockEvent event;
  event.m_fSecs = n_secs;
  event.m_strCommand = strCommand;
  if (!m_bIsRunning)
  {
    StopThread();
    Create();
    m_bIsRunning = true;
  }

  CStdString strAlarmClock;
  CStdString strStarted;
  if (event.m_strCommand.Equals("xbmc.shutdown") || event.m_strCommand.Equals("xbmc.shutdown()"))
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

  strMessage.Format(strStarted.c_str(),static_cast<int>(event.m_fSecs)/60);
  g_application.m_guiDialogKaiToast.QueueNotification(strAlarmClock,strMessage);
  event.watch.StartZero();
  m_event.insert(std::make_pair<CStdString,SAlarmClockEvent>(lowerName,event));
  CLog::Log(LOGDEBUG,"started alarm with name: %s",lowerName.c_str());
}
void CAlarmClock::stop(const CStdString& strName)
{
  CStdString lowerName(strName);
  lowerName.ToLower();          // lookup as lowercase only
  std::map<CStdString,SAlarmClockEvent>::iterator iter = m_event.find(lowerName);

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
    g_application.m_guiDialogKaiToast.QueueNotification(strAlarmClock,strMessage);
  else
    CUtil::ExecBuiltIn(iter->second.m_strCommand);

  iter->second.watch.Stop();
  m_event.erase(iter);
}
void CAlarmClock::Process()
{
  while( !m_bStop)
  {
    CStdString strLast = "";
    for (std::map<CStdString,SAlarmClockEvent>::iterator iter=m_event.begin();iter != m_event.end(); ++iter)
      if (iter->second.watch.GetElapsedSeconds() >= iter->second.m_fSecs)
      {    
        stop(iter->first);
        iter = m_event.find(strLast);
      }
      else
        strLast = iter->first;

    Sleep(100);
  }
}

