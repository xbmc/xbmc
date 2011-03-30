/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      http://www.xbmc.org
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

#include "VNSISession.h"
#include "client.h"

// windows specific

#ifdef __WINDOWS__
#include <winsock2.h>
#include <ws2tcpip.h>
#define SHUT_RDWR SD_BOTH
#undef SendMessage

// other (linux) specific

#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <poll.h>
#define closesocket close
#endif

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "responsepacket.h"
#include "requestpacket.h"
#include "vdrcommand.h"
#include "tools.h"

/* Needed on Mac OS/X */
 
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

cVNSISession::cVNSISession()
  : m_fd(INVALID_SOCKET)
  , m_protocol(0)
{
}

cVNSISession::~cVNSISession()
{
  Close();
}

void cVNSISession::Abort()
{
  shutdown(m_fd, SHUT_RDWR);
}

void cVNSISession::Close()
{
  if(m_fd != INVALID_SOCKET)
  {
    closesocket(m_fd);
    m_fd = INVALID_SOCKET;
  }
}

bool cVNSISession::Open(const std::string& hostname, int port, const char *name)
{
  struct hostent *hp;
  int fd, r;
  struct sockaddr_in in;
  struct sockaddr_in6 in6;

  if (port == 0)
    port = 34890;

  hp = gethostbyname(hostname.c_str());

  if(hp == NULL)
  {
    switch(h_errno)
    {
      case HOST_NOT_FOUND:
        XBMC->Log(LOG_ERROR, "%s - The specified host is unknown", __FUNCTION__);
        break;
      case NO_ADDRESS:
        XBMC->Log(LOG_ERROR, "%s - The requested name is valid but does not have an IP address", __FUNCTION__);
        break;
      case NO_RECOVERY:
        XBMC->Log(LOG_ERROR, "%s - A non-recoverable name server error occurred", __FUNCTION__);
        break;
      case TRY_AGAIN:
        XBMC->Log(LOG_ERROR, "%s - A temporary error occurred on an authoritative name server", __FUNCTION__);
        break;
      default:
        XBMC->Log(LOG_ERROR, "%s - Unknown error", __FUNCTION__);
        break;
    }

    return false;
  }

  fd = socket(hp->h_addrtype, SOCK_STREAM, 0);
  if (fd == -1)
  {
    XBMC->Log(LOG_ERROR, "%s - Unable to create socket: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  /**
   * Switch to nonblocking
   */
#ifdef __WINDOWS__
  u_long nb = 1;
  ioctlsocket(fd, FIONBIO, &nb);
#else
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
#endif

  switch(hp->h_addrtype)
  {
    case AF_INET:
      memset(&in, 0, sizeof(in));
      in.sin_family = AF_INET;
      in.sin_port = htons(port);
      memcpy(&in.sin_addr, hp->h_addr_list[0], sizeof(struct in_addr));
      r = connect(fd, (struct sockaddr *)&in, sizeof(struct sockaddr_in));
      break;

    case AF_INET6:
      memset(&in6, 0, sizeof(in6));
      in6.sin6_family = AF_INET6;
      in6.sin6_port = htons(port);
      memcpy(&in6.sin6_addr, hp->h_addr_list[0], sizeof(struct in6_addr));
      r = connect(fd, (struct sockaddr *)&in, sizeof(struct sockaddr_in6));
      break;

    default:
      XBMC->Log(LOG_ERROR, "cVNSISession::Open - Invalid protocol family");
      return false;
  }

  if (r == -1)
  {
#ifdef __WINDOWS__
    if (WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == EAGAIN)
#else
    if (errno == EINPROGRESS)
#endif
    {
      fd_set fd_write, fd_except;
      struct timeval tv;

      tv.tv_sec  = g_iConnectTimeout;
      tv.tv_usec = 0;

      FD_ZERO(&fd_write);
      FD_ZERO(&fd_except);

      FD_SET(fd, &fd_write);
      FD_SET(fd, &fd_except);

      r = select((int)fd+1, NULL, &fd_write, &fd_except, &tv);

      // Timeout
      if (r == 0)
      {
        XBMC->Log(LOG_ERROR, "Connection attempt timed out %i", g_iConnectTimeout);
      }
    }
  }

  if (r <= 0)
  {
    XBMC->Log(LOG_ERROR, "%s", strerror(errno));
    Close();
    return false;
  }

#ifdef __WINDOWS__
  nb = 0;
  ioctlsocket(fd, FIONBIO, &nb);
  char val = 1;
#else
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
  int val = 1;
#endif

  setsockopt(fd, SOL_TCP, TCP_NODELAY, &val, sizeof(val));

  try
  {
    m_fd = fd;
    if (m_fd == INVALID_SOCKET)
      throw "Can't connect to VSNI Server";

    cRequestPacket vrp;
    if (!vrp.init(VDR_LOGIN))                 throw "Can't init cRequestPacket";
    if (!vrp.add_U32(VNSIProtocolVersion))    throw "Can't add protocol version to RequestPacket";
    if (!vrp.add_U8(false))                   throw "Can't add netlog flag";
    if (name && strlen(name) > 0)
    {
      if (!vrp.add_String(name))                throw "Can't add client name to RequestPacket";
    }
    else
    {
      if (!vrp.add_String("XBMC Media Center")) throw "Can't add client name to RequestPacket";
    }

    // read welcome
    cResponsePacket* vresp = ReadResult(&vrp);
    if (!vresp)
      throw "failed to read greeting from server";

    uint32_t    protocol      = vresp->extract_U32();
    uint32_t    vdrTime       = vresp->extract_U32();
    int32_t     vdrTimeOffset = vresp->extract_S32();
    const char *ServerName    = vresp->extract_String();
    const char *ServerVersion = vresp->extract_String();

    m_server    = ServerName;
    m_version   = ServerVersion;
    m_protocol  = protocol;

    if (!name || strlen(name) <= 0)
      XBMC->Log(LOG_NOTICE, "Logged in at '%lu+%lu' to '%s' Version: '%s' with protocol version '%lu'", vdrTime, vdrTimeOffset, ServerName, ServerVersion, protocol);

    delete vresp;
  }
  catch (const char * str)
  {
    XBMC->Log(LOG_ERROR, "cVNSISession::Open - %s", str);
    close(m_fd);
    m_fd = INVALID_SOCKET;
    return false;
  }

  return true;
}

cResponsePacket* cVNSISession::ReadMessage()
{
  uint32_t channelID = 0;
  uint32_t requestID;
  uint32_t userDataLength = 0;
  uint8_t* userData = NULL;
  uint32_t streamID;
  uint32_t duration;
  uint32_t opCodeID;
  int64_t  dts = 0;
  int64_t  pts = 0;

  cResponsePacket* vresp = NULL;

  bool readSuccess = readData((uint8_t*)&channelID, sizeof(uint32_t)) > 0;
  if (!readSuccess)
    return NULL;

  // Data was read

  channelID = ntohl(channelID);
  if (channelID == CHANNEL_REQUEST_RESPONSE)
  {
    if (!readData((uint8_t*)&m_responsePacketHeader, sizeof(m_responsePacketHeader))) return NULL;

    requestID = ntohl(m_responsePacketHeader.requestID);
    userDataLength = ntohl(m_responsePacketHeader.userDataLength);

    if (userDataLength > 5000000) return NULL; // how big can these packets get?
    userData = NULL;
    if (userDataLength > 0)
    {
      userData = (uint8_t*)malloc(userDataLength);
      if (!userData) return NULL;
      if (!readData(userData, userDataLength)) {
        free(userData);
        return NULL;
      }
    }

    vresp = new cResponsePacket();
    vresp->setResponse(requestID, userData, userDataLength);
  }
  else if (channelID == CHANNEL_STREAM)
  {
    if (!readData((uint8_t*)&m_streamPacketHeader, sizeof(m_streamPacketHeader))) return NULL;

    opCodeID = ntohl(m_streamPacketHeader.opCodeID);
    streamID = ntohl(m_streamPacketHeader.streamID);
    duration = ntohl(m_streamPacketHeader.duration);
    pts = ntohll(*(int64_t*)m_streamPacketHeader.pts);
    dts = ntohll(*(int64_t*)m_streamPacketHeader.dts);
    userDataLength = ntohl(m_streamPacketHeader.userDataLength);

    if(opCodeID == VDR_STREAM_MUXPKT) {
      DemuxPacket* p = PVR->AllocateDemuxPacket(userDataLength);
      userData = (uint8_t*)p;
      if (userDataLength > 0)
      {
        if (!userData) return NULL;
        if (!readData(p->pData, userDataLength))
        {
          PVR->FreeDemuxPacket(p);
          return NULL;
        }
      }
    }
    else if (userDataLength > 0) {
      userData = (uint8_t*)malloc(userDataLength);
      if (!userData) return NULL;
      if (!readData(userData, userDataLength))
      {
        free(userData);
        return NULL;
      }
    }

    vresp = new cResponsePacket();
    vresp->setStream(opCodeID, streamID, duration, dts, pts, userData, userDataLength);
  }
  else
  {
    XBMC->Log(LOG_ERROR, "cVNSISession::ReadMessage() - Rxd a response packet on channel %lu !!", channelID);
  }

  return vresp;
}

bool cVNSISession::SendMessage(cRequestPacket* vrp)
{
  if (sendData(vrp->getPtr(), vrp->getLen()) != vrp->getLen())
    return false;

  return true;
}

cResponsePacket* cVNSISession::ReadResult(cRequestPacket* vrp, bool sequence)
{
  if(!SendMessage(vrp))
    return NULL;

  cResponsePacket *pkt = NULL;

  while((pkt = ReadMessage()))
  {
    /* Discard everything other as response packets until it is received */
    if (pkt->getChannelID() == CHANNEL_REQUEST_RESPONSE
        && (!sequence || pkt->getRequestID() == vrp->getSerial()))
    {
      return pkt;
    }
    else
      delete pkt;
  }
  return NULL;
}

bool cVNSISession::ReadSuccess(cRequestPacket* vrp, bool sequence)
{
  cResponsePacket *pkt = NULL;
  if((pkt = ReadResult(vrp, sequence)) == NULL)
  {
    return false;
  }
  uint32_t retCode = pkt->extract_U32();
  delete pkt;

  if(retCode != VDR_RET_OK)
  {
    XBMC->Log(LOG_ERROR, "cVNSISession::ReadSuccess - failed with error code '%i'", retCode);
    return false;
  }
  return true;
}

int cVNSISession::sendData(void* bufR, size_t count)
{
  size_t bytes_sent = 0;
  int this_write;
  int temp_write;

  unsigned char* buf = (unsigned char*)bufR;

  while (bytes_sent < count)
  {
#ifdef __WINDOWS__
    do
    {
      temp_write = this_write = send(m_fd,(char*) buf, count- bytes_sent,0);
    } while ( (this_write == SOCKET_ERROR) && (WSAGetLastError() == WSAEINTR) );
#else
    {
      temp_write = this_write = write(m_fd, buf, count - bytes_sent);
    } while ( (this_write < 0) && (errno == EINTR) );
#endif
    if (this_write <= 0)
    {
      return(this_write);
    }
    bytes_sent += this_write;
    buf += this_write;
  }

  return(count);
}

int cVNSISession::readData(uint8_t* buffer, int totalBytes)
{
  int bytesRead = 0;
  int thisRead;
  int success;
  fd_set readSet;
  struct timeval timeout;

  while(1)
  {
    FD_ZERO(&readSet);
    FD_SET(m_fd, &readSet);
    timeout.tv_sec = g_iConnectTimeout;
    timeout.tv_usec = 0;
    success = select(m_fd + 1, &readSet, NULL, NULL, &timeout);
    if (success < 1)
    {
      return 0;  // error, or timeout
    }
#ifdef __WINDOWS__
    thisRead = recv(m_fd, (char*)&buffer[bytesRead], totalBytes - bytesRead, 0);
#else
    thisRead = read(m_fd, &buffer[bytesRead], totalBytes - bytesRead);
#endif
    if (!thisRead)
    {
      // if read returns 0 then connection is closed
      // in non-blocking mode if read is called with no data available, it returns -1
      // and sets errno to EGAGAIN. but we use select so it wouldn't do that anyway.
      XBMC->Log(LOG_ERROR, "cVNSISession::readData - Detected connection closed");
      return -1;
    }
    bytesRead += thisRead;
    if (bytesRead == totalBytes)
    {
      return 1;
    }
  }
}
