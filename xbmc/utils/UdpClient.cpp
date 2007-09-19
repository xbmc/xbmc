
#include "stdafx.h"
#include "UdpClient.h"
#ifdef _LINUX
#include <sys/ioctl.h>
#endif
#include "../utils/Network.h"

#define UDPCLIENT_DEBUG_LEVEL LOGDEBUG

CUdpClient::CUdpClient(void) : CThread()
{}

CUdpClient::~CUdpClient(void)
{
}

bool CUdpClient::Create(void)
{
  m_bStop = false;

  InitializeCriticalSection(&critical_section);

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
  DeleteCriticalSection(&critical_section);
}

void CUdpClient::OnStartup()
{
  SetPriority( THREAD_PRIORITY_LOWEST );
}

bool CUdpClient::Broadcast(int aPort, CStdString& aMessage)
{
  EnterCriticalSection(&critical_section);

  SOCKADDR_IN addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(aPort);
  addr.sin_addr.s_addr = INADDR_BROADCAST;

  UdpCommand broadcast = {addr, aMessage, NULL, 0};
  commands.push_back(broadcast);

  LeaveCriticalSection(&critical_section);
  return true;
}


bool CUdpClient::Send(CStdString aIpAddress, int aPort, CStdString& aMessage)
{
  EnterCriticalSection(&critical_section);

  SOCKADDR_IN addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(aPort);
  addr.sin_addr.s_addr = inet_addr(aIpAddress);

  UdpCommand transmit = {addr, aMessage, NULL, 0};
  commands.push_back(transmit);

  LeaveCriticalSection(&critical_section);
  return true;
}

bool CUdpClient::Send(SOCKADDR_IN aAddress, CStdString& aMessage)
{
  EnterCriticalSection(&critical_section);

  UdpCommand transmit = {aAddress, aMessage, NULL, 0};
  commands.push_back(transmit);

  LeaveCriticalSection(&critical_section);
  return true;
}

bool CUdpClient::Send(SOCKADDR_IN aAddress, LPBYTE pMessage, DWORD dwSize)
{
  EnterCriticalSection(&critical_section);

  UdpCommand transmit = {aAddress, "", pMessage, dwSize};
  commands.push_back(transmit);

  LeaveCriticalSection(&critical_section);
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
    // is there any data to read
    dataAvailable = 0;
    ioctlsocket(client_socket, FIONREAD, &dataAvailable);

    // while there is data to read
    while (dataAvailable > 0)
    {
      // read data
      int messageLength = sizeof(messageBuffer) - 1 ;
#ifndef _LINUX
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

        CStdString message = messageBuffer;

        CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT RX: %u\t\t<- '%s'", timeGetTime(), message.c_str() );

        // NOTE: You should consider locking access to the screen device
        // or at least wait until after vertical refresh before firing off events
        // to protect access to graphics resources.

        g_graphicsContext.Lock();
        OnMessage(remoteAddress, message, (LPBYTE)messageBuffer, messageLength);
        g_graphicsContext.Unlock();
      }
      else
      {
        CStdString debug;
        debug.Format("UDPCLIENT: Socket error %u", WSAGetLastError());
        CLog::Log(UDPCLIENT_DEBUG_LEVEL, debug);
      }

      // is there any more data to read?
      dataAvailable = 0;
      ioctlsocket(client_socket, FIONREAD, &dataAvailable);
    }

    // dispatch a single command if any pending
    DispatchNextCommand();
  }

  closesocket(client_socket);

  CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT: Stopped listening.");
}


void CUdpClient::DispatchNextCommand()
{
  EnterCriticalSection(&critical_section);

  if (commands.size() <= 0)
  {
    LeaveCriticalSection(&critical_section);

    // relinquish the remainder of this threads time slice
    Sleep(1);
    return ;
  }

  COMMANDITERATOR it = commands.begin();
  UdpCommand command = *it;
  commands.erase(it);
  LeaveCriticalSection(&critical_section);

  int ret;
  if (command.binarySize > 0)
  {
    // only perform the following if logging level at debug
    CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT TX: %u\t\t-> <binary payload %u bytes>", timeGetTime(), command.binarySize );

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
    CLog::Log(UDPCLIENT_DEBUG_LEVEL, "UDPCLIENT TX: %u\t\t-> '%s'", timeGetTime(), command.message.c_str() );

    do
    {
      ret = sendto(client_socket, command.message, command.message.GetLength(), 0, (struct sockaddr *) & command.address, sizeof(command.address));
    }
    while (ret == -1 && !m_bStop);
  }
}
