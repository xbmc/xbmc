/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "IRServerSuite.h"
#include "AppInboundProtocol.h"
#include "IrssMessage.h"
#include "platform/win32/CharsetConverter.h"
#include "profiles/ProfilesManager.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#include <WS2tcpip.h>

#define IRSS_PORT 24000
#define IRSS_MAP_FILENAME "IRSSmap.xml"

CIRServerSuite::CIRServerSuite()
  : CThread("RemoteControl")
  , m_bInitialized(false)
  , m_socket(INVALID_SOCKET)
  , m_isConnecting(false)
{
}

CIRServerSuite::~CIRServerSuite()
{
  m_event.Set();
  {
    CSingleLock lock(m_critSection);
    Close();
  }
  StopThread();
}

void CIRServerSuite::Close()
{
  m_isConnecting = false;
  if (m_socket != INVALID_SOCKET)
  {
    if (m_bInitialized)
    {
      m_bInitialized = false;
      CIrssMessage message(IRSSMT_UnregisterClient, IRSSMF_Request | IRSSMF_ForceNotRespond);
      SendPacket(message);
    }
    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }
}

void CIRServerSuite::Initialize()
{
  Create();
  SetPriority(GetMinPriority());
}

void CIRServerSuite::Process()
{
  m_profileId = CServiceBroker::GetProfileManager().GetCurrentProfileId();
  m_irTranslator.Load(IRSS_MAP_FILENAME);

  bool logging = true;

  // try to connect continuously with a 5 second interval
  // multiple tries because:
  // 1. irss might be up and running a little later on boot.
  // 2. irss might be restarted
  while (!m_bStop)
  {
    if (!Connect(logging))
    {
      if (logging)
        CLog::LogF(LOGINFO, "failed to connect to irss, will keep retrying every 5 seconds");

      if (AbortableWait(m_event, 5000) == WAIT_SIGNALED)
        break;

      logging = false;
      continue;
    }

    while (!m_bStop)
    {
      if (!ReadNext())
      {
        logging = true;
        break;
      }
    }
  }
  Close();
}

bool CIRServerSuite::Connect(bool logMessages)
{
  char     namebuf[NI_MAXHOST], portbuf[NI_MAXSERV];
  struct   addrinfo hints = {};
  struct   addrinfo *result, *addr;
  char     service[33];
  int      res;

  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  sprintf(service, "%d", IRSS_PORT);

  res = getaddrinfo("localhost", service, &hints, &result);
  if(res)
  {
    if (logMessages)
      CLog::LogF(LOGDEBUG, "getaddrinfo failed: %s",
                KODI::PLATFORM::WINDOWS::FromW(gai_strerror(res)));
    return false;
  }

  for(addr = result; addr; addr = addr->ai_next)
  {
    if(getnameinfo(addr->ai_addr, addr->ai_addrlen, namebuf, sizeof(namebuf), portbuf, sizeof(portbuf),NI_NUMERICHOST))
    {
      strcpy(namebuf, "[unknown]");
      strcpy(portbuf, "[unknown]");
    }

    if (logMessages)
      CLog::LogF(LOGDEBUG, "connecting to: %s:%s ...", namebuf, portbuf);

    m_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(m_socket == INVALID_SOCKET)
      continue;

    if(connect(m_socket, addr->ai_addr, addr->ai_addrlen) != SOCKET_ERROR)
      break;

    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }

  freeaddrinfo(result);
  if(m_socket == INVALID_SOCKET)
  {
    if (logMessages)
      CLog::LogF(LOGDEBUG, "failed to connect");
    Close();
    return false;
  }

  u_long iMode = 1; //non-blocking
  if (ioctlsocket(m_socket, FIONBIO, &iMode) == SOCKET_ERROR)
  {
    if (logMessages)
      CLog::LogF(LOGERROR, "failed to set socket to non-blocking.");
    Close();
    return false;
  }

  //register
  CIrssMessage mess(IRSSMT_RegisterClient, IRSSMF_Request);
  if (!SendPacket(mess))
  {
    if (logMessages)
      CLog::LogF(LOGERROR, "failed to send RegisterClient packet.");
    return false;
  }
  m_isConnecting = true;
  return true;
}

bool CIRServerSuite::SendPacket(CIrssMessage& message)
{
  int iSize = 0;
  char* bytes = message.ToBytes(iSize);
  char buffer[4];
  uint32_t len = htonl(iSize);
  memcpy(&buffer[0], &len, 4);
  bool bResult = WriteN(&buffer[0], 4);
  if (bResult)
  {
    bResult = WriteN(bytes, iSize);
  }
  delete[] bytes;
  if (!bResult)
  {
    Close();
    return false;
  }
  return true;
}

