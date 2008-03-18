
#pragma once

#define FILETIME_TO_ULARGE_INTEGER(ularge, filetime) { ularge.u.HighPart = filetime.dwHighDateTime; ularge.u.LowPart = filetime.dwLowDateTime; }

class CDVDMessageQueue;

typedef struct stProcessPerformance
{
  ULARGE_INTEGER  timer_thread;
  ULARGE_INTEGER  timer_system;
  HANDLE          hThread;
  DWORD           iPercent; // in percent
} ProcessPerformance;

class CDVDPerformanceCounter
{
public:
  CDVDPerformanceCounter();
  ~CDVDPerformanceCounter();
  
  bool Initialize();
  void DeInitialize();
  
  void Lock()                                       { EnterCriticalSection(&m_critSection); }
  void Unlock()                                     { LeaveCriticalSection(&m_critSection); }
  
  void EnableAudioQueue(CDVDMessageQueue* pQueue)   { Lock(); m_pAudioQueue = pQueue; Unlock(); }
  void DisableAudioQueue()                          { Lock(); m_pAudioQueue = NULL; Unlock(); }

  void EnableVideoQueue(CDVDMessageQueue* pQueue)   { Lock(); m_pVideoQueue = pQueue; Unlock(); }
  void DisableVideoQueue()                          { Lock(); m_pVideoQueue = NULL; Unlock(); }
  
  void EnableVideoDecodePerformance(HANDLE hThread) { Lock(); m_videoDecodePerformance.hThread = hThread; Unlock(); }
  void DisableVideoDecodePerformance()              { Lock(); m_videoDecodePerformance.hThread = NULL; Unlock(); }

  void EnableAudioDecodePerformance(HANDLE hThread) { Lock(); m_audioDecodePerformance.hThread = hThread; Unlock(); }
  void DisableAudioDecodePerformance()              { Lock(); m_audioDecodePerformance.hThread = NULL; Unlock(); }

  void EnableMainPerformance(HANDLE hThread)        { Lock(); m_mainPerformance.hThread = hThread; Unlock(); }
  void DisableMainPerformance()                     { Lock(); m_mainPerformance.hThread = NULL; Unlock(); }

  CDVDMessageQueue*         m_pAudioQueue;
  CDVDMessageQueue*         m_pVideoQueue;
  
  ProcessPerformance        m_videoDecodePerformance;
  ProcessPerformance        m_audioDecodePerformance;
  ProcessPerformance        m_mainPerformance;
  
private:
  CRITICAL_SECTION m_critSection;
};

extern CDVDPerformanceCounter g_dvdPerformanceCounter;

