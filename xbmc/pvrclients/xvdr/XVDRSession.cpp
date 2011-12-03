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
#include "client.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "responsepacket.h"
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
  : m_fd(INVALID_SOCKET)
  , m_protocol(0)
  , m_connectionLost(false)
{
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

  tcp_close(m_fd);
  m_fd = INVALID_SOCKET;
}

bool cXVDRSession::Open(const std::string& hostname, int port, const char *name)
{
  Close();

  char errbuf[128];
  m_fd = tcp_connect(hostname.c_str(), port, errbuf, sizeof(errbuf), g_iConnectTimeout * 1000);

  if (m_fd == INVALID_SOCKET)
  {
    XBMC->Log(LOG_ERROR, "%s - Can't connect to XVDR Server: %s", __FUNCTION__, errbuf);
    return false;
  }

  // store connection data
  m_hostname = hostname;
  m_port = port;

  if(name != NULL)
    m_name = name;

  return true;
}

bool cXVDRSession::Login()
{
  try
  {
    cRequestPacket vrp;
    if (!vrp.init(XVDR_LOGIN))                  throw "Can't init cRequestPacket";
    if (!vrp.add_U32(XVDRPROTOCOLVERSION))      throw "Can't add protocol version to RequestPacket";
#ifdef HAVE_ZLIB
    if (!vrp.add_U8(g_iCompression))            throw "Can't add compression parameter";
#else
    if (!vrp.add_U8(0))                         throw "Can't add compression parameter";
#endif
    if (!m_name.empty())
    {
      if (!vrp.add_String(m_name.c_str()))      throw "Can't add client name to RequestPacket";
    }
    else
    {
      if (!vrp.add_String("XBMC Media Center")) throw "Can't add client name to RequestPacket";
    }

    const char* code = XBMC->GetDVDMenuLanguage();
    const char* lang = ISO639_FindLanguage(code);

    XBMC->Log(LOG_INFO, "Preferred Audio Language: %s", lang);

    if (!vrp.add_String((lang != NULL) ? lang : "")) throw "Can't language to RequestPacket";
    if (!vrp.add_U8(g_iAudioType))              throw "Can't add audiotype parameter";

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

    if (m_name.empty())
      XBMC->Log(LOG_NOTICE, "Logged in at '%u+%i' to '%s' Version: '%s' with protocol version '%u'",
        vdrTime, vdrTimeOffset, ServerName, ServerVersion, protocol);

    delete[] ServerName;
    delete[] ServerVersion;

    delete vresp;
  }
  catch (const char * str)
  {
    XBMC->Log(LOG_ERROR, "%s - %s", __FUNCTION__,str);
    tcp_close(m_fd);
    m_fd = INVALID_SOCKET;
    return false;
  }

  return true;
}

cResponsePacket* cXVDRSession::ReadMessage()
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

    vresp = new cResponsePacket();
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

    vresp = new cResponsePacket();
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
  return (tcp_send(m_fd, vrp->getPtr(), vrp->getLen(), 0) == (int)vrp->getLen());
}

cResponsePacket* cXVDRSession::ReadResult(cRequestPacket* vrp)
{
  if(!SendMessage(vrp))
  {
    SignalConnectionLost();
    return NULL;
  }

  cResponsePacket *pkt = NULL;

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
  cResponsePacket *pkt = NULL;
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
  if(!Open(m_hostname, m_port))
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
  return (tcp_read_timeout(m_fd, buffer, totalBytes, g_iConnectTimeout * 1000) == 0);
}

void cXVDRSession::SleepMs(int ms)
{
#ifdef __WINDOWS__
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}
