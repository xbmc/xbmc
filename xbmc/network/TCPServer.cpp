/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TCPServer.h"

#include "ServiceBroker.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "network/Network.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "websocket/WebSocketManager.h"

#include <memory>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <utility>

#include <arpa/inet.h>
#include <memory.h>
#include <netinet/in.h>

using namespace std::chrono_literals;

#if defined(TARGET_WINDOWS) || defined(HAVE_LIBBLUETOOTH)
static const char     bt_service_name[] = "XBMC JSON-RPC";
static const char     bt_service_desc[] = "Interface for XBMC remote control over bluetooth";
static const char     bt_service_prov[] = "XBMC JSON-RPC Provider";
static const uint32_t bt_service_guid[] = {0x65AE4CC0, 0x775D11E0, 0xBE16CE28, 0x4824019B};
#endif

#if defined(TARGET_WINDOWS)
#include "platform/win32/CharsetConverter.h"
#endif

#ifdef HAVE_LIBBLUETOOTH
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

 /* The defines BDADDR_ANY and BDADDR_LOCAL are broken so use our own structs */
static const bdaddr_t bt_bdaddr_any   = {{0, 0, 0, 0, 0, 0}};
static const bdaddr_t bt_bdaddr_local = {{0, 0, 0, 0xff, 0xff, 0xff}};

#endif

using namespace JSONRPC;

#define RECEIVEBUFFER 4096

namespace
{
constexpr size_t maxBufferLength = 64 * 1024;
}

std::shared_ptr<CTCPServer> CTCPServer::ServerInstance;

bool CTCPServer::StartServer(int port, bool nonlocal)
{
  StopServer(true);

  // The constructor is private, so make_shared cannot reach it.
  ServerInstance = std::shared_ptr<CTCPServer>(new CTCPServer(port, nonlocal));
  ServerInstance->m_self = ServerInstance;
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create(false);
    return true;
  }
  else
    return false;
}

void CTCPServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      // Deinitialize has told every worker to stop; one inside a modal dialog may not return
      ServerInstance->WaitForWorkers(std::chrono::seconds(2));
      ServerInstance.reset();
    }
  }
}

bool CTCPServer::IsRunning()
{
  if (!ServerInstance)
    return false;

  return ServerInstance->CThread::IsRunning();
}

CTCPServer::CTCPServer(int port, bool nonlocal) : CThread("TCPServer")
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_sdpd = NULL;
}

