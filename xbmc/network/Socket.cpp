/*
 * Socket classes
 *  Copyright (c) 2008 d4rk
 *  Copyright (C) 2008-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Socket.h"

#include "utils/ScopeGuard.h"
#include "utils/log.h"

#include <vector>

using namespace SOCKETS;

#ifdef WINSOCK_VERSION
#define close closesocket
#endif

/**********************************************************************/
/* CPosixUDPSocket                                                    */
/**********************************************************************/

bool CPosixUDPSocket::Bind(bool localOnly, int port, int range)
{
  // close any existing sockets
  Close();

  // If we can, create a socket that works with IPv6 and IPv4.
  // If not, try an IPv4-only socket (we don't want to end up
  // with an IPv6-only socket).
  if (!localOnly) // Only bind loopback to ipv4. TODO : Implement dual bindinds.
  {
    m_ipv6Socket = CheckIPv6(port, range);

    if (m_ipv6Socket)
    {
      m_iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      if (m_iSock != INVALID_SOCKET)
      {
#ifdef WINSOCK_VERSION
        const char zero = 0;
#else
        int zero = 0;
#endif
        if (setsockopt(m_iSock, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero)) == -1)
        {
          closesocket(m_iSock);
          m_iSock = INVALID_SOCKET;
        }
      }
    }
  }

  if (m_iSock == INVALID_SOCKET)
    m_iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (m_iSock == INVALID_SOCKET)
  {
#ifdef TARGET_WINDOWS
    int ierr = WSAGetLastError();
    CLog::Log(LOGERROR, "UDP: Could not create socket {}", ierr);
    // hack for broken third party libs
    if (ierr == WSANOTINITIALISED)
    {
      WSADATA wd;
      if (WSAStartup(MAKEWORD(2,2), &wd) != 0)
        CLog::Log(LOGERROR, "UDP: WSAStartup failed");
    }
#else
    CLog::Log(LOGERROR, "UDP: Could not create socket");
#endif
    CLog::Log(LOGERROR, "UDP: {}", strerror(errno));
    return false;
  }

  // make sure we can reuse the address
#ifdef WINSOCK_VERSION
  const char yes=1;
#else
  int yes = 1;
#endif
  if (setsockopt(m_iSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
  {
    CLog::Log(LOGWARNING, "UDP: Could not enable the address reuse options");
    CLog::Log(LOGWARNING, "UDP: {}", strerror(errno));
  }

  // bind to any address or localhost
  if (m_ipv6Socket)
  {
    if (localOnly)
      m_addr = CAddress("::1");
    else
      m_addr = CAddress("::");
  }
  else
  {
    if (localOnly)
      m_addr = CAddress("127.0.0.1");
    else
      m_addr = CAddress("0.0.0.0");
  }

  // bind the socket ( try from port to port+range )
  for (m_iPort = port; m_iPort <= port + range; ++m_iPort)
  {
    if (m_ipv6Socket)
      m_addr.saddr.saddr6.sin6_port = htons(m_iPort);
    else
      m_addr.saddr.saddr4.sin_port = htons(m_iPort);

    if (bind(m_iSock, (struct sockaddr*)&m_addr.saddr, m_addr.size) != 0)
    {
      CLog::Log(LOGWARNING, "UDP: Error binding socket on port {} (ipv6 : {})", m_iPort,
                m_ipv6Socket ? "true" : "false");
      CLog::Log(LOGWARNING, "UDP: {}", strerror(errno));
    }
    else
    {
      CLog::Log(LOGINFO, "UDP: Listening on port {} (ipv6 : {})", m_iPort,
                m_ipv6Socket ? "true" : "false");
      SetBound();
      SetReady();
      break;
    }
  }

  // check for errors
  if (!Bound())
  {
    CLog::Log(LOGERROR, "UDP: No suitable port found");
    Close();
    return false;
  }

  return true;
}

bool CPosixUDPSocket::CheckIPv6(int port, int range)
{
  CAddress testaddr("::");
#if defined(TARGET_WINDOWS)
  using CAutoPtrSocket = KODI::UTILS::CScopeGuard<SOCKET, INVALID_SOCKET, decltype(closesocket)>;
  CAutoPtrSocket testSocket(closesocket, socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP));
#else
  using CAutoPtrSocket = KODI::UTILS::CScopeGuard<int, -1, decltype(close)>;
  CAutoPtrSocket testSocket(close, socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP));
#endif

  if (static_cast<SOCKET>(testSocket) == INVALID_SOCKET)
  {
    CLog::LogF(LOGDEBUG, "Could not create IPv6 socket: {}", strerror(errno));
    return false;
  }

#ifdef WINSOCK_VERSION
  const char zero = 0;