bool CIRServerSuite::ReadNext()
{
  if ((!m_bInitialized && !m_isConnecting) || (m_socket == INVALID_SOCKET))
  {
    return false;
  }

  CIrssMessage mess;
  int res = ReadPacket(mess);
  if (res < 0)
  {
    Close();
    return false;
  }

  // nothing received
  if (!res)
    return true;

  switch (mess.GetType())
  {
  case IRSSMT_RegisterClient:
    m_isConnecting = false;
    if ((mess.GetFlags() & IRSSMF_Success) != IRSSMF_Success)
    {
      //uh oh, it failed to register
      Close();
      CLog::LogF(LOGERROR, "failed to register application as a client.");
      return false;
    }
    else
    {
      m_bInitialized = true;
      //request info about receivers
      CIrssMessage mess(IRSSMT_DetectedReceivers, IRSSMF_Request);
      if (!SendPacket(mess))
      {
        CLog::LogF(LOGERROR, "failed to send `AvailableReceivers` packet.");
        return false;
      }
      mess.SetType(IRSSMT_AvailableReceivers);
      if (!SendPacket(mess))
      {
        CLog::LogF(LOGERROR, "failed to send `AvailableReceivers` packet.");
        return false;
      }
      mess.SetType(IRSSMT_ActiveReceivers);
      if (!SendPacket(mess))
      {
        CLog::LogF(LOGERROR, "failed to send `AvailableReceivers` packet.");
        return false;
      }
    }
    break;
  case IRSSMT_RemoteEvent:
    HandleRemoteEvent(mess);
    break;
  case IRSSMT_Error:
    //I suppose the errormessage is in the packet somewhere...
    CLog::LogF(LOGERROR, "we got an error message.");
    break;
  case IRSSMT_ServerShutdown:
    Close();
    return false;
  case IRSSMT_ServerSuspend:
    //should we do something?
    break;
  case IRSSMT_ServerResume:
    //should we do something?
    break;
  case IRSSMT_AvailableReceivers:
    {
      uint32_t size = mess.GetDataSize();
      if (size > 0)
      {
        char* data = mess.GetData();
        char* availablereceivers = new char[size + 1];
        memcpy(availablereceivers, data, size);
        availablereceivers[size] = '\0';
        CLog::LogF(LOGINFO, "available receivers: %s", availablereceivers);
        delete[] availablereceivers;
      }
    }
    break;
  case IRSSMT_DetectedReceivers:
    {
      uint32_t size = mess.GetDataSize();
      if (size > 0)
      {
        char* data = mess.GetData();
        char* detectedreceivers = new char[size + 1];
        memcpy(detectedreceivers, data, size);
        detectedreceivers[size] = '\0';
        CLog::LogF(LOGINFO, "detected receivers: %s", detectedreceivers);
        delete[] detectedreceivers;
      }
    }
    break;
  case IRSSMT_ActiveReceivers:
    {
      uint32_t size = mess.GetDataSize();
      if (size > 0)
      {
        char* data = mess.GetData();
        char* activereceivers = new char[size + 1];
        memcpy(activereceivers, data, size);
        activereceivers[size] = '\0';
        CLog::LogF(LOGINFO, "active receivers: %s", activereceivers);
        delete[] activereceivers;
      }
    }
    break;
  }
  return true;
}

bool CIRServerSuite::HandleRemoteEvent(CIrssMessage& message)
{
  try
  {
    //flag should be notify, maybe check it?
    char* data = message.GetData();
    uint32_t datalen = message.GetDataSize();
    char* deviceName;
    char* keycode;
    uint32_t devicenamelength;
    uint32_t keycodelength;
    if (datalen == 0)
    {
      CLog::LogF(LOGERROR, "no data in remote message.");
      return false;
    }
    if (datalen <= 8)
    {
      //seems to be version 1.0.4.1, only keycode is sent, use Microsoft MCE mapping??
      devicenamelength = 13;
      deviceName = new char[devicenamelength + 1];
      sprintf(deviceName, "Microsoft MCE");
      keycodelength = datalen;
      keycode = new char[keycodelength + 1];
      memcpy(keycode, data, keycodelength);
    }
    else
    {
      //first 4 bytes is devicename length
      memcpy(&devicenamelength, data, 4);
      //devicename itself
      if (datalen < 4 + devicenamelength)
      {
        CLog::LogF(LOGERROR, "invalid data in remote message (size: %u).", datalen);
        return false;
      }
      deviceName = new char[devicenamelength + 1];
      memcpy(deviceName, data + 4, devicenamelength);
      if (datalen < 8 + devicenamelength)
      {
        CLog::LogF(LOGERROR, "invalid data in remote message (size: %u).", datalen);
        delete[] deviceName;
        return false;
      }
      //next 4 bytes is keycode length
      memcpy(&keycodelength, data + 4 + devicenamelength, 4);
      //keycode itself
      if (datalen < 8 + devicenamelength + keycodelength)
      {
        CLog::LogF(LOGERROR, "invalid data in remote message (size: %u).", datalen);
        delete[] deviceName;
        return false;
      }
      keycode = new char[keycodelength + 1];
      memcpy(keycode, data + 8 + devicenamelength, keycodelength);
    }
    deviceName[devicenamelength] = '\0';
    keycode[keycodelength] = '\0';

    if (m_profileId != CServiceBroker::GetProfileManager().GetCurrentProfileId())
    {
      m_profileId = CServiceBroker::GetProfileManager().GetCurrentProfileId();
      m_irTranslator.Load(IRSS_MAP_FILENAME);
    }

    //translate to a buttoncode xbmc understands
    CLog::LogF(LOGDEBUG, "remoteEvent: %s %s", deviceName, keycode);
    unsigned button = m_irTranslator.TranslateButton(deviceName, keycode);

    XBMC_Event newEvent;
    newEvent.type = XBMC_BUTTON;
    newEvent.keybutton.button = button;
    newEvent.keybutton.holdtime = 0;
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      appPort->OnEvent(newEvent);

    delete[] deviceName;
    delete[] keycode;
    return true;
  }
  catch(...)
  {
    CLog::LogF(LOGERROR, "exception while processing RemoteEvent.");
    return false;
  }
}

int CIRServerSuite::ReadN(char *buffer, int n)
{
  int nOriginalSize = n;
  memset(buffer, 0, n);
  char *ptr = buffer;
  while (n > 0)
  {
    int nBytes = 0;
    {
      CSingleLock lock(m_critSection);
      nBytes = recv(m_socket, ptr, n, 0);
    }

    if (WSAGetLastError() == WSAEWOULDBLOCK)
    {
      return nOriginalSize - n;
    }
    if (nBytes < 0)
    {
      if (!m_isConnecting)
      {
        CLog::LogF(LOGERROR, "recv error %d", WSAGetLastError());
      }
      Close();
      return -1;
    }

    if (nBytes == 0)
    {
      CLog::LogF(LOGDEBUG, "socket closed by server");
      Close();
      break;
    }

    n -= nBytes;
    ptr += nBytes;
  }

  return nOriginalSize - n;
}

bool CIRServerSuite::WriteN(const char *buffer, int n)
{
  const char *ptr = buffer;
  while (n > 0)
  {
    int nBytes;
    {
      CSingleLock lock(m_critSection);
      nBytes = send(m_socket, ptr, n, 0);
    }

    if (nBytes < 0)
    {
      CLog::LogF(LOGERROR, "send error %d (%d bytes)", WSAGetLastError(), n);
      Close();
      return false;
    }

    if (nBytes == 0)
      break;

    n -= nBytes;
    ptr += nBytes;
  }

  return n == 0;
}

int CIRServerSuite::ReadPacket(CIrssMessage &message)
{
  try
  {
    char sizebuf[4];
    int iRead = ReadN(&sizebuf[0], 4);
    if (iRead <= 0)
      return iRead; // error or nothing to read

    if (iRead != 4)
    {
      CLog::LogF(LOGERROR, "failed to read packetsize.");
      return -1;
    }

    uint32_t size = 0;
    memcpy(&size, &sizebuf[0], 4);
    size = ntohl(size);
    char* messagebytes = new char[size];

    if (ReadN(messagebytes, size) != size)
    {
      CLog::LogF(LOGERROR, "failed to read packet.");
      delete[] messagebytes;
      return false;
    }

    if (!CIrssMessage::FromBytes(messagebytes, size, message))
    {
      CLog::LogF(LOGERROR, "invalid packet received (size: %u).", size);
      delete[] messagebytes;
      return -1;
    }

    delete[] messagebytes;
    return size;
  }
  catch(...)
  {
    CLog::LogF(LOGERROR, "exception while processing packet.");
    return -1;
  }
}