void CTCPServer::Process()
{
  m_bStop = false;

  while (!m_bStop)
  {
    SOCKET          max_fd = 0;
    fd_set          rfds;
    struct timeval  to     = {1, 0};
    bool backlogged = false;
    FD_ZERO(&rfds);

    {
      // Build the fd_set under lock so a concurrent Announce or modification
      // of m_connections doesn't tear out from under us. Drop the lock for
      // the select() call below since it sleeps up to 1s and would otherwise
      // starve Announce.
      std::unique_lock lock(m_connectionsCritSection);

      for (auto& it : m_servers)
      {
        FD_SET(it, &rfds);
        if ((intptr_t)it > (intptr_t)max_fd)
          max_fd = it;
      }

      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        if (m_connections[i]->Closing())
        {
          CLog::Log(LOGINFO, "JSONRPC Server: Disconnection requested");
          m_connections[i]->StopWorker();
          m_connections[i]->Disconnect();
          m_connections.erase(m_connections.begin() + i);
          continue;
        }

        // Reading faster than the worker parses would queue without limit, so a connection
        // that is far enough ahead is left unread until the worker catches up. Its receive
        // window closes and the client waits, which is the backpressure that running the
        // request on this thread used to provide. Nothing is refused, and a request of any
        // size still arrives - at the rate it can be consumed.
        if (m_connections[i]->Backlogged())
        {
          backlogged = true;
          continue;
        }

        FD_SET(m_connections[i]->m_socket, &rfds);
        if ((intptr_t)m_connections[i]->m_socket > (intptr_t)max_fd)
          max_fd = m_connections[i]->m_socket;
      }
    }

    // A held-back connection is not in the set, so nothing wakes the select when its worker
    // drains. Look again soon rather than after the idle timeout.
    if (backlogged)
      to = {0, 50000};

    int res = select((intptr_t)max_fd+1, &rfds, NULL, NULL, &to);
    if (res < 0)
    {
      CLog::Log(LOGERROR, "JSONRPC Server: Select failed");
      CThread::Sleep(1000ms);
      Initialize();
    }
    else if (res > 0)
    {
      // Re-acquire for the I/O and accept passes; both modify m_connections.
      std::unique_lock lock(m_connectionsCritSection);

      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        int socket = m_connections[i]->m_socket;
        if (FD_ISSET(socket, &rfds))
        {
          char buffer[RECEIVEBUFFER] = {};
          int  nread = 0;
          nread = recv(socket, (char*)&buffer, RECEIVEBUFFER, 0);
          bool close = false;
          if (nread > 0)
          {
            std::string response;
            if (m_connections[i]->IsNew())
            {
              CWebSocket *websocket = CWebSocketManager::Handle(buffer, nread, response);

              if (!response.empty())
                m_connections[i]->Send(response.c_str(), response.size());

              if (websocket != NULL)
              {
                // Replace the CTCPClient with a CWebSocketClient
                m_connections[i] =
                    std::make_shared<CWebSocketClient>(websocket, *(m_connections[i]));
              }
            }

            if (response.empty())
            {
              m_connections[i]->Enqueue(m_connections[i], this, buffer, nread);
            }
          }
          else
            close = true;

          if (close)
          {
            CLog::Log(LOGINFO, "JSONRPC Server: Disconnection detected");
            m_connections[i]->StopWorker();
            m_connections[i]->Disconnect();
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      for (auto& it : m_servers)
      {
        if (FD_ISSET(it, &rfds))
        {
          CLog::Log(LOGDEBUG, "JSONRPC Server: New connection detected");
          auto newconnection = std::make_shared<CTCPClient>();
          newconnection->m_socket =
              accept(it, (sockaddr*)&newconnection->m_cliaddr, &newconnection->m_addrlen);

          if (newconnection->m_socket == INVALID_SOCKET)
          {
            CLog::Log(LOGERROR, "JSONRPC Server: Accept of new connection failed: {}", errno);
            if (EBADF == errno)
            {
              CThread::Sleep(1000ms);
              Initialize();
              break;
            }
          }
          else
          {
            CLog::Log(LOGINFO, "JSONRPC Server: New connection added");
            m_connections.push_back(std::move(newconnection));
          }
        }
      }
    }
  }

  Deinitialize();
}

bool CTCPServer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
{
  return false;
}

bool CTCPServer::Download(const char *path, CVariant &result)
{
  return false;
}

int CTCPServer::GetCapabilities()
{
  return Response | Announcing;
}

void CTCPServer::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                          const std::string& sender,
                          const std::string& message,
                          const CVariant& data)
{
  // Take a snapshot under the lock and send outside it. Each entry is a shared_ptr, so a
  // connection the Process thread drops mid-iteration stays alive until we are done with it.
  std::vector<std::shared_ptr<CTCPClient>> connections;
  {
    std::unique_lock lock(m_connectionsCritSection);
    if (m_connections.empty())
      return;

    connections = m_connections;
  }

  std::string str = IJSONRPCAnnouncer::AnnouncementToJSONRPC(flag, sender, message, data, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_jsonOutputCompact);

  for (const auto& connection : connections)
  {
    {
      std::unique_lock connLock(connection->m_critSection);
      if ((connection->GetAnnouncementFlags() & flag) == 0)
        continue;
    }

    connection->Send(str.c_str(), str.size());
  }
}

bool CTCPServer::Initialize()
{
  Deinitialize();

  bool started = false;

  started |= InitializeBlue();
  started |= InitializeTCP();

  if (started)
  {
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
    CLog::Log(LOGINFO, "JSONRPC Server: Successfully initialized");
    return true;
  }
  return false;
}

#ifdef TARGET_WINDOWS_STORE
bool CTCPServer::InitializeBlue()
{
  CLog::Log(LOGDEBUG, "{} is not implemented", __FUNCTION__);
  return true; // need to fake it for now
}

