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

#include "threads/SystemClock.h"
#include "UdpClient.h"
#ifdef TARGET_POSIX
#include <sys/ioctl.h>
#endif
#include "Network.h"
#include "guilib/GraphicContext.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include "threads/SingleLock.h"

#include <arpa/inet.h>

#define UDPCLIENT_DEBUG_LEVEL LOGDEBUG

CUdpClient::CUdpClient(void) : CThread("UDPClient")
{}

CUdpClient::~CUdpClient(void)
{
}

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
  CThread::Create(false, THREAD_MINSTACKSIZE);

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

  SOCKADDR_IN addr;
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

  SOCKADDR_IN addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(aPort);
  addr.sin_addr.s_addr = inet_addr(aIpAddress.c_str());
  memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

  UdpCommand transmit = {addr, aMessage, NULL, 0};
  commands.push_back(transmit);

  return true;
}

bool CUdpClient::Send(SOCKADDR_IN aAddress, const std::string& aMessage)
{
  CSingleLock lock(critical_section);

  UdpCommand transmit = {aAddress, aMessage, NULL, 0};
  commands.push_back(transmit);

  return true;
}

bool CUdpClient::Send(SOCKADDR_IN aAddress, LPBYTE pMessage, DWORD dwSize)
{
  CSingleLock lock(critical_section);

  UdpCommand transmit = {aAddress, "", pMessage, dwSize};
  commands.push_back(transmit);

  return true;
}


void CUdpClient::Process()
{
  Sleep(2000);

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Listening.");

  SOCKADDR_IN remoteAddress;
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

        CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT RX: %u\t\t<- '%s'",
                  XbmcThreads::SystemClockMillis(), message.c_str() );

        // NOTE: You should consider locking access to the screen device
        // or at least wait until after vertical refresh before firing off events
        // to protect access to graphics resources.

        g_graphicsContext.Lock();
        OnMessage(remoteAddress, message, (LPBYTE)messageBuffer, messageLength);
        g_graphicsContext.Unlock();
      }
      else
      {
        CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Socket error %u", WSAGetLastError());
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
    CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT TX: %u\t\t-> "
                                     "<binary payload %u bytes>",
              XbmcThreads::SystemClockMillis(), command.binarySize );

    do
    {
      ret = sendto(client_socket, (LPCSTR) command.binary, command.binarySize, 0, (struct sockaddr *) & command.address, sizeof(command.address));
    }
    while (ret == -1);

    delete[] command.binary;
  }
  else
  {
    // only perform the following if logging level at debug
    CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT TX: %u\t\t-> '%s'",
              XbmcThreads::SystemClockMillis(), command.message.c_str() );

    do
    {
      ret = sendto(client_socket, command.message.c_str(), command.message.size(), 0, (struct sockaddr *) & command.address, sizeof(command.address));
    }
    while (ret == -1 && !m_bStop);
  }
  return true;
}
