/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UdpClient.h"
#ifdef TARGET_POSIX
#include <sys/ioctl.h>
#endif
#include "Network.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <chrono>

#include <arpa/inet.h>

using namespace std::chrono_literals;

#define UDPCLIENT_DEBUG_LEVEL LOGDEBUG

CUdpClient::CUdpClient(void) : CThread("UDPClient")
{}

CUdpClient::~CUdpClient(void) = default;

bool CUdpClient::Create(void)
{
  m_bStop = false;

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Creating UDP socket...");

  // Create a UDP socket
  client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (client_socket == SOCKET_ERROR)
  {
    CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Unable to create socket.");
    return false;
  }

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Setting broadcast socket option...");

  unsigned int value = 1;
  if ( setsockopt( client_socket, SOL_SOCKET, SO_BROADCAST, (char*) &value, sizeof( unsigned int ) ) == SOCKET_ERROR)
  {
    CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Unable to set socket option.");
    return false;
  }

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Setting non-blocking socket options...");

  unsigned long nonblocking = 1;
  ioctlsocket(client_socket, FIONBIO, &nonblocking);

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Spawning listener thread...");
  CThread::Create(false);

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Ready.");

  return true;
}

void CUdpClient::Destroy()
{
  StopThread();
  closesocket(client_socket);
}

void CUdpClient::OnStartup()
{
  SetPriority( GetMinPriority() );
}

bool CUdpClient::Broadcast(int aPort, const std::string& aMessage)
{
  CSingleLock lock(critical_section);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(aPort);
  addr.sin_addr.s_addr = INADDR_BROADCAST;
  memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

  UdpCommand broadcast = {addr, aMessage, NULL, 0};
  commands.push_back(broadcast);

  return true;
}


bool CUdpClient::Send(const std::string& aIpAddress, int aPort, const std::string& aMessage)
{
  CSingleLock lock(critical_section);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(aPort);
  addr.sin_addr.s_addr = inet_addr(aIpAddress.c_str());
  memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

  UdpCommand transmit = {addr, aMessage, NULL, 0};
  commands.push_back(transmit);

  return true;
}

bool CUdpClient::Send(struct sockaddr_in aAddress, const std::string& aMessage)
{
  CSingleLock lock(critical_section);

  UdpCommand transmit = {aAddress, aMessage, NULL, 0};
  commands.push_back(transmit);

  return true;
}

bool CUdpClient::Send(struct sockaddr_in aAddress, unsigned char* pMessage, DWORD dwSize)
{
  CSingleLock lock(critical_section);

  UdpCommand transmit = {aAddress, "", pMessage, dwSize};
  commands.push_back(transmit);

  return true;
}


void CUdpClient::Process()
{
  CThread::Sleep(2000ms);

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Listening.");

  struct sockaddr_in remoteAddress;
  char messageBuffer[1024];
  DWORD dataAvailable;

  while ( !m_bStop )
  {
    fd_set readset, exceptset;
    FD_ZERO(&readset);    FD_SET(client_socket, &readset);
    FD_ZERO(&exceptset);  FD_SET(client_socket, &exceptset);

    int nfds = (int)(client_socket);
    timeval tv = { 0, 100000 };
    if (select(nfds, &readset, NULL, &exceptset, &tv) < 0)
    {
      CLog::Log(LOGERROR, "UDPCLIENT: failed to select on socket");
      break;
    }

    // is there any data to read
    dataAvailable = 0;
    ioctlsocket(client_socket, FIONREAD, &dataAvailable);

    // while there is data to read
    while (dataAvailable > 0)
    {
      // read data
      int messageLength = sizeof(messageBuffer) - 1 ;
#ifndef TARGET_POSIX
      int remoteAddressSize;
#else
      socklen_t remoteAddressSize;
#endif
      remoteAddressSize = sizeof(remoteAddress);

      int ret = recvfrom(client_socket, messageBuffer, messageLength, 0, (struct sockaddr *) & remoteAddress, &remoteAddressSize);
      if (ret != SOCKET_ERROR)
      {
        // Packet received
        messageLength = ret;
        messageBuffer[messageLength] = '\0';

        std::string message = messageBuffer;

        auto now = std::chrono::steady_clock::now();
        auto timestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

        CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT RX: {}\t\t<- '{}'", timestamp.count(), message);

        OnMessage(remoteAddress, message, reinterpret_cast<unsigned char*>(messageBuffer), messageLength);
      }
      else
      {
        CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Socket error {}", WSAGetLastError());
      }

      // is there any more data to read?
      dataAvailable = 0;
      ioctlsocket(client_socket, FIONREAD, &dataAvailable);
    }

    // dispatch a single command if any pending
    while(DispatchNextCommand()) {}
  }

  closesocket(client_socket);

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Stopped listening.");
}


bool CUdpClient::DispatchNextCommand()
{
  UdpCommand command;
  {
    CSingleLock lock(critical_section);

    if (commands.size() <= 0)
      return false;

    COMMANDITERATOR it = commands.begin();
    command = *it;
    commands.erase(it);
  }

  int ret;
  if (command.binarySize > 0)
  {
    // only perform the following if logging level at debug

    auto now = std::chrono::steady_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    CLog::Log(UDPCLIENT_DEBUG_LEVEL,
              "UDPCLIENT TX: {}\t\t-> "
              "<binary payload {} bytes>",
              timestamp.count(), command.binarySize);

    do
    {
      ret = sendto(client_socket, (const char*) command.binary, command.binarySize, 0, (struct sockaddr *) & command.address, sizeof(command.address));
    }
    while (ret == -1);

    delete[] command.binary;
  }
  else
  {
    // only perform the following if logging level at debug
    auto now = std::chrono::steady_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT TX: {}\t\t-> '{}'", timestamp.count(),
              command.message);

    do
    {
      ret = sendto(client_socket, command.message.c_str(), command.message.size(), 0, (struct sockaddr *) & command.address, sizeof(command.address));
    }
    while (ret == -1 && !m_bStop);
  }
  return true;
}