#else
bool CTCPServer::InitializeBlue()
{
  if (!m_nonlocal)
    return false;

#ifdef TARGET_WINDOWS

  SOCKET fd = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
  if (fd == INVALID_SOCKET)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to get bluetooth socket");
    return false;
  }
  SOCKADDR_BTH sa  = {};
  sa.addressFamily = AF_BTH;
  sa.port          = BT_PORT_ANY;

  if (bind(fd, (SOCKADDR*)&sa, sizeof(sa)) < 0)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to bind to bluetooth socket");
    closesocket(fd);
    return false;
  }

  ULONG optval = TRUE;
  if (setsockopt(fd, SOL_RFCOMM, SO_BTH_AUTHENTICATE, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to force authentication for bluetooth socket");
    closesocket(fd);
    return false;
  }

  int len = sizeof(sa);
  if (getsockname(fd, (SOCKADDR*)&sa, &len) < 0)
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to get bluetooth port");

  if (listen(fd, 10) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to listen to bluetooth port");
    closesocket(fd);
    return false;
  }

  m_servers.push_back(fd);

  CSADDR_INFO addrinfo;
  addrinfo.iProtocol   = BTHPROTO_RFCOMM;
  addrinfo.iSocketType = SOCK_STREAM;
  addrinfo.LocalAddr.lpSockaddr       = (SOCKADDR*)&sa;
  addrinfo.LocalAddr.iSockaddrLength  = sizeof(sa);
  addrinfo.RemoteAddr.lpSockaddr      = (SOCKADDR*)&sa;
  addrinfo.RemoteAddr.iSockaddrLength = sizeof(sa);

  using KODI::PLATFORM::WINDOWS::ToW;

  WSAQUERYSET service = {};
  service.dwSize = sizeof(service);
  service.lpszServiceInstanceName = const_cast<LPWSTR>(ToW(bt_service_name).c_str());
  service.lpServiceClassId        = (LPGUID)&bt_service_guid;
  service.lpszComment             = const_cast<LPWSTR>(ToW(bt_service_desc).c_str());
  service.dwNameSpace             = NS_BTH;
  service.lpNSProviderId          = NULL; /* RFCOMM? */
  service.lpcsaBuffer             = &addrinfo;
  service.dwNumberOfCsAddrs       = 1;

  if (WSASetService(&service, RNRSERVICE_REGISTER, 0) == SOCKET_ERROR)
    CLog::Log(LOGERROR, "JSONRPC Server: failed to register bluetooth service error {}",
              WSAGetLastError());

  return true;
#endif

#ifdef HAVE_LIBBLUETOOTH

  SOCKET fd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (fd == INVALID_SOCKET)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to get bluetooth socket");
    return false;
  }
  struct sockaddr_rc sa = {};
  sa.rc_family  = AF_BLUETOOTH;
  sa.rc_bdaddr  = bt_bdaddr_any;
  sa.rc_channel = 0;

  if (bind(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0)
  {
    CLog::Log(LOGINFO, "JSONRPC Server: Unable to bind to bluetooth socket");
    closesocket(fd);
    return false;
  }

  socklen_t len = sizeof(sa);
  if (getsockname(fd, (struct sockaddr*)&sa, &len) < 0)
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to get bluetooth port");

  if (listen(fd, 10) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to listen to bluetooth port {}", sa.rc_channel);
    closesocket(fd);
    return false;
  }

  uint8_t rfcomm_channel = sa.rc_channel;

  uuid_t root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid;
  sdp_list_t *l2cap_list = 0,
             *rfcomm_list = 0,
             *root_list = 0,
             *proto_list = 0,
             *access_proto_list = 0,
             *service_class = 0;

  sdp_data_t *channel = 0;

  sdp_record_t *record = sdp_record_alloc();

  // set the general service ID
  sdp_uuid128_create(&svc_uuid, &bt_service_guid);
  sdp_set_service_id(record, svc_uuid);

  // make the service record publicly browseable
  sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
  root_list = sdp_list_append(0, &root_uuid);
  sdp_set_browse_groups(record, root_list);

  // set l2cap information
  sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
  l2cap_list = sdp_list_append(0, &l2cap_uuid);
  proto_list = sdp_list_append(0, l2cap_list);

  // set rfcomm information
  sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
  channel = sdp_data_alloc(SDP_UINT8, &rfcomm_channel);
  rfcomm_list = sdp_list_append(0, &rfcomm_uuid);
  sdp_list_append(rfcomm_list, channel);
  sdp_list_append(proto_list, rfcomm_list);

  // attach protocol information to service record
  access_proto_list = sdp_list_append(0, proto_list);
  sdp_set_access_protos(record, access_proto_list);

  // set the name, provider, and description
  sdp_set_info_attr(record, bt_service_name, bt_service_prov, bt_service_desc);

  // set the Service class ID
  service_class = sdp_list_append(0, &svc_uuid);
  sdp_set_service_classes(record, service_class);

  // cleanup
  sdp_data_free(channel);
  sdp_list_free(l2cap_list, 0);
  sdp_list_free(rfcomm_list, 0);
  sdp_list_free(root_list, 0);
  sdp_list_free(access_proto_list, 0);
  sdp_list_free(service_class, 0);

  // connect to the local SDP server, register the service record
  sdp_session_t *session = sdp_connect(&bt_bdaddr_any, &bt_bdaddr_local, SDP_RETRY_IF_BUSY);
  if (session == NULL)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to connect to sdpd");
    closesocket(fd);
    sdp_record_free(record);
    return false;
  }

  if (sdp_record_register(session, record, 0) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to register record with error {}", errno);
    closesocket(fd);
    sdp_close(session);
    sdp_record_free(record);
    return false;
  }

  m_sdpd = session;
  m_servers.push_back(fd);

  return true;
