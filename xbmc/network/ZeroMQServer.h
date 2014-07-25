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
#include <zmq.hpp>

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/IJSONRPCAnnouncer.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "websocket/WebSocket.h"

namespace JSONRPC
{
  class CZeroMQServer : public ITransportLayer, public JSONRPC::IJSONRPCAnnouncer, public CThread
  {
  public:
    static bool StartServer(int port);
    static void StopServer(bool bWait);
    static bool IsRunning();

    virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol);
    virtual bool Download(const char *path, CVariant &result);
    virtual int GetCapabilities();

    virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
  protected:
    void Process();
  private:
    CZeroMQServer(int port);

    bool Initialize();
    void Deinitialize();

    class CZeroMQClient : public IClient
    {
    public:
      CZeroMQClient();
      //Copying a CCriticalSection is not allowed, so copy everything but that
      //when adding a member variable, make sure to copy it in CTCPClient::Copy
      CZeroMQClient(const CZeroMQClient& client);
      CZeroMQClient& operator=(const CZeroMQClient& client);
      virtual ~CZeroMQClient() { };

      virtual int  GetPermissionFlags();
      virtual int  GetAnnouncementFlags();
      virtual bool SetAnnouncementFlags(int flags);

      virtual bool IsNew() const { return m_new; }
      virtual bool Closing() const { return false; }

    protected:
      void Copy(const CZeroMQClient& client);
    private:
      bool m_new;
      int m_announcementflags;
    };

    unsigned int m_port;
    zmq::context_t m_context;
    zmq::socket_t m_repSocket;
    zmq::socket_t m_pubSocket;

    std::vector<CZeroMQClient*> m_connections;
    static CZeroMQServer *ServerInstance;
  };
}
