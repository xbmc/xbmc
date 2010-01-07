#include "TCPServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "JSONRPC.h"
#include "../libjsoncpp/json.h"

using namespace JSONRPC;
using namespace BROADCAST;
using namespace std;
using namespace Json;

#define RECEIVEBUFFER 1024

CTCPServer *CTCPServer::ServerInstance;

void CTCPServer::StartServer(int port)
{
  StopServer(true);

  ServerInstance = new CTCPServer(port);
  ServerInstance->Create();
}

void CTCPServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->StopThread(bWait);
  }
}

CTCPServer::CTCPServer(int port)
{
  m_port = port;
  m_ServerSocket = -1;
}

void CTCPServer::Process()
{
  m_bStop = Initialize();

  while (m_bStop)
  {
    int             max_fd = 0;
    fd_set          rfds;
    struct timeval  to     = {5, 0};
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
    if (res < -1)
    {
        perror("select: ");
        m_bStop = false;
    }
    else if (res == 0)
      printf("select timeout\n");
    else
    {
      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        int socket = m_connections[i].m_socket;
        if (FD_ISSET(socket, &rfds))
        {
          char buffer[RECEIVEBUFFER] = {};
          int  nread = 0;
          nread = recv(socket, &buffer, RECEIVEBUFFER, 0);
          if (nread > 0)
          {
            printf("recieved %d bytes from client %d (%*s)\n", nread, socket, nread, buffer);
            m_connections[i].PushBuffer(this, buffer, nread);


          }
          if (nread <= 0)
          {
            printf("client %d disconnected (%d)\n", socket, nread);
            m_connections[i].Disconnect();
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      if (FD_ISSET(m_ServerSocket, &rfds))
      {
        CTCPClient newconnection;
        newconnection.m_socket = accept(m_ServerSocket, &newconnection.m_cliaddr, &newconnection.m_addrlen);

        if (newconnection.m_socket < 0)
          perror("error accept failed");
        else
        {
          printf("new connection\n");
          m_connections.push_back(newconnection);
        }
      }
    }
  }

  Deinitialize();
}

bool CTCPServer::CanBroadcast()
{
  return true;
}

void CTCPServer::Broadcast(EBroadcastFlag flag, std::string message)
{
  Value root;
  root["jsonrpc"] = "2.0";
  root["method"]  = message;

  StyledWriter writer;
  string str = writer.write(root);

  printf("Broadcast (%s) -> (%s)\n", message.c_str(), str.c_str());
}

bool CTCPServer::Initialize()
{
  Deinitialize();

  struct sockaddr_in addr;
  m_ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (m_ServerSocket < 0)
  {
    perror("can not create socket");
    return false;
  }
  printf("Server is at %d\n", m_ServerSocket);

  memset(&addr, 0, sizeof(struct sockaddr_in));

  addr.sin_family = PF_INET;
  addr.sin_port = htons(m_port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(m_ServerSocket, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
  {
    perror("error bind failed");
    close(m_ServerSocket);
    return false;
  }

  if (listen(m_ServerSocket, 10) < 0)
  {
    perror("error listen failed");
    close(m_ServerSocket);
    return false;
  }

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
}

CTCPServer::CTCPClient::CTCPClient()
{
  m_broadcastcapabilities = 0;
  m_socket = -1;
  m_beginBrackets = 0;
  m_endBrackets = 0;

  m_addrlen = sizeof(struct sockaddr);
}

int CTCPServer::CTCPClient::GetPermissionFlags()
{
  return OPERATION_PERMISSION_ALL;
}

int CTCPServer::CTCPClient::GetBroadcastFlags()
{
  return m_broadcastcapabilities;
}

bool CTCPServer::CTCPClient::SetBroadcastFlags(int flags)
{
  m_broadcastcapabilities = flags;
  return true;
}

void CTCPServer::CTCPClient::PushBuffer(CTCPServer *host, const char *buffer, int length)
{
  for (int i = 0; i < length; i++)
  {
    char c = buffer[i];
    m_buffer.push_back(c);
    if (c == '{')
      m_beginBrackets++;
    else if (c == '}')
      m_endBrackets++;
    if (m_beginBrackets > 0 && m_endBrackets > 0 && m_beginBrackets == m_endBrackets)
    {
      printf("found something parseable %s\n", m_buffer.c_str());
      string line = CJSONRPC::MethodCall(m_buffer, host, this);
      send(m_socket, line.c_str(), line.size(), 0);
      m_beginBrackets = m_endBrackets = 0;
      m_buffer.clear();
    }
  }
}

void CTCPServer::CTCPClient::Disconnect()
{
  if (m_socket > 0)
  {
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
    m_socket = -1;
  }
}