#endif
  return false;
}
#endif

bool CTCPServer::InitializeTCP()
{
  Deinitialize();

  std::vector<SOCKET> sockets = CreateTCPServerSocket(m_port, !m_nonlocal, 10, "JSONRPC");
  if (sockets.empty())
    return false;

  m_servers.insert(m_servers.end(), sockets.begin(), sockets.end());
  return true;
}

void CTCPServer::Deinitialize()
{
  // unregister first so no Announce callback runs during teardown
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  std::unique_lock lock(m_connectionsCritSection);

  for (const auto& connection : m_connections)
  {
    connection->StopWorker();
    connection->Disconnect();
  }

  m_connections.clear();

  for (unsigned int i = 0; i < m_servers.size(); i++)
    closesocket(m_servers[i]);

  m_servers.clear();

#ifdef HAVE_LIBBLUETOOTH
  if (m_sdpd)
    sdp_close((sdp_session_t*)m_sdpd);
  m_sdpd = NULL;
#endif
}

CTCPServer::CTCPClient::CTCPClient()
{
  m_new = true;
  m_announcementflags = ANNOUNCEMENT::ANNOUNCE_ALL;
  m_beginBrackets = 0;
  m_endBrackets = 0;
  m_beginChar = 0;
  m_endChar = 0;

  m_addrlen = sizeof(m_cliaddr);
}

CTCPServer::CTCPClient::CTCPClient(const CTCPClient& client)
{
  Copy(client);
}

CTCPServer::CTCPClient& CTCPServer::CTCPClient::operator=(const CTCPClient& client)
{
  Copy(client);
  return *this;
}

int CTCPServer::CTCPClient::GetPermissionFlags()
{
  return OPERATION_PERMISSION_ALL;
}

int CTCPServer::CTCPClient::GetAnnouncementFlags()
{
  return m_announcementflags;
}

bool CTCPServer::CTCPClient::SetAnnouncementFlags(int flags)
{
  m_announcementflags = flags;
  return true;
}

void CTCPServer::CTCPClient::Send(const char *data, unsigned int size)
{
  std::unique_lock lock(m_critSection);
  unsigned int sent = 0;
  while (sent < size)
  {
    const auto written = send(m_socket, data + sent, size - sent, 0);
    if (written <= 0)
    {
      // The server thread can close the socket while a send is in progress. send() then
      // returns -1, which must not reach the unsigned counter below.
      CLog::Log(LOGERROR, "JSONRPC Server: Send failed, dropping {} of {} bytes", size - sent,
                size);
      return;
    }

    sent += static_cast<unsigned int>(written);
  }
}

void CTCPServer::CTCPClient::Enqueue(const std::shared_ptr<CTCPClient>& self,
                                     CTCPServer* host,
                                     const char* buffer,
                                     int length)
{
  // Cleared on the server thread so a second read cannot re-enter the WebSocket handshake
  // before the first buffer is parsed.
  m_new = false;

  bool start = false;
  {
    std::unique_lock<std::mutex> lock(m_inboundMutex);
    m_inbound.emplace_back(buffer, length);
    m_inboundBytes += static_cast<size_t>(length);
    if (!m_workerStarted)
    {
      m_workerStarted = true;
      start = true;
    }
  }

  m_inboundEvent.notify_one();

  if (start)
  {
    {
      std::lock_guard lock(host->m_workersMutex);
      ++host->m_activeWorkers;
    }

    try
    {
      std::thread(&CTCPClient::RunWorker, self, host->m_self.lock()).detach();
    }
    catch (const std::exception& error)
    {
      // Out of threads. The count was reserved for a worker that will never run, and letting
      // this reach the server thread would end it: CThread swallows the exception, so Process()
      // would return without deinitializing and no connection would ever be served again.
      {
        std::lock_guard lock(host->m_workersMutex);
        --host->m_activeWorkers;
      }
      host->m_workersDone.notify_all();

      {
        std::unique_lock<std::mutex> lock(m_inboundMutex);
        m_workerStarted = false;
        m_inbound.clear();
        m_inboundBytes = 0;
      }

      CLog::Log(LOGERROR,
                "JSONRPC Server: Could not start a request worker ({}), dropping the "
                "connection",
                error.what());
      RequestClose();
    }
  }
}

