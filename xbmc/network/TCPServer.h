#pragma once
#ifdef _WIN32
#include "WS2tcpip.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <string>
#include <vector>
#include "interfaces/IAnnouncer.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "interfaces/json-rpc/JSONUtils.h"

class CVariant;
namespace JSONRPC
{
  class CTCPServer : public ITransportLayer, public ANNOUNCEMENT::IAnnouncer, public CThread, protected CJSONUtils
  {
  public:
    static bool StartServer(int port, bool nonlocal);
    static void StopServer(bool bWait);

    virtual bool Download(const char *path, Json::Value *result);
    virtual int GetCapabilities();

    virtual void Announce(ANNOUNCEMENT::EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
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
      //Copying a CCriticalSection is not allowed, so copy everything but that
      //when adding a member variable, make sure to copy it in CTCPClient::Copy
      CTCPClient(const CTCPClient& client);
      CTCPClient& operator=(const CTCPClient& client);
      virtual int  GetPermissionFlags();
      virtual int  GetAnnouncementFlags();
      virtual bool SetAnnouncementFlags(int flags);
      void PushBuffer(CTCPServer *host, const char *buffer, int length);
      void Disconnect();

      int m_socket;
      struct sockaddr m_cliaddr;
      socklen_t m_addrlen;
      CCriticalSection m_critSection;

    private:
      void Copy(const CTCPClient& client);
      int m_announcementflags;
      int m_beginBrackets, m_endBrackets;
      char m_beginChar, m_endChar;
      std::string m_buffer;
    };

    std::vector<CTCPClient> m_connections;
    int m_ServerSocket;
    int m_port;
    bool m_nonlocal;

    static CTCPServer *ServerInstance;
  };
}