#else
  int zero = 0;
#endif

  if (setsockopt(static_cast<SOCKET>(testSocket), IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero)) ==
      -1)
  {
    CLog::LogF(LOGDEBUG, "Could not disable IPV6_V6ONLY for socket: {}", strerror(errno));
    return false;
  }

  // Try to bind a socket to validate ipv6 status
  for (; port <= port + range; port++)
  {
    testaddr.saddr.saddr6.sin6_port = htons(port);
    if (bind(static_cast<SOCKET>(testSocket), reinterpret_cast<struct sockaddr*>(&testaddr.saddr),
             testaddr.size) == 0)
    {
      CLog::LogF(LOGDEBUG, "IPv6 socket bound successfully");
      return true;
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Could not bind IPv6 socket: {}", strerror(errno));
    }
  }

  return false;
}

void CPosixUDPSocket::Close()
{
  if (m_iSock>=0)
  {
    close(m_iSock);
    m_iSock = INVALID_SOCKET;
  }
  SetBound(false);
  SetReady(false);
}

int CPosixUDPSocket::Read(CAddress& addr, const int buffersize, void *buffer)
{
  if (m_ipv6Socket)
    addr.SetAddress("::");
  return (int)recvfrom(m_iSock, (char*)buffer, (size_t)buffersize, 0,
                       (struct sockaddr*)&addr.saddr, &addr.size);
}

int CPosixUDPSocket::SendTo(const CAddress& addr, const int buffersize,
                          const void *buffer)
{
  return (int)sendto(m_iSock, (const char *)buffer, (size_t)buffersize, 0,
                     (const struct sockaddr*)&addr.saddr, addr.size);
}

/**********************************************************************/
/* CSocketFactory                                                     */
/**********************************************************************/

std::unique_ptr<CUDPSocket> CSocketFactory::CreateUDPSocket()
{
  return std::make_unique<CPosixUDPSocket>();
}

/**********************************************************************/
/* CSocketListener                                                    */
/**********************************************************************/

CSocketListener::CSocketListener()
{
  Clear();
}

void CSocketListener::AddSocket(CBaseSocket *sock)
{
  // WARNING: not threadsafe (which is ok for now)

  if (sock && sock->Ready())
  {
    m_sockets.push_back(sock);
    FD_SET(sock->Socket(), &m_fdset);
#ifndef WINSOCK_VERSION
    if (sock->Socket() > m_iMaxSockets)
      m_iMaxSockets = sock->Socket();
#endif
  }
}

bool CSocketListener::Listen(int timeout)
{
  if (m_sockets.size()==0)
  {
    CLog::Log(LOGERROR, "SOCK: No sockets to listen for");
    throw LISTENEMPTY;
  }

  m_iReadyCount = 0;
  m_iCurrentSocket = 0;

  FD_ZERO(&m_fdset);
  for (unsigned int i = 0 ; i<m_sockets.size() ; i++)
  {
    FD_SET(m_sockets[i]->Socket(), &m_fdset);
  }

  // set our timeout
  struct timeval tv;
  int rem = timeout % 1000;
  tv.tv_usec = rem * 1000;
  tv.tv_sec = timeout / 1000;

  m_iReadyCount = select(m_iMaxSockets+1, &m_fdset, NULL, NULL, (timeout < 0 ? NULL : &tv));

  if (m_iReadyCount<0)
  {
    CLog::Log(LOGERROR, "SOCK: Error selecting socket(s)");
    Clear();
    throw LISTENERROR;
  }
  else
  {
    m_iCurrentSocket = 0;
    return (m_iReadyCount>0);
  }
}

void CSocketListener::Clear()
{
  m_sockets.clear();
  FD_ZERO(&m_fdset);
  m_iReadyCount = 0;
  m_iMaxSockets = 0;
  m_iCurrentSocket = 0;
}

CBaseSocket* CSocketListener::GetFirstReadySocket()
{
  if (m_iReadyCount<=0)
    return NULL;

  for (int i = 0 ; i < (int)m_sockets.size() ; i++)
  {
    if (FD_ISSET((m_sockets[i])->Socket(), &m_fdset))
    {
      m_iCurrentSocket = i;
      return m_sockets[i];
    }
  }
  return NULL;
}

CBaseSocket* CSocketListener::GetNextReadySocket()
{
  if (m_iReadyCount<=0)
    return NULL;

  for (int i = m_iCurrentSocket+1 ; i<(int)m_sockets.size() ; i++)
  {
    if (FD_ISSET(m_sockets[i]->Socket(), &m_fdset))
    {
      m_iCurrentSocket = i;
      return m_sockets[i];
    }
  }
  return NULL;
}
