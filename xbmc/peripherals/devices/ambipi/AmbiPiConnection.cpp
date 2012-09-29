/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AmbiPiConnection.h"
#include "utils/log.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace PERIPHERALS;
using namespace AUTOPTR;


CAmbiPiConnection::CAmbiPiConnection(void) :
  CThread("AmbiPi"),
  m_socket(INVALID_SOCKET),
  m_bConnected(false),
  m_bConnecting(false)
{
}

CAmbiPiConnection::~CAmbiPiConnection(void)
{
  Disconnect();
}

void CAmbiPiConnection::Connect(const CStdString ip_address_or_name, unsigned int port)
{
  Disconnect();
  m_ip_address_or_name = ip_address_or_name;
  m_port = port;
  Create();
}

void CAmbiPiConnection::Reconnect() {
  Disconnect();
  Create();
}

void CAmbiPiConnection::Disconnect()
{
  if (m_bConnecting)
  {
    StopThread(true);
  }

  if (!m_bConnected)
  {
    return;
  }

  if (m_socket.isValid())
  {
    m_socket.reset();
  }
  m_bConnected = false;
  CLog::Log(LOGINFO, "%s - disconnected", __FUNCTION__);
}

#define CONNECT_RETRY_DELAY 5

void CAmbiPiConnection::Process(void)
{
  CLog::Log(LOGINFO, "%s - connecting", __FUNCTION__);
  {
    CSingleLock lock(m_critSection);
    m_bConnecting = true;
  }

  while (!m_bStop && !m_bConnected)
  {
    AttemptConnection();

    if (!m_bConnected && !m_bStop)
      Sleep(CONNECT_RETRY_DELAY * 1000);
  }

  {
    CSingleLock lock(m_critSection);
    m_bConnecting = false;
  }
}

void CAmbiPiConnection::AttemptConnection()
{
  struct addrinfo *pAddressInfo;

  BYTE *helloMessage = (BYTE *)"ambipi\n";
  try
  {
    pAddressInfo = GetAddressInfo(m_ip_address_or_name, m_port);
    AttemptConnection(pAddressInfo);
    Send(helloMessage, strlen((char *)helloMessage));
    freeaddrinfo(pAddressInfo);
  } 
  catch (...) 
  {
    CLog::Log(LOGERROR, "%s - connection to AmbiPi failed", __FUNCTION__);
    return;
  }

  {
    CSingleLock lock(m_critSection);
    m_bConnected = true;
  }
  CLog::Log(LOGINFO, "%s - connected", __FUNCTION__);
}

struct CAmbiPiConnectionException : std::exception { char const* what() const throw() { return "Connection exception"; }; };
struct CAmbiPiSendException : std::exception { char const* what() const throw() { return "Send exception"; }; };


struct addrinfo *CAmbiPiConnection::GetAddressInfo(const CStdString ip_address_or_name, unsigned int port)
{
  struct   addrinfo hints, *pAddressInfo;
  char service[33];
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  sprintf(service, "%d", port);

  int res = getaddrinfo(ip_address_or_name.c_str(), service, &hints, &pAddressInfo);
  if(res)
  {
    CLog::Log(LOGERROR, "%s - failed to lookup %s, error: '%s'", __FUNCTION__, ip_address_or_name.c_str(), gai_strerror(res));
    throw CAmbiPiConnectionException();
  }
  return pAddressInfo;
}

void CAmbiPiConnection::AttemptConnection(struct addrinfo *pAddressInfo)
{
  char     nameBuffer[NI_MAXHOST], portBuffer[NI_MAXSERV];

  SOCKET   socketHandle = INVALID_SOCKET;
  struct addrinfo *pCurrentAddressInfo;

  for (pCurrentAddressInfo = pAddressInfo; pCurrentAddressInfo; pCurrentAddressInfo = pCurrentAddressInfo->ai_next)
  {
    if (getnameinfo(
      pCurrentAddressInfo->ai_addr, 
      pCurrentAddressInfo->ai_addrlen,
      nameBuffer, sizeof(nameBuffer),
      portBuffer, sizeof(portBuffer),
      NI_NUMERICHOST)
    )
    {
      strcpy(nameBuffer, "[unknown]");
      strcpy(portBuffer, "[unknown]");
  	}
    CLog::Log(LOGDEBUG, "%s - connecting to: %s:%s ...", __FUNCTION__, nameBuffer, portBuffer);

    socketHandle = socket(pCurrentAddressInfo->ai_family, pCurrentAddressInfo->ai_socktype, pCurrentAddressInfo->ai_protocol);
    if (socketHandle == INVALID_SOCKET)
      continue;

    if (connect(socketHandle, pCurrentAddressInfo->ai_addr, pCurrentAddressInfo->ai_addrlen) != SOCKET_ERROR)
      break;

    closesocket(socketHandle);
    socketHandle = INVALID_SOCKET;
  }

  if(socketHandle == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "%s - failed to connect", __FUNCTION__);
    throw CAmbiPiConnectionException();
  }

  m_socket.attach(socketHandle);
  CLog::Log(LOGINFO, "%s - connected to: %s:%s ...", __FUNCTION__, nameBuffer, portBuffer);
}

void CAmbiPiConnection::Send(const BYTE *buffer, int length)
{
  int iErr = send((SOCKET)m_socket, (const char *)buffer, length, 0);
  if (iErr <= 0)
  {
    CLog::Log(LOGERROR, "%s - send failed", __FUNCTION__);
    if (!m_bConnecting) {
      Reconnect();
    }
  }
}

bool CAmbiPiConnection::IsConnected(void) const
{
  CSingleLock lock(m_critSection);
  return m_bConnected;
}