bool CTCPServer::CTCPClient::Backlogged()
{
  std::unique_lock<std::mutex> lock(m_inboundMutex);
  return m_inboundBytes >= maxBufferLength;
}

void CTCPServer::CTCPClient::StopWorker()
{
  {
    std::unique_lock<std::mutex> lock(m_inboundMutex);
    m_workerStop = true;
  }

  m_inboundEvent.notify_one();
}

void CTCPServer::CTCPClient::RunWorker(std::shared_ptr<CTCPClient> self,
                                       std::shared_ptr<CTCPServer> host)
{
  RunRequests(self, host.get());

  {
    std::lock_guard lock(host->m_workersMutex);
    --host->m_activeWorkers;
  }
  host->m_workersDone.notify_all();
}

void CTCPServer::CTCPClient::RunRequests(const std::shared_ptr<CTCPClient>& self, CTCPServer* host)
{
  while (true)
  {
    std::string buffer;
    {
      std::unique_lock<std::mutex> lock(self->m_inboundMutex);
      self->m_inboundEvent.wait(lock,
                                [&self] { return !self->m_inbound.empty() || self->m_workerStop; });

      // Drain what has already been accepted before exiting, so a client that sends a command
      // and closes immediately still gets it executed.
      if (self->m_inbound.empty())
        return;

      buffer = std::move(self->m_inbound.front());
      self->m_inbound.pop_front();
      self->m_inboundBytes -= buffer.size();
    }

    self->PushBuffer(host, buffer.data(), static_cast<int>(buffer.size()));
  }
}

void CTCPServer::WaitForWorkers(std::chrono::milliseconds timeout)
{
  std::unique_lock lock(m_workersMutex);
  m_workersDone.wait_for(lock, timeout, [this] { return m_activeWorkers == 0; });
}

void CTCPServer::CTCPClient::PushBuffer(CTCPServer* host, const char* buffer, int length)
{
  bool inObject = false;
  bool inString = false;
  bool escapeNext = false;

  for (int i = 0; i < length; i++)
  {
    char c = buffer[i];

    if (m_beginChar == 0 && c == '{')
    {
      m_beginChar = '{';
      m_endChar = '}';
    }
    else if (m_beginChar == 0 && c == '[')
    {
      m_beginChar = '[';
      m_endChar = ']';
    }

    if (m_beginChar != 0)
    {
      m_buffer.push_back(c);
      if (inObject)
      {
        if (!inString)
        {
          if (c == '"')
            inString = true;
        }
        else
        {
          if (escapeNext)
          {
            escapeNext = false;
          }
          else
          {
            if (c == '\\')
              escapeNext = true;
            else if (c == '"')
              inString = false;
          }
        }
      }
      if (!inString)
      {
        if (c == m_beginChar)
        {
          m_beginBrackets++;
          inObject = true;
        }
        else if (c == m_endChar)
        {
          m_endBrackets++;
          if (m_beginBrackets == m_endBrackets)
            inObject = false;
        }
      }
      if (m_beginBrackets > 0 && m_endBrackets > 0 && m_beginBrackets == m_endBrackets)
      {
        std::string line = CJSONRPC::MethodCall(m_buffer, host, this);
        Send(line.c_str(), line.size());
        m_beginChar = m_beginBrackets = m_endBrackets = 0;
        m_buffer.clear();
      }
    }
  }
}

void CTCPServer::CTCPClient::Disconnect()
{
  if (m_socket > 0)
  {
    std::unique_lock lock(m_critSection);
    shutdown(m_socket, SHUT_RDWR);
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }
}

void CTCPServer::CTCPClient::Copy(const CTCPClient& client)
{
  m_new               = client.m_new;
  m_socket            = client.m_socket;
  m_cliaddr           = client.m_cliaddr;
  m_addrlen           = client.m_addrlen;
  m_announcementflags = client.m_announcementflags;
  m_beginBrackets     = client.m_beginBrackets;
  m_endBrackets       = client.m_endBrackets;
  m_beginChar         = client.m_beginChar;
  m_endChar           = client.m_endChar;
  m_buffer            = client.m_buffer;
}

CTCPServer::CWebSocketClient::CWebSocketClient(CWebSocket *websocket)
{
  m_websocket = websocket;
  m_buffer.reserve(maxBufferLength);
}

CTCPServer::CWebSocketClient::CWebSocketClient(const CWebSocketClient& client)
  : CTCPServer::CTCPClient(client)
{
  *this = client;
  m_buffer.reserve(maxBufferLength);
}

CTCPServer::CWebSocketClient::CWebSocketClient(CWebSocket *websocket, const CTCPClient& client)
{
  Copy(client);

  m_websocket = websocket;
  m_buffer.reserve(maxBufferLength);
}

CTCPServer::CWebSocketClient::~CWebSocketClient()
{
  delete m_websocket;
}

CTCPServer::CWebSocketClient& CTCPServer::CWebSocketClient::operator=(const CWebSocketClient& client)
{
  Copy(client);

  m_websocket = client.m_websocket;
  m_buffer = client.m_buffer;

  return *this;
}

void CTCPServer::CWebSocketClient::Send(const char *data, unsigned int size)
{
  // The announcement thread and this connection's worker both send here, so framing and the
  // writes that carry one message have to stay together.
  std::unique_lock lock(m_critSection);

  const CWebSocketMessage *msg = m_websocket->Send(WebSocketTextFrame, data, size);
  if (msg == NULL || !msg->IsComplete())
    return;

  std::vector<const CWebSocketFrame *> frames = msg->GetFrames();
  for (unsigned int index = 0; index < frames.size(); index++)
    CTCPClient::Send(frames.at(index)->GetFrameData(), (unsigned int)frames.at(index)->GetFrameLength());
}

void CTCPServer::CWebSocketClient::PushBuffer(CTCPServer *host, const char *buffer, int length)
{
  bool send;
  const CWebSocketMessage *msg = NULL;
  std::vector<std::string> payloads;

  {
    // Unframing shares the websocket with the announcement thread and is locked; the JSON-RPC
    // calls below are not, as one may sit in a modal dialog.
    std::unique_lock lock(m_critSection);

    if (m_buffer.size() + length > maxBufferLength)
    {
      CLog::Log(LOGINFO, "WebSocket: client buffer size {} exceeded", maxBufferLength);
      RequestClose();
      return;
    }

    m_buffer.append(buffer, length);

    const char* buf = m_buffer.data();
    size_t len = m_buffer.size();

    do
    {
      if ((msg = m_websocket->Handle(buf, len, send)) != NULL && msg->IsComplete())
      {
        std::vector<const CWebSocketFrame*> frames = msg->GetFrames();
        if (send)
        {
          for (unsigned int index = 0; index < frames.size(); index++)
            CTCPClient::Send(frames.at(index)->GetFrameData(),
                             static_cast<unsigned int>(frames.at(index)->GetFrameLength()));
        }
        else
        {
          for (unsigned int index = 0; index < frames.size(); index++)
            payloads.emplace_back(frames.at(index)->GetApplicationData(),
                                  static_cast<size_t>(frames.at(index)->GetLength()));
        }

        delete msg;
      }
    } while (len > 0 && msg != NULL);

    if (len < m_buffer.size())
      m_buffer = m_buffer.substr(m_buffer.size() - len);

    if (m_websocket->GetState() == WebSocketStateClosed)
      RequestClose();
  }

  for (const auto& payload : payloads)
    CTCPClient::PushBuffer(host, payload.data(), static_cast<int>(payload.size()));
}

void CTCPServer::CWebSocketClient::Disconnect()
{
  std::unique_lock lock(m_critSection);

  if (m_socket > 0)
  {
    if (m_websocket->GetState() != WebSocketStateClosed && m_websocket->GetState() != WebSocketStateNotConnected)
    {
      const CWebSocketFrame *closeFrame = m_websocket->Close();
      if (closeFrame)
        Send(closeFrame->GetFrameData(), (unsigned int)closeFrame->GetFrameLength());
    }

    if (m_websocket->GetState() == WebSocketStateClosed)
      CTCPClient::Disconnect();
  }
}
