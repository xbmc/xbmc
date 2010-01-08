#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include "IAnnouncer.h"
#include "ITransportLayer.h"
#include "Thread.h"

namespace JSONRPC
{
  class CTCPServer : public ITransportLayer, public ANNOUNCEMENT::IAnnouncer, public CThread
  {
  public:
    static void StartServer(int port, bool nonlocal);
    static void StopServer(bool bWait);

    virtual bool CanAnnounce();
    virtual void Announce(ANNOUNCEMENT::EAnnouncementFlag flag, const char *sender, const char *message, const char *data);
  protected:
    void Process();
  private:
    CTCPServer(int port, bool nonlocal);
    bool Initialize();
    void Deinitialize();

    class CTCPClient : public IClient
    {
    public:
      CTCPClient();
      virtual int  GetPermissionFlags();
      virtual int  GetAnnouncementFlags();
      virtual bool SetAnnouncementFlags(int flags);
      void PushBuffer(CTCPServer *host, const char *buffer, int length);
      void Disconnect();

      int m_socket;
      struct sockaddr m_cliaddr;
      socklen_t m_addrlen;

    private:
      int m_announcementflags;
      int m_beginBrackets, m_endBrackets;
      std::string m_buffer;
    };

    std::vector<CTCPClient> m_connections;
    int m_ServerSocket;
    int m_port;
    bool m_nonlocal;

    static CTCPServer *ServerInstance;
  };
}
