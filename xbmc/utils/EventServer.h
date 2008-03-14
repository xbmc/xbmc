#ifndef __EVENT_SERVER_H__
#define __EVENT_SERVER_H__

#include "system.h"
#include "include.h"
#include "Thread.h"
#include "Socket.h"
#include "EventClient.h"
#include <map>
#include <queue>

#ifdef _LINUX

namespace EVENTSERVER
{

  /**********************************************************************/
  /* UDP Event Server Class                                             */
  /**********************************************************************/
  class CEventServer : public IRunnable
  {
  public:
    static CEventServer* GetInstance();
    // ProcessPacket() -> create client if needed and push to client
    // RefreshClients() -> delete timed out clients

    // IRunnable entry point for thread
    virtual void  Run();
    bool          Running()
    {
      return m_bRunning;
    }

    // start / stop server
    void StartServer();
    void StopServer();

  protected:
    CEventServer();
    void Cleanup();
    void ProcessPacket(SOCKETS::CAddress& addr, int packetSize);
    void ExecuteEvents();

    CThread*                                m_pThread;
    static CEventServer*                    m_pInstance;
    std::map<unsigned long, EVENTCLIENT::CEventClient*>  m_clients;
    SOCKETS::CUDPSocket*                    m_pSocket;
    int                                     m_iPort;
    int                                     m_iListenTimeout;
    int                                     m_iMaxClients;
    unsigned char*                          m_pPacketBuffer;
    bool                                    m_bStop;
    bool                                    m_bRunning;
  };

}

#endif // _LINUX
#endif // __EVENT_SERVER_H__
