#ifndef __EVENT_SERVER_H__
#define __EVENT_SERVER_H__

#include "system.h"
#include "Thread.h"
#include "Socket.h"
#include "EventClient.h"
#include "CriticalSection.h"
#include <map>
#include <queue>

namespace EVENTSERVER
{

  /**********************************************************************/
  /* UDP Event Server Class                                             */
  /**********************************************************************/
  class CEventServer : public IRunnable
  {
  public:
    static CEventServer* GetInstance();
    virtual ~CEventServer() {}

    // IRunnable entry point for thread
    virtual void  Run();

    bool Running()
    {
      return m_bRunning;
    }

    void RefreshSettings()
    {
      CSingleLock lock(m_critSection);
      m_bRefreshSettings = true;
    }

    // start / stop server
    void StartServer();
    void StopServer();
    
    // get events
    unsigned short GetButtonCode(std::string& strMapName);
    bool GetMousePos(float &x, float &y);

  protected:
    CEventServer();
    void Cleanup();
    void ProcessPacket(SOCKETS::CAddress& addr, int packetSize);
    void ExecuteEvents();
    void RefreshClients();

    std::map<unsigned long, EVENTCLIENT::CEventClient*>  m_clients;
    CThread*             m_pThread;
    static CEventServer* m_pInstance;
    SOCKETS::CUDPSocket* m_pSocket;
    int              m_iPort;
    int              m_iListenTimeout;
    int              m_iMaxClients;
    unsigned char*   m_pPacketBuffer;
    bool             m_bStop;
    bool             m_bRunning;
    CCriticalSection m_critSection;
    bool             m_bRefreshSettings;
  };

}

#endif // __EVENT_SERVER_H__
