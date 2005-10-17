#include "AlarmClock.h"
#include "../application.h"
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
  stop(strName);
  SAlarmClockEvent event;
  event.m_fSecs = n_secs;
  event.m_strCommand = strCommand;
  if (!m_bIsRunning)
  {
    StopThread();
    Create();
    m_bIsRunning = true;
  }
  CStdString strAlarmClock = g_localizeStrings.Get(13208);
  CStdString strMessage;
  CStdString strStarted = g_localizeStrings.Get(13210);
  strMessage.Format(strStarted.c_str(),static_cast<int>(event.m_fSecs)/60);
  g_application.m_guiDialogKaiToast.QueueNotification(strAlarmClock,strMessage);
  event.watch.StartZero();
  m_event.insert(std::make_pair<CStdString,SAlarmClockEvent>(strName,event));
  CLog::Log(LOGDEBUG,"started alarm with name: %s",strName.c_str());
}
void CAlarmClock::stop(const CStdString& strName)
{
  std::map<CStdString,SAlarmClockEvent>::iterator iter = m_event.find(strName);

  if (iter == m_event.end())
    return;

  CStdString strAlarmClock = g_localizeStrings.Get(13208);
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