#include "AlarmClock.h"
#include "../application.h"

CAlarmClock g_alarmClock;

CAlarmClock::CAlarmClock() : m_bIsRunning(false), m_fSecs(0)
{
}

CAlarmClock::~CAlarmClock()
{
}

void CAlarmClock::start(float n_secs)
{
	StopThread();
	m_fSecs = n_secs;
	Create();
}

void CAlarmClock::stop()
{
	CLog::Log(LOGDEBUG,"stopping timer!");
	if( m_bIsRunning ) 
		StopThread();
}

void CAlarmClock::OnStartup()
{
	CLog::Log(LOGDEBUG,"starting watch");
	watch.StartZero();
	CStdString strAlarmClock = g_localizeStrings.Get(13208);
	CStdString strMessage;
	CStdString strCanceled = g_localizeStrings.Get(13210);
	strMessage.Format(strCanceled.c_str(),static_cast<int>(m_fSecs)/60);
	g_application.m_guiDialogKaiToast.QueueNotification(strAlarmClock,strMessage);
	m_bIsRunning = true;
}

void CAlarmClock::OnExit()
{
	CLog::Log(LOGDEBUG,"timer onexit");
	CStdString strAlarmClock = g_localizeStrings.Get(13208);
	CStdString strMessage;
	if( watch.GetElapsedSeconds() > m_fSecs )
		strMessage = g_localizeStrings.Get(13211);
	else {
		float remaining = static_cast<float>(m_fSecs-watch.GetElapsedSeconds());
		CStdString strStarted = g_localizeStrings.Get(13212);
		strMessage.Format(strStarted.c_str(),static_cast<int>(remaining)/60,static_cast<int>(remaining)%60);
	}

	g_application.m_guiDialogKaiToast.QueueNotification(strAlarmClock,strMessage);
	watch.Stop();
	m_bIsRunning = false;
}

void CAlarmClock::Process()
{
	while( (watch.GetElapsedSeconds() < m_fSecs) && (!m_bStop) ) ;
}
