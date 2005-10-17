#pragma once
#include "../lib/common/xbstopwatch.h"
#include "Thread.h"
#include "../stdafx.h"
#include "../../guilib/LocalizeStrings.h"
struct SAlarmClockEvent
{
  CXBStopWatch watch;
	double m_fSecs;
  CStdString m_strCommand;
};

class CAlarmClock : public CThread
{
public:
	CAlarmClock();
	~CAlarmClock();
	void start(const CStdString& strName, float n_secs, const CStdString& strCommand);
	inline bool isRunning()
	{
		return( m_bIsRunning );
	}

  inline bool hasAlarm(const CStdString& strName)
  {
    CLog::Log(LOGDEBUG,"checking for %s",strName.c_str());
    return (m_event.find(strName) != m_event.end());
  }

	void stop(const CStdString& strName);
	virtual void Process();
private:
  std::map<CStdString,SAlarmClockEvent> m_event;
  
  bool m_bIsRunning;
};
extern CAlarmClock g_alarmClock;