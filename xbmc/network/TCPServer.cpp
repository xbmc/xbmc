#include "TCPServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "interfaces/json-rpc/JSONRPC.h"
#include "jsoncpp/include/json/json.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "threads/SingleLock.h"

#ifdef _WIN32
extern "C" int inet_pton(int af, const char *src, void *dst);
#define close closesocket
#define SHUT_RDWR SD_BOTH
#endif

using namespace JSONRPC;
using namespace ANNOUNCEMENT;
//using namespace std; On VS2010, bind conflicts with std::bind
using namespace Json;

#define RECEIVEBUFFER 1024

CTCPServer *CTCPServer::ServerInstance = NULL;

bool CTCPServer::StartServer(int port, bool nonlocal)
{
  StopServer(true);

  ServerInstance = new CTCPServer(port, nonlocal);
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create();
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
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

CTCPServer::CTCPServer(int port, bool nonlocal)
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_ServerSocket = -1;
}

void CTCPServer::Process()
{
  m_bStop = false;

  while (!m_bStop)
  {
    int             max_fd = 0;
    fd_set          rfds;
    struct timeval  to     = {1, 0};
    FD_ZERO(&rfds);

    FD_SET(m_ServerSocket, &rfds);
    max_fd = m_ServerSocket;

    for (unsigned int i = 0; i < m_connections.size(); i++)
    {
      FD_SET(m_connections[i].m_socket, &rfds);
      if (m_connections[i].m_socket > max_fd)
        max_fd = m_connections[i].m_socket;
    }

    int res = select(max_fd+1, &rfds, NULL, NULL, &to);
    if (res < 0)
    {
      CLog::Log(LOGERROR, "JSONRPC Server: Select failed");
      Sleep(1000);
      Initialize();
    }
    else if (res > 0)
    {
      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        int socket = m_connections[i].m_socket;
        if (FD_ISSET(socket, &rfds))
        {
          char buffer[RECEIVEBUFFER] = {};
          int  nread = 0;
          nread = recv(socket, (char*)&buffer, RECEIVEBUFFER, 0);
          if (nread > 0)
          {
            m_connections[i].PushBuffer(this, buffer, nread);
          }
          if (nread <= 0)
          {
            CLog::Log(LOGINFO, "JSONRPC Server: Disconnection detected");
            m_connections[i].Disconnect();
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      if (FD_ISSET(m_ServerSocket, &rfds))
      {
        CLog::Log(LOGDEBUG, "JSONRPC Server: New connection detected");
        CTCPClient newconnection;
        newconnection.m_socket = accept(m_ServerSocket, &newconnection.m_cliaddr, &newconnection.m_addrlen);

        if (newconnection.m_socket < 0)
          CLog::Log(LOGERROR, "JSONRPC Server: Accept of new connection failed");
        else
        {
          CLog::Log(LOGINFO, "JSONRPC Server: New connection added");
          m_connections.push_back(newconnection);
        }
      }
    }
  }

  Deinitialize();
}

bool CTCPServer::Download(const char *path, Json::Value *result)
{
  return false;
}

int CTCPServer::GetCapabilities()
{
  return Response | Announcing;
}

void CTCPServer::Announce(EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  Value root;
  root["jsonrpc"] = "2.0";
  root["method"]  = "Announcement";
  root["params"]["sender"] = sender;
  root["params"]["message"] = message;
  if (!data.isNull())
    data.toJsonValue(root["params"]["data"]);

  StyledWriter writer;
  std::string str = writer.write(root);

  for (unsigned int i = 0; i < m_connections.size(); i++)
  {
    {
      CSingleLock lock (m_connections[i].m_critSection);
      if ((m_connections[i].GetAnnouncementFlags() & flag) == 0)
        continue;
    }

    unsigned int sent = 0;
    do
    {
      CSingleLock lock (m_connections[i].m_critSection);
      sent += send(m_connections[i].m_socket, str.c_str(), str.size() - sent, sent);
    } while (sent < str.size());
  }
}

bool CTCPServer::Initialize()
{
  Deinitialize();

  struct sockaddr_in myaddr;
  memset(&myaddr, 0, sizeof(myaddr));

  myaddr.sin_family = AF_INET;
  myaddr.sin_port = htons(m_port);

  if (m_nonlocal)
    myaddr.sin_addr.s_addr = INADDR_ANY;
  else
    inet_pton(AF_INET, "127.0.0.1", &myaddr.sin_addr.s_addr);

  m_ServerSocket = socket(PF_INET, SOCK_STREAM, 0);

  if (m_ServerSocket < 0)
  {
#ifdef _WIN32
    int ierr = WSAGetLastError();
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to create serversocket %d", ierr);
    // hack for broken third party libs
    if(ierr == WSANOTINITIALISED)
    {
      WSADATA wd;
      if (WSAStartup(MAKEWORD(2,2), &wd) != 0)
        CLog::Log(LOGERROR, "JSONRPC Server: WSAStartup failed");
    }
#else
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to create serversocket");
#endif
    return false;
  }

  if (bind(m_ServerSocket, (struct sockaddr*)&myaddr, sizeof myaddr) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to bind serversocket");
    close(m_ServerSocket);
    return false;
  }

  if (listen(m_ServerSocket, 10) < 0)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Failed to set listen");
    close(m_ServerSocket);
    return false;
  }

  CAnnouncementManager::AddAnnouncer(this);

  CLog::Log(LOGINFO, "JSONRPC Server: Successfully initialized");
  return true;
}

void CTCPServer::Deinitialize()
{
  for (unsigned int i = 0; i < m_connections.size(); i++)
    m_connections[i].Disconnect();

  m_connections.clear();

  if (m_ServerSocket > 0)
  {
    shutdown(m_ServerSocket, SHUT_RDWR);
    close(m_ServerSocket);
    m_ServerSocket = -1;
  }

  CAnnouncementManager::RemoveAnnouncer(this);
}

CTCPServer::CTCPClient::CTCPClient()
{
  m_announcementflags = ANNOUNCE_ALL;
  m_socket = -1;
  m_beginBrackets = 0;
  m_endBrackets = 0;
  m_beginChar = 0;
  m_endChar = 0;

  m_addrlen = sizeof(struct sockaddr);
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

void CTCPServer::CTCPClient::PushBuffer(CTCPServer *host, const char *buffer, int length)
{
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
      if (c == m_beginChar)
        m_beginBrackets++;
      else if (c == m_endChar)
        m_endBrackets++;
      if (m_beginBrackets > 0 && m_endBrackets > 0 && m_beginBrackets == m_endBrackets)
      {
        std::string line = CJSONRPC::MethodCall(m_buffer, host, this);
        CSingleLock lock (m_critSection);
        send(m_socket, line.c_str(), line.size(), 0);
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
    CSingleLock lock (m_critSection);
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
    m_socket = -1;
  }
}

void CTCPServer::CTCPClient::Copy(const CTCPClient& client)
{
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

