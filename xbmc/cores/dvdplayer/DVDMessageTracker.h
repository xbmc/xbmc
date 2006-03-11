
#pragma once

#include "dvd_config.h"

#ifdef DVDDEBUG_MESSAGE_TRACKER

#include "../../utils/thread.h"

class CDVDMsg;

class CDVDMessageTrackerItem
{
public:
  CDVDMessageTrackerItem(CDVDMsg* pMsg)
  {
    m_pMsg = pMsg;
    m_debug_logged = false;
    m_time_created = GetTickCount();
  }
  
  CDVDMsg* m_pMsg;
  bool m_debug_logged;
  DWORD m_time_created;
    
};

class CDVDMessageTracker : private CThread
{
public:
  CDVDMessageTracker();
  ~CDVDMessageTracker();
  
  void Init();
  void DeInit();

  void Register(CDVDMsg* pMsg);
  void UnRegister(CDVDMsg* pMsg);
  
protected:
  void OnStartup();
  void Process();
  void OnExit();
  
private:
  bool m_bInitialized;
  std::list<CDVDMessageTrackerItem*> m_messageList;
  CRITICAL_SECTION m_critSection;
};

extern CDVDMessageTracker g_dvdMessageTracker;

#endif //DVDDEBUG_MESSAGE_TRACKER
