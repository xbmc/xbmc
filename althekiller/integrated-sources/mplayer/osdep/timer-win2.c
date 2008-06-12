// Precise timer routines for WINDOWS

#include <windows.h>
#include <mmsystem.h>
#include "timer.h"
#ifdef _XBOX
const char *timer_name = "XBOX High Resolution Timers for";
LARGE_INTEGER lPerfFreq;
//extern bool QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);


void InitTimer()
{
  QueryPerformanceFrequency(&lPerfFreq);
  GetRelativeTime(); //Needed to set RelativeTime correctly as it will give invalid value otherwise on first run.
}

unsigned int GetTimer()
{
  LARGE_INTEGER lCount;
  QueryPerformanceCounter(&lCount);
  return ((unsigned int)(lCount.QuadPart * (__int64)1000000 / lPerfFreq.QuadPart));
}

unsigned int GetTimerMS()
{
  LARGE_INTEGER lCount;
  QueryPerformanceCounter(&lCount);
  return ((unsigned int)(lCount.QuadPart * (__int64)1000 / lPerfFreq.QuadPart));
}

static __int64 RelativeTime = 0;

float GetRelativeTime(){
  LARGE_INTEGER t;
  __int64 r;
  QueryPerformanceCounter(&t);
  r = t.QuadPart - RelativeTime;
  RelativeTime = t.QuadPart;
  return (float) r / (float) lPerfFreq.QuadPart;
}

int usec_sleep(int usec_delay){
  // Sleep(0) won't sleep for one clocktick as the unix usleep 
  // instead it will only make the thread ready
  // it may take some time until it actually starts to run again
  if(usec_delay<1000)usec_delay=1000;  
  Sleep( usec_delay/1000);
  return 0;
}

#else
const char *timer_name = "Windows native";

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
#endif
