#ifndef __EVENT_SERVER_H__
#define __EVENT_SERVER_H__

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
  class CEventServer : CThread, public IRunnable
  {
  public:
    CEventServer();
    
    // ProcessPacket() -> create client if needed and push to client
    // RefreshClients() -> delete timed out clients

    // IRunnable entry point for thread
    void  Run();

  protected:
    void Cleanup();
    void ProcessPacket(SOCKETS::CAddress& addr, int packetSize);

    std::map<unsigned long, EVENTCLIENT::CEventClient*>  m_clients;
    SOCKETS::CUDPSocket*                    m_pSocket;
    int                                     m_iPort;
    int                                     m_iListenTimeout;
    int                                     m_iMaxClients;
    unsigned char*                          m_pPacketBuffer;   
  };

}

#endif // _LINUX
#endif // __EVENT_SERVER_H__
