#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include "IBroadcastListener.h"
#include "ITransportLayer.h"
#include "Thread.h"

namespace JSONRPC
{
  class CTCPServer : public ITransportLayer, public BROADCAST::IBroadcastListener, public CThread
  {
  public:
    static void StartServer(int port);
    static void StopServer();

    virtual bool CanBroadcast();
    virtual void Broadcast(BROADCAST::EBroadcastFlag flag, std::string message);
  protected:
    void Process();
  private:
    CTCPServer(int port);
    bool Initialize();
    void Deinitialize();

    class CTCPClient : public IClient
    {
    public:
      CTCPClient();
      virtual int  GetPermissionFlags();
      virtual int  GetBroadcastFlags();
      virtual bool SetBroadcastFlags(int flags);
      void PushBuffer(CTCPServer *host, const char *buffer, int length);
      void Disconnect();

      int m_socket;
      struct sockaddr m_cliaddr;
      socklen_t m_addrlen;

    private:
      int m_broadcastcapabilities;
      int m_beginBrackets, m_endBrackets;
      std::string m_buffer;
    };

    std::vector<CTCPClient> m_connections;
    int m_ServerSocket;
    int m_port;

    static CTCPServer *ServerInstance;
  };
}
