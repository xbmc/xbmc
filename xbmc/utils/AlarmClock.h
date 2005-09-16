#pragma once
#include "../lib/common/xbstopwatch.h"
#include "Thread.h"
#include "../stdafx.h"
#include "../../guilib/LocalizeStrings.h"
class CAlarmClock : public CThread
{
public:
	CAlarmClock();
	~CAlarmClock();
	void start(float n_secs, const CStdString& strCommand);
	inline bool isRunning()
	{
		return( m_bIsRunning );
	}
	void stop();
	virtual void OnStartup();
	virtual void OnExit();
	virtual void Process();
private:
	CXBStopWatch watch;
	double m_fSecs;
	bool m_bIsRunning;
  CStdString m_strCommand;
};
extern CAlarmClock g_alarmClock;