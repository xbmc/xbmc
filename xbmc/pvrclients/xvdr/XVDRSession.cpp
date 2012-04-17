/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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

#include "XVDRSession.h"
#include "XVDRResponsePacket.h"
#include "client.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "requestpacket.h"
#include "xvdrcommand.h"
#include "tools.h"
#include "iso639.h"

/* Needed on Mac OS/X */
 
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

using namespace ADDON;

cXVDRSession::cXVDRSession()
  : m_timeout(3000)
  , m_fd(INVALID_SOCKET)
  , m_protocol(0)
  , m_connectionLost(false)
  , m_compressionlevel(0)
  , m_audiotype(0)
{
  m_port = 34891;
}

cXVDRSession::~cXVDRSession()
{
  Close();
}

void cXVDRSession::Abort()
{
  tcp_shutdown(m_fd);
}

void cXVDRSession::Close()
{
  if(!IsOpen())
    return;

  Abort();

  tcp_close(m_fd);
  m_fd = INVALID_SOCKET;
}

bool cXVDRSession::Open(const std::string& hostname, const char *name)
{
  Close();

  char errbuf[128];
  errbuf[0] = 0;

  m_fd = tcp_connect(hostname.c_str(), m_port, errbuf, sizeof(errbuf), m_timeout);

  if (m_fd == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't connect to XVDR Server: %s", __FUNCTION__, errbuf);
    return false;
  }

  // store connection data
  m_hostname = hostname;

  if(name != NULL)
    m_name = name;

  return true;
}

bool cXVDRSession::Login()
{
  cRequestPacket vrp;

  if (!vrp.init(XVDR_LOGIN))
    return false;
  if (!vrp.add_U32(XVDRPROTOCOLVERSION))
    return false;
#ifdef HAVE_ZLIB
  if (!vrp.add_U8(m_compressionlevel))
    return false;
#else
  if (!vrp.add_U8(0))
    return false;
#endif
  if (!vrp.add_String(m_name.empty() ? "XBMC Media Center" : m_name.c_str()))
    return false;

  const char* code = XBMC->GetDVDMenuLanguage();
  const char* lang = ISO639_FindLanguage(code);

  if (!vrp.add_String((lang != NULL) ? lang : ""))
    return false;
  if (!vrp.add_U8(m_audiotype))
    return false;

  // read welcome
  cXVDRResponsePacket* vresp = ReadResult(&vrp);
  if (!vresp)
  {
    XBMC->Log(LOG_ERROR, "failed to read greeting from server");
    return false;
  }

  m_protocol                = vresp->extract_U32();
  uint32_t    vdrTime       = vresp->extract_U32();
  int32_t     vdrTimeOffset = vresp->extract_S32();
  m_server                  = vresp->extract_String();
  m_version                 = vresp->extract_String();

  if (m_name.empty())
    XBMC->Log(LOG_NOTICE, "Logged in at '%u+%i' to '%s' Version: '%s' with protocol version '%u'", vdrTime, vdrTimeOffset, m_server.c_str(), m_version.c_str(), m_protocol);

  XBMC->Log(LOG_INFO, "Preferred Audio Language: %s", lang);

  delete vresp;

  return true;
}

cXVDRResponsePacket* cXVDRSession::ReadMessage()
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

  cXVDRResponsePacket* vresp = NULL;

  if(!readData((uint8_t*)&channelID, sizeof(uint32_t)))
    return NULL;

  // Data was read

  bool compressed = (channelID & htonl(0x80000000));

  if(compressed)
    channelID ^= htonl(0x80000000);

  channelID = ntohl(channelID);

  if (channelID == XVDR_CHANNEL_STREAM)
  {
    if (!readData((uint8_t*)&m_streamPacketHeader, sizeof(m_streamPacketHeader))) return NULL;

    opCodeID = ntohl(m_streamPacketHeader.opCodeID);
    streamID = ntohl(m_streamPacketHeader.streamID);
    duration = ntohl(m_streamPacketHeader.duration);
    pts = ntohll(*(int64_t*)m_streamPacketHeader.pts);
    dts = ntohll(*(int64_t*)m_streamPacketHeader.dts);
    userDataLength = ntohl(m_streamPacketHeader.userDataLength);

    if(opCodeID == XVDR_STREAM_MUXPKT) {
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

    vresp = new cXVDRResponsePacket();
    vresp->setStream(opCodeID, streamID, duration, dts, pts, userData, userDataLength);
  }
  else
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

    vresp = new cXVDRResponsePacket();
    if (channelID == XVDR_CHANNEL_STATUS)
      vresp->setStatus(requestID, userData, userDataLength);
    else
      vresp->setResponse(requestID, userData, userDataLength);

    if(compressed)
      vresp->uncompress();
  }

  return vresp;
}

bool cXVDRSession::SendMessage(cRequestPacket* vrp)
{
  return (tcp_send_timeout(m_fd, vrp->getPtr(), vrp->getLen(), m_timeout) == 0);
}

cXVDRResponsePacket* cXVDRSession::ReadResult(cRequestPacket* vrp)
{
  if(!SendMessage(vrp))
  {
    SignalConnectionLost();
    return NULL;
  }

  cXVDRResponsePacket *pkt = NULL;

  while((pkt = ReadMessage()))
  {
    /* Discard everything other as response packets until it is received */
    if (pkt->getChannelID() == XVDR_CHANNEL_REQUEST_RESPONSE && pkt->getRequestID() == vrp->getSerial())
    {
      return pkt;
    }
    else
      delete pkt;
  }

  SignalConnectionLost();
  return NULL;
}

bool cXVDRSession::ReadSuccess(cRequestPacket* vrp) {
  uint32_t rc;
  return ReadSuccess(vrp, rc);
}

bool cXVDRSession::ReadSuccess(cRequestPacket* vrp, uint32_t& rc)
{
  cXVDRResponsePacket *pkt = NULL;
  if((pkt = ReadResult(vrp)) == NULL)
    return false;

  rc = pkt->extract_U32();
  delete pkt;

  if(rc != XVDR_RET_OK)
  {
    XBMC->Log(LOG_ERROR, "%s - failed with error code '%i'", __FUNCTION__, rc);
    return false;
  }

  return true;
}

void cXVDRSession::OnReconnect() {
}

void cXVDRSession::OnDisconnect() {
}

bool cXVDRSession::TryReconnect() {
  if(!Open(m_hostname))
    return false;

  if(!Login())
    return false;

  XBMC->Log(LOG_DEBUG, "%s - reconnected", __FUNCTION__);
  m_connectionLost = false;

  OnReconnect();

  return true;
}

void cXVDRSession::SignalConnectionLost()
{
  if(m_connectionLost)
    return;

  XBMC->Log(LOG_ERROR, "%s - connection lost !!!", __FUNCTION__);

  m_connectionLost = true;
  Abort();
  Close();

  OnDisconnect();
}

bool cXVDRSession::readData(uint8_t* buffer, int totalBytes)
{
  return (tcp_read_timeout(m_fd, buffer, totalBytes, m_timeout) == 0);
}

void cXVDRSession::SetTimeout(int ms)
{
  m_timeout = ms;
}

void cXVDRSession::SetCompressionLevel(int level)
{
  if (level < 0 || level > 9)
    return;

  m_compressionlevel = level;
}

void cXVDRSession::SetAudioType(int type)
{
  m_audiotype = type;
}

void cXVDRSession::SleepMs(int ms)
{
#ifdef __WINDOWS__
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}
