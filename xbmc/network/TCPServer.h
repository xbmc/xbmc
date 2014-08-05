#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>
#include <sys/socket.h>

#include "system.h"
#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/IJSONRPCAnnouncer.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "websocket/WebSocket.h"

namespace JSONRPC
{
  class CTCPServer : public ITransportLayer, public JSONRPC::IJSONRPCAnnouncer, public CThread
  {
  public:
    static bool StartServer(int port, bool nonlocal);
    static void StopServer(bool bWait);
    static bool IsRunning();

    virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol);
    virtual bool Download(const char *path, CVariant &result);
    virtual int GetCapabilities();

    virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
  protected:
    void Process();
  private:
    CTCPServer(int port, bool nonlocal);
    bool Initialize();
    bool InitializeBlue();
    bool InitializeTCP();
    void Deinitialize();

    class CTCPClient : public IClient
    {
    public:
      CTCPClient();
      //Copying a CCriticalSection is not allowed, so copy everything but that
      //when adding a member variable, make sure to copy it in CTCPClient::Copy
      CTCPClient(const CTCPClient& client);
      CTCPClient& operator=(const CTCPClient& client);
      virtual ~CTCPClient() { };

      virtual int  GetPermissionFlags();
      virtual int  GetAnnouncementFlags();
      virtual bool SetAnnouncementFlags(int flags);

      virtual void Send(const char *data, unsigned int size);
      virtual void PushBuffer(CTCPServer *host, const char *buffer, int length);
      virtual void Disconnect();

      virtual bool IsNew() const { return m_new; }
      virtual bool Closing() const { return false; }

      SOCKET           m_socket;
      sockaddr_storage m_cliaddr;
      socklen_t        m_addrlen;
      CCriticalSection m_critSection;

    protected:
      void Copy(const CTCPClient& client);
    private:
      bool m_new;
      int m_announcementflags;
      int m_beginBrackets, m_endBrackets;
      char m_beginChar, m_endChar;
      std::string m_buffer;
    };

    class CWebSocketClient : public CTCPClient
    {
    public:
      CWebSocketClient(CWebSocket *websocket);
      CWebSocketClient(const CWebSocketClient& client);
      CWebSocketClient(CWebSocket *websocket, const CTCPClient& client);
      CWebSocketClient& operator=(const CWebSocketClient& client);
      ~CWebSocketClient();

      virtual void Send(const char *data, unsigned int size);
      virtual void PushBuffer(CTCPServer *host, const char *buffer, int length);
      virtual void Disconnect();

      virtual bool IsNew() const { return m_websocket == NULL; }
      virtual bool Closing() const { return m_websocket != NULL && m_websocket->GetState() == WebSocketStateClosed; }

    private:
      CWebSocket *m_websocket;
    };

    std::vector<CTCPClient*> m_connections;
    std::vector<SOCKET> m_servers;
    int m_port;
    bool m_nonlocal;
    void* m_sdpd;

    static CTCPServer *ServerInstance;
  };
}
