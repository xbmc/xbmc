// Precise timer routines for WINDOWS
#include <windows.h>
#include <mmsystem.h>
#include "timer.h"
#if 0
/*
// Returns current time in microseconds
unsigned int GetTimer(){
  return timeGetTime() * 1000;
}

// Returns current time in milliseconds
unsigned int GetTimerMS(){
  return timeGetTime() ;
}

int usec_sleep(int usec_delay){
  // Sleep(0) won't sleep for one clocktick as the unix usleep 
  // instead it will only make the thread ready
  // it may take some time until it actually starts to run again
  if(usec_delay<1000)usec_delay=1000;  
  Sleep( usec_delay/1000);
  return 0;
}

	static DWORD RelativeTime = 0;

	float GetRelativeTime(){
		DWORD t, r;
		t = GetTimer();
		r = t - RelativeTime;
		RelativeTime = t;
		return (float) r *0.000001F;
	}

	void InitTimer(){
		GetRelativeTime();
	}
*/
#endif

static unsigned long	RelativeTime=0;
FLOAT									m_fuSecsPerTick;
FLOAT									fLastTime = 0.0f;
LARGE_INTEGER         m_lStartTime;


// Returns current time in microseconds
extern unsigned long GetTimer()
{
	LARGE_INTEGER qwTime ;
	FLOAT					fTime;
	UINT64				uiQuadPart;
	QueryPerformanceCounter( &qwTime );
	qwTime.QuadPart -= m_lStartTime.QuadPart;
	uiQuadPart  =(UINT64)qwTime.QuadPart;
	uiQuadPart /= ((UINT64)10);  // prevent overflow after 4294.1 secs, now overflows after 42941 secs
	fTime = ((FLOAT)(uiQuadPart)) / m_fuSecsPerTick ;
	return (unsigned long)fTime;	
}  


// Returns current time in microseconds
float GetRelativeTime()
{
	unsigned long t,r;
  t=GetTimer();
  r=t-RelativeTime;
  RelativeTime=t;
  return (float)r * 0.000001F;
}

void InitTimer()
{
  LARGE_INTEGER qwTicksPerSec;
	FLOAT t;
  QueryPerformanceFrequency( &qwTicksPerSec );   // ticks/sec
  m_fuSecsPerTick = (FLOAT)(((FLOAT)(qwTicksPerSec.QuadPart))  /1000.0);			 // tics/msec
	m_fuSecsPerTick = (FLOAT)(m_fuSecsPerTick/1000.0);			 // tics/usec
	m_fuSecsPerTick/=10.0;
	QueryPerformanceCounter( &m_lStartTime );
	t=GetRelativeTime();
}

int usec_sleep(int usec_delay)
{
  if(usec_delay<1000) usec_delay=1000;  
  Sleep( usec_delay/1000);

  return 0;
}

unsigned long GetTimerMS()
{
  return GetTimer()/1000 ;
}

