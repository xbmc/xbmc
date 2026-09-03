/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/IJSONRPCAnnouncer.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "websocket/WebSocket.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <sys/socket.h>

#include "PlatformDefs.h"

class CVariant;

namespace JSONRPC
{
  class CTCPServer : public ITransportLayer, public JSONRPC::IJSONRPCAnnouncer, public CThread
  {
  public:
    static bool StartServer(int port, bool nonlocal);
    static void StopServer(bool bWait);
    static bool IsRunning();

    bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) override;
    bool Download(const char *path, CVariant &result) override;
    int GetCapabilities() override;

    void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const CVariant& data) override;

  protected:
    void Process() override;
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
      // worker state is deliberately not copied
      CTCPClient(const CTCPClient& client);
      CTCPClient& operator=(const CTCPClient& client);
      ~CTCPClient() override = default;

      int GetPermissionFlags() override;
      int GetAnnouncementFlags() override;
      bool SetAnnouncementFlags(int flags) override;

      virtual void Send(const char *data, unsigned int size);
      virtual void PushBuffer(CTCPServer *host, const char *buffer, int length);
      virtual void Disconnect();

      /*!
       * \brief Hand a received buffer to this connection's own thread, starting it if needed.
       *
       * The server thread never executes a request itself: a handler may block in a modal
       * dialog, and every other client would wait behind it.
       *
       * \param self shared ownership of this client, kept alive by the worker
       * \param host the server, kept alive by the worker through its own shared_ptr
       */
      void Enqueue(const std::shared_ptr<CTCPClient>& self,
                   CTCPServer* host,
                   const char* buffer,
                   int length);

      /*!
       * \brief Ask the worker to finish; never joins, as the worker may be blocked in a dialog.
       */
      void StopWorker();

      virtual bool IsNew() const { return m_new; }
      virtual bool Closing() const { return m_closing; }

      /*!
       * \brief Whether this connection has more accepted but unparsed input than it should.
       *
       * The server thread stops reading a backlogged connection, which closes the receive
       * window and holds the client back. Running the request on the server thread used to do
       * that on its own.
       */
      bool Backlogged();

      SOCKET m_socket{INVALID_SOCKET};
      sockaddr_storage m_cliaddr;
      socklen_t m_addrlen;
      CCriticalSection m_critSection;

    protected:
      void Copy(const CTCPClient& client);

      /*!
       * \brief Ask the server thread to drop this connection.
       *
       * Only the server thread closes a socket: it may be in select() on that descriptor.
       */
      void RequestClose() { m_closing = true; }

    private:
      static void RunWorker(std::shared_ptr<CTCPClient> self, std::shared_ptr<CTCPServer> host);
      static void RunRequests(const std::shared_ptr<CTCPClient>& self, CTCPServer* host);

      bool m_new;
      int m_announcementflags;
      int m_beginBrackets, m_endBrackets;
      char m_beginChar, m_endChar;
      std::string m_buffer;

      std::atomic<bool> m_closing{false};

      std::mutex m_inboundMutex;
      std::condition_variable m_inboundEvent;
      std::deque<std::string> m_inbound;
      size_t m_inboundBytes{0};
      bool m_workerStop{false};
      bool m_workerStarted{false};
    };

    class CWebSocketClient : public CTCPClient
    {
    public:
      explicit CWebSocketClient(CWebSocket *websocket);
      CWebSocketClient(const CWebSocketClient& client);
      CWebSocketClient(CWebSocket *websocket, const CTCPClient& client);
      CWebSocketClient& operator=(const CWebSocketClient& client);
      ~CWebSocketClient() override;

      void Send(const char *data, unsigned int size) override;
      void PushBuffer(CTCPServer *host, const char *buffer, int length) override;
      void Disconnect() override;

      bool IsNew() const override { return m_websocket == NULL; }

    private:
      CWebSocket *m_websocket;
      std::string m_buffer;
    };

    std::vector<std::shared_ptr<CTCPClient>> m_connections;
    std::vector<SOCKET> m_servers;
    CCriticalSection m_connectionsCritSection;
    int m_port;
    bool m_nonlocal;
    void* m_sdpd;

    // Handed to each worker, so a request still running when StopServer() drops ServerInstance
    // keeps the server alive until it returns.
    std::weak_ptr<CTCPServer> m_self;

    //! \brief Wait for every worker to finish, for at most the given time
    void WaitForWorkers(std::chrono::milliseconds timeout);

    std::mutex m_workersMutex;
    std::condition_variable m_workersDone;
    unsigned int m_activeWorkers{0};

    static std::shared_ptr<CTCPServer> ServerInstance;
  };
}
