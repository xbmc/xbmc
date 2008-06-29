/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "rtmp.h"
#include "AMFObject.h"

#ifdef _LINUX
  #include "PlatformInclude.h"
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <netinet/in.h>
#endif

#ifdef _XBOX
  #include "..\..\cores\DllLoader\exports\emu_socket\emu_socket.h"

  extern "C" {
  extern struct mphostent* __stdcall dllgethostbyname(const char* name);
  }

  #define gethostbyname(name) dllgethostbyname(name)
  #define hostent mphostent
  #define h_addr  h_addr_list[0]
#endif

#include "URL.h"
#include "utils/log.h"

#define RTMP_SIG_SIZE 1536
#define RTMP_LARGE_HEADER_SIZE 12

#define RTMP_BUFFER_CACHE_SIZE (512*1024)

using namespace RTMP_LIB;

static const int packetSize[] = { 12, 8, 4, 1 };
#define RTMP_PACKET_SIZE_LARGE    0
#define RTMP_PACKET_SIZE_MEDIUM   1
#define RTMP_PACKET_SIZE_SMALL    2
#define RTMP_PACKET_SIZE_MINIMUM  3

CRTMP::CRTMP() : m_socket(INVALID_SOCKET)
{
  Close();
  m_strPlayer = "http://www.boxee.tv/player.swf";
  m_pBuffer = new char[RTMP_BUFFER_CACHE_SIZE];
}

CRTMP::~CRTMP()
{
  Close();
  delete [] m_pBuffer;
}

void CRTMP::SetPlayer(const std::string &strPlayer)
{
  m_strPlayer = strPlayer;
}

void CRTMP::SetPageUrl(const std::string &strPageUrl)
{
  m_strPageUrl = strPageUrl;
}

void CRTMP::SetPlayPath(const std::string &strPlayPath)
{
  m_strPlayPath = strPlayPath;
}

bool CRTMP::Connect(const std::string &strRTMPLink)
{
  Close();

  CURL url(strRTMPLink.c_str());

  sockaddr_in service;
  memset(&service, 0, sizeof(sockaddr_in));
  service.sin_family = AF_INET;
  service.sin_addr.s_addr = inet_addr(url.GetHostName().c_str());
  if (service.sin_addr.s_addr == INADDR_NONE)
  {
    CStdString strIpAddress = "";
    struct hostent *host = gethostbyname(url.GetHostName().c_str());
    if (host == NULL || host->h_addr == NULL)
    {
      CLog::Log(LOGWARNING, "ERROR: Problem accessing the DNS. (addr: %s)", url.GetHostName().c_str());
      return false;
    }
    service.sin_addr = *(struct in_addr*)host->h_addr;
  }

  if (url.GetPort() == 0)
    url.SetPort(1935);

  m_strLink = strRTMPLink;

  service.sin_port = htons(url.GetPort());
  m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_socket > 0)
  {
    if (connect(m_socket, (sockaddr*) &service, sizeof(struct sockaddr)) < 0)
    {
      CLog::Log(LOGERROR,"%s, failed to connect. file: %s", __FUNCTION__, strRTMPLink.c_str());
      Close();
      return false;
    }

    if (!HandShake())
    {
      CLog::Log(LOGERROR,"%s, handshake failed. file: %s", __FUNCTION__, strRTMPLink.c_str());
      Close();
      return false;
    }

    if (!Connect())
    {
      CLog::Log(LOGERROR,"%s, connect failed. file: %s", __FUNCTION__, strRTMPLink.c_str());
      Close();
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR,"%s, failed to create socket. file: %s", __FUNCTION__, strRTMPLink.c_str());
    return false;
  }

  return true;
}

bool CRTMP::GetNextMediaPacket(RTMPPacket &packet)
{
  bool bHasMediaPacket = false;
  while (!bHasMediaPacket && IsConnected() && ReadPacket(packet))
  {
    if (!packet.IsReady())
    {
      packet.FreePacket();
      Sleep(30);
      continue;
    }

    switch (packet.m_packetType)
    {
      case 0x01:
        // chunk size
        HandleChangeChunkSize(packet);
        break;

      case 0x03:
        // bytes read report
        break;

      case 0x04:
        // ping
        HandlePing(packet);
        break;

      case 0x05:
        // server bw
        break;

      case 0x06:
        // client bw
        break;

      case 0x08:
        // audio data
        HandleAudio(packet);
        bHasMediaPacket = true;
        break;

      case 0x09:
        // video data
        HandleVideo(packet);
        bHasMediaPacket = true;
        break;

      case 0x12:
        // metadata
        HandleMetadata(packet);
        bHasMediaPacket = true;
        break;

      case 0x14:
        // invoke
        HandleInvoke(packet);
        break;

      default:
        CLog::Log(LOGDEBUG,"unknown packet type received: 0x%02x", packet.m_packetType);
    }

    if (!bHasMediaPacket)
      packet.FreePacket();    
  }
        
  if (bHasMediaPacket)
    m_bPlaying = true;

  return bHasMediaPacket;
}

int CRTMP::ReadN(char *buffer, int n)
{
  int nOriginalSize = n;
  memset(buffer, 0, n);
  char *ptr = buffer;
  while (n > 0)
  {
    int nBytes = 0;
    if (m_bPlaying)
    {
      if (m_nBufferSize < n)
        FillBuffer();

      int nRead = ((n<m_nBufferSize)?n:m_nBufferSize);
      if (nRead > 0)
      {
        memcpy(buffer, m_pBuffer, nRead);
        memmove(m_pBuffer, m_pBuffer + nRead, m_nBufferSize - nRead); // crunch buffer
        m_nBufferSize -= nRead;
        nBytes = nRead;
        m_nBytesIn += nRead;
        if (m_nBytesIn > m_nBytesInSent + (600*1024) ) // report every 600K
          SendBytesReceived();
      }
    }
    else
      nBytes = recv(m_socket, ptr, n, 0);

    if (nBytes < 0)
    {
      CLog::Log(LOGERROR, "%s, RTMP recv error %d", __FUNCTION__, GetLastError());
      Close();
      return false;
    }
    
    if (nBytes == 0)
    {
      CLog::Log(LOGDEBUG,"%s, RTMP socket closed by server", __FUNCTION__);
      Close();
      break;
    }
    
    n -= nBytes;
    ptr += nBytes;
  }

  return nOriginalSize - n;
}

bool CRTMP::WriteN(const char *buffer, int n)
{
  const char *ptr = buffer;
  while (n > 0)
  {
    int nBytes = send(m_socket, ptr, n, 0);
    if (nBytes < 0)
    {
      CLog::Log(LOGERROR, "%s, RTMP send error %d (%d bytes)", __FUNCTION__, GetLastError(), n);
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

bool CRTMP::Connect()
{
  if (!SendConnectPacket())
  {
    CLog::Log(LOGERROR, "%s, failed to send connect RTMP packet", __FUNCTION__);
    return false;
  }

  return true;
}

bool CRTMP::SendConnectPacket()
{
  CURL url(m_strLink);

  RTMPPacket packet;
  packet.m_nChannel = 0x03;   // control channel (invoke)
  packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = 0x14; // INVOKE
  packet.AllocPacket(4096);

  char *enc = packet.m_body;
  enc += EncodeString(enc, "connect");
  enc += EncodeNumber(enc, 1.0);
  *enc = 0x03; //Object Datatype
  enc++;
  enc += EncodeString(enc, "app", url.GetFileName());
  enc += EncodeString(enc, "flashVer", "LNX 9,0,115,0");
  enc += EncodeString(enc, "swfUrl", m_strPlayer.c_str());
  enc += EncodeString(enc, "tcUrl", m_strLink.c_str());
  enc += EncodeBoolean(enc, "fpad", false);
  enc += EncodeNumber(enc, "capabilities", 15.0);
  enc += EncodeNumber(enc, "audioCodecs", 1639.0);
  enc += EncodeNumber(enc, "videoCodecs", 252.0);
  enc += EncodeNumber(enc, "videoFunction", 1.0);
  enc += EncodeString(enc, "pageUrl", m_strPageUrl.c_str());  
  enc += 2; // end of object - 0x00 0x00 0x09
  *enc = 0x09;
  enc++;
  packet.m_nBodySize = enc-packet.m_body;

  return SendRTMP(packet);
}

bool CRTMP::SendCreateStream(double dStreamId)
{
  RTMPPacket packet;
  packet.m_nChannel = 0x03;   // control channel (invoke)
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = 0x14; // INVOKE

  packet.AllocPacket(256); // should be enough
  char *enc = packet.m_body;
  enc += EncodeString(enc, "createStream");
  enc += EncodeNumber(enc, dStreamId);
  *enc = 0x05; // NULL
  enc++;

  packet.m_nBodySize = enc - packet.m_body;

  return SendRTMP(packet);
}

bool CRTMP::SendPause()
{
  RTMPPacket packet;
  packet.m_nChannel = 0x08;   // video channel 
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = 0x14; // invoke

  packet.AllocPacket(256); // should be enough
  char *enc = packet.m_body;
  enc += EncodeString(enc, "pause");
  enc += EncodeNumber(enc, 0);
  *enc = 0x05; // NULL
  enc++;
  enc += EncodeBoolean(enc, true);
  enc += EncodeNumber(enc, 0);

  packet.m_nBodySize = enc - packet.m_body;

  return SendRTMP(packet);
}

bool CRTMP::SendSeek(double dTime)
{
  RTMPPacket packet;
  packet.m_nChannel = 0x08;   // video channel 
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = 0x14; // invoke

  packet.AllocPacket(256); // should be enough
  char *enc = packet.m_body;
  enc += EncodeString(enc, "seek");
  enc += EncodeNumber(enc, 0);
  *enc = 0x05; // NULL
  enc++;
  enc += EncodeNumber(enc, dTime);

  packet.m_nBodySize = enc - packet.m_body;

  return SendRTMP(packet);
}

bool CRTMP::SendServerBW()
{
  RTMPPacket packet;
  packet.m_nChannel = 0x02;   // control channel (invoke)
  packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = 0x05; // Server BW

  packet.AllocPacket(4);
  packet.m_nBodySize = 4;

  EncodeInt32(packet.m_body, 0x001312d0); // hard coded for now
  return SendRTMP(packet);
}

bool CRTMP::SendBytesReceived()
{
  RTMPPacket packet;
  packet.m_nChannel = 0x02;   // control channel (invoke)
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = 0x03; // bytes in

  packet.AllocPacket(4);
  packet.m_nBodySize = 4;

  EncodeInt32(packet.m_body, m_nBytesIn); // hard coded for now
  m_nBytesInSent = m_nBytesIn;

  CLog::Log(LOGDEBUG,"Send bytes report. 0x%x (%d bytes)", (unsigned int)m_nBytesIn, m_nBytesIn);
  return SendRTMP(packet);
}

bool CRTMP::SendCheckBW()
{
  RTMPPacket packet;
  packet.m_nChannel = 0x03;   // control channel (invoke)
  packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = 0x14; // INVOKE
  packet.m_nInfoField1 = timeGetTime();

  packet.AllocPacket(256); // should be enough
  char *enc = packet.m_body;
  enc += EncodeString(enc, "_checkbw");
  enc += EncodeNumber(enc, 0x00);
  *enc = 0x05; // NULL
  enc++;

  packet.m_nBodySize = enc - packet.m_body;

  return SendRTMP(packet);
}

bool CRTMP::SendCheckBWResult()
{
  RTMPPacket packet;
  packet.m_nChannel = 0x03;   // control channel (invoke)
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = 0x14; // INVOKE
  packet.m_nInfoField1 = 0x16 * m_nBWCheckCounter; // temp inc value. till we figure it out.

  packet.AllocPacket(256); // should be enough
  char *enc = packet.m_body;
  enc += EncodeString(enc, "_result");
  enc += EncodeNumber(enc, (double)time(NULL)); // temp
  *enc = 0x05; // NULL
  enc++;
  enc += EncodeNumber(enc, (double)m_nBWCheckCounter++); 

  packet.m_nBodySize = enc - packet.m_body;

  return SendRTMP(packet);
}

bool CRTMP::SendPlay()
{
  CURL url(m_strLink);
  RTMPPacket packet;
  packet.m_nChannel = 0x08;   // we make 8 our stream channel
  packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = 0x14; // INVOKE
  packet.m_nInfoField2 = 0x01000000;

  packet.AllocPacket(256); // should be enough
  char *enc = packet.m_body;
  enc += EncodeString(enc, "play");
  enc += EncodeNumber(enc, 0.0);
  *enc = 0x05; // NULL
  enc++;
  CStdString strPlay = m_strPlayPath;
  if (strPlay.IsEmpty())
  {
    int nPos = url.GetFileName().Find("slist=");
    if (nPos > 0)
      strPlay = url.GetFileName().Mid(nPos + 6);
    if (strPlay.IsEmpty())
      return false;
  }

  enc += EncodeString(enc, strPlay.c_str());
  enc += EncodeNumber(enc, 0.0);

  packet.m_nBodySize = enc - packet.m_body;

  return SendRTMP(packet);
}

bool CRTMP::SendPing(short nType, unsigned int nObject, unsigned int nTime)
{
  CLog::Log(LOGDEBUG,"sending ping. type: 0x%04x", (unsigned short)nType);

  RTMPPacket packet; 
  packet.m_nChannel = 0x02;   // control channel (ping)
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = 0x04; // ping
  packet.m_nInfoField1 = timeGetTime();

  int nSize = (nType==0x03?10:6); // type 3 is the buffer time and requires all 3 parameters. all in all 10 bytes.
  packet.AllocPacket(nSize);
  packet.m_nBodySize = nSize;

  char *buf = packet.m_body;
  buf += EncodeInt16(buf, nType);

  if (nSize > 2)
    buf += EncodeInt32(buf, nObject);

  if (nSize > 6)
    buf += EncodeInt32(buf, nTime);

  return SendRTMP(packet);
}

void CRTMP::HandleInvoke(const RTMPPacket &packet)
{
  if (packet.m_body[0] != 0x02) // make sure it is a string method name we start with
  {
    CLog::Log(LOGWARNING,"%s, Sanity failed. no string method in invoke packet", __FUNCTION__);
    return;
  }

  RTMP_LIB::AMFObject obj;
  int nRes = obj.Decode(packet.m_body, packet.m_nBodySize);
  if (nRes < 0)
  { 
    CLog::Log(LOGERROR,"%s, error decoding invoke packet", __FUNCTION__);
    return;
  }

  obj.Dump();

  CStdString method = obj.GetProperty(0).GetString();

  if (method == "_result")
  {
    CStdString methodInvoked = m_methodCalls[0];
    m_methodCalls.erase(m_methodCalls.begin());

    CLog::Log(LOGDEBUG,"%s, received result for method call <%s>", __FUNCTION__, methodInvoked.c_str());
  
    if (methodInvoked == "connect")
    {
      SendServerBW();
      SendPing(3, 0, 300);
      SendCreateStream(2.0);
    }
    else if (methodInvoked == "createStream")
    {
      SendPlay();
      SendPing(3, 1, 300);
    }
    else if (methodInvoked == "play")
    {
    }
  }
  else if (method == "onBWDone")
  {
    //SendCheckBW();
  }
  else if (method == "_onbwcheck")
  {
    SendCheckBWResult();
  }
  else if (method == "_error")
  {
    CLog::Log(LOGERROR,"rtmp server sent error");
  }
  else if (method == "close")
  {
    CLog::Log(LOGERROR,"rtmp server requested close");
    Close();
  }
  else if (method == "onstatus")
  {
  }
  else
  {
    CLog::Log(LOGDEBUG,"%s, server invoking <%s>", __FUNCTION__, method.c_str());
  }
}

void CRTMP::HandleMetadata(const RTMPPacket &packet)
{
}

void CRTMP::HandleChangeChunkSize(const RTMPPacket &packet)
{
  if (packet.m_nBodySize >= 4)
  {
    m_chunkSize = ReadInt32(packet.m_body);
    CLog::Log(LOGDEBUG,"Changed chunk size to %d", m_chunkSize);
  }
}

void CRTMP::HandleAudio(const RTMPPacket &packet)
{
}

void CRTMP::HandleVideo(const RTMPPacket &packet)
{
}

void CRTMP::HandlePing(const RTMPPacket &packet)
{
  short nType = -1;
  if (packet.m_body && packet.m_nBodySize >= sizeof(short))
    nType = ReadInt16(packet.m_body);
  CLog::Log(LOGDEBUG,"server sent ping. type: %d", nType);

  if (nType == 0x06 && packet.m_nBodySize >= 6) // server ping. reply with pong.
  {
    unsigned int nTime = ReadInt32(packet.m_body + sizeof(short));
    SendPing(0x07, nTime);
  }
}

bool CRTMP::ReadPacket(RTMPPacket &packet)
{
  char type;
  if (ReadN(&type,1) != 1)
  {
    CLog::Log(LOGERROR, "%s, failed to read RTMP packet header", __FUNCTION__);
    return false;
  } 

  packet.m_headerType = (type & 0xc0) >> 6;
  packet.m_nChannel = (type & 0x3f);

  int nSize = packetSize[packet.m_headerType];
  if (nSize < RTMP_LARGE_HEADER_SIZE) // using values from the last message of this channel
    packet = m_vecChannelsIn[packet.m_nChannel];
  
  nSize--;

  char header[RTMP_LARGE_HEADER_SIZE] = {0};
  if (nSize > 0 && ReadN(header,nSize) != nSize)
  {
    CLog::Log(LOGERROR, "%s, failed to read RTMP packet header. type: %x", __FUNCTION__, (unsigned int)type);
    return false;
  }

  if (nSize >= 3)
    packet.m_nInfoField1 = ReadInt24(header);

  if (nSize >= 6)
  {
    packet.m_nBodySize = ReadInt24(header + 3);
    packet.m_nBytesRead = 0;
    packet.FreePacketHeader(); // new packet body
  }
  
  if (nSize > 6)
    packet.m_packetType = header[6];

  if (nSize == 11)
    packet.m_nInfoField2 = ReadInt32(header+7);

  if (packet.m_nBodySize > 0 && packet.m_body == NULL && !packet.AllocPacket(packet.m_nBodySize))
  {
    CLog::Log(LOGDEBUG,"%s, failed to allocate packet", __FUNCTION__);
    return false;
  }

  int nToRead = packet.m_nBodySize - packet.m_nBytesRead;
  int nChunk = m_chunkSize;
  if (nToRead < nChunk)
     nChunk = nToRead;

  if (ReadN(packet.m_body + packet.m_nBytesRead, nChunk) != nChunk)
  {
    CLog::Log(LOGERROR, "%s, failed to read RTMP packet body. len: %lu", __FUNCTION__, packet.m_nBodySize);
    packet.m_body = NULL; // we dont want it deleted since its pointed to from the stored packets (m_vecChannelsIn)
    return false;  
  }

  packet.m_nBytesRead += nChunk;

  // keep the packet as ref for other packets on this channel
  m_vecChannelsIn[packet.m_nChannel] = packet;

  if (packet.IsReady())
  {
    // reset the data from the stored packet. we keep the header since we may use it later if a new packet for this channel
    // arrives and requests to re-use some info (small packet header)
    m_vecChannelsIn[packet.m_nChannel].m_body = NULL;
    m_vecChannelsIn[packet.m_nChannel].m_nBytesRead = 0;
  }
  else
    packet.m_body = NULL; // so it wont be erased on "free"

  return true;
}

short  CRTMP::ReadInt16(const char *data)
{
  short val;
  memcpy(&val,data,sizeof(short));
  return ntohs(val);
}

int  CRTMP::ReadInt24(const char *data)
{
  char tmp[4] = {0};
  memcpy(tmp+1, data, 3);
  int val;
  memcpy(&val, tmp, sizeof(int));
  return ntohl(val);
}

int  CRTMP::ReadInt32(const char *data)
{
  int val;
  memcpy(&val, data, sizeof(int));
  return ntohl(val);
}

std::string CRTMP::ReadString(const char *data)
{
  std::string strRes;
  short len = ReadInt16(data);
  if (len > 0)
  {
    char *pStr = new char[len+1]; 
    memset(pStr, 0, len+1);
    memcpy(pStr, data + sizeof(short), len);
    strRes = pStr;
    delete [] pStr;
  }
  return strRes;
}

bool CRTMP::ReadBool(const char *data)
{
  return *data == 0x01;
}

double CRTMP::ReadNumber(const char *data)
{
  double val;
  char *dPtr = (char *)&val;
  for (int i=7;i>=0;i--)
  {
    *dPtr = data[i];
    dPtr++;
  }

  return val;
}

int CRTMP::EncodeString(char *output, const std::string &strName, const std::string &strValue)
{
  char *buf = output;
  short length = htons(strName.size());
  memcpy(buf, &length, 2);
  buf += 2;

  memcpy(buf, strName.c_str(), strName.size());
  buf += strName.size();
  
  buf += EncodeString(buf, strValue);
  return buf - output;
}

int CRTMP::EncodeInt16(char *output, short nVal)
{
  nVal = htons(nVal);
  memcpy(output, &nVal, sizeof(short));
  return sizeof(short);
}

int CRTMP::EncodeInt24(char *output, int nVal)
{
  nVal = htonl(nVal);
  char *ptr = (char *)&nVal;
  ptr++;
  memcpy(output, ptr, 3);
  return 3;
}

int CRTMP::EncodeInt32(char *output, int nVal)
{
  nVal = htonl(nVal);
  memcpy(output, &nVal, sizeof(int));
  return sizeof(int);
}

int CRTMP::EncodeNumber(char *output, const std::string &strName, double dVal)
{
  char *buf = output;

  unsigned short length = htons(strName.size());
  memcpy(buf, &length, 2);
  buf += 2;

  memcpy(buf, strName.c_str(), strName.size());
  buf += strName.size();

  buf += EncodeNumber(buf, dVal);
  return buf - output;
}

int CRTMP::EncodeBoolean(char *output, const std::string &strName, bool bVal)
{
  char *buf = output;
  unsigned short length = htons(strName.size());
  memcpy(buf, &length, 2);
  buf += 2;

  memcpy(buf, strName.c_str(), strName.size());
  buf += strName.size();

  buf += EncodeBoolean(buf, bVal);

  return buf - output;
}

int CRTMP::EncodeString(char *output, const std::string &strValue)
{
  char *buf = output;
  *buf = 0x02; // Datatype: String
  buf++;

  short length = htons(strValue.size());
  memcpy(buf, &length, 2);
  buf += 2;

  memcpy(buf, strValue.c_str(), strValue.size());
  buf += strValue.size();

  return buf - output;
}

int CRTMP::EncodeNumber(char *output, double dVal)
{
  char *buf = output;  
  *buf = 0x00; // type: Number
  buf++;

  char *dPtr = (char *)&dVal;
  for (int i=7;i>=0;i--)
  {
    buf[i] = *dPtr;
    dPtr++;
  }

  buf += 8;

  return buf - output;
}

int CRTMP::EncodeBoolean(char *output, bool bVal)
{
  char *buf = output;  

  *buf = 0x01; // type: Boolean
  buf++;

  *buf = bVal?0x01:0x00; 
  buf++;

  return buf - output;
}

bool CRTMP::HandShake()
{
  char clientsig[RTMP_SIG_SIZE+1];
  char serversig[RTMP_SIG_SIZE];

  clientsig[0] = 0x3;
  DWORD uptime = htonl(timeGetTime());
  memcpy(clientsig + 1, &uptime, sizeof(DWORD));
  memset(clientsig + 5, 0, 4);

  for (int i=9; i<=RTMP_SIG_SIZE; i++)
    clientsig[i] = (char)(rand() % 256);

  if (!WriteN(clientsig, RTMP_SIG_SIZE + 1))
    return false;

  char dummy;
  if (ReadN(&dummy, 1) != 1) // 0x03
    return false;

  
  if (ReadN(serversig, RTMP_SIG_SIZE) != RTMP_SIG_SIZE)
    return false;

  char resp[RTMP_SIG_SIZE];
  if (ReadN(resp, RTMP_SIG_SIZE) != RTMP_SIG_SIZE)
    return false;

  bool bMatch = (memcmp(resp, clientsig + 1, RTMP_SIG_SIZE) == 0);
  if (!bMatch)
  {
    CLog::Log(LOGWARNING,"%s, client signiture does not match!",__FUNCTION__);
  }

  if (!WriteN(serversig, RTMP_SIG_SIZE))
    return false;

  return true;
}

bool CRTMP::SendRTMP(RTMPPacket &packet)
{
  const RTMPPacket &prevPacket = m_vecChannelsOut[packet.m_nChannel];
  if (packet.m_headerType != RTMP_PACKET_SIZE_LARGE)
  {
    // compress a bit by using the prev packet's attributes
    if (prevPacket.m_nBodySize == packet.m_nBodySize && packet.m_headerType == RTMP_PACKET_SIZE_MEDIUM) 
      packet.m_headerType = RTMP_PACKET_SIZE_SMALL;

    if (prevPacket.m_nInfoField2 == packet.m_nInfoField2 && packet.m_headerType == RTMP_PACKET_SIZE_SMALL)
      packet.m_headerType = RTMP_PACKET_SIZE_MINIMUM;
      
  }

  if (packet.m_headerType > 3) // sanity
  { 
    CLog::Log(LOGERROR,"sanity failed!! tring to send header of type: 0x%02x.", (unsigned char)packet.m_headerType);
    return false;
  }

  int nSize = packetSize[packet.m_headerType];
  char header[RTMP_LARGE_HEADER_SIZE] = { 0 };
  header[0] = (char)((packet.m_headerType << 6) | packet.m_nChannel);
  if (nSize > 1)
    EncodeInt24(header+1, packet.m_nInfoField1);
  
  if (nSize > 4)
  {
    EncodeInt24(header+4, packet.m_nBodySize);
    header[7] = packet.m_packetType;
  }

  if (nSize > 8)
    EncodeInt32(header+8, packet.m_nInfoField2);

  if (!WriteN(header, nSize))
  {
    CLog::Log(LOGWARNING,"couldnt send rtmp header");
    return false;
  }

  nSize = packet.m_nBodySize;
  char *buffer = packet.m_body;

  while (nSize)
  {
    int nChunkSize = packet.m_packetType == 0x14?m_chunkSize:packet.m_nBodySize;
    if (nSize < m_chunkSize)
      nChunkSize = nSize;

    if (!WriteN(buffer, nChunkSize))
      return false;

    nSize -= nChunkSize;
    buffer += nChunkSize;

    if (nSize > 0)
    {
      char sep = (0xc0 | packet.m_nChannel);
      if (!WriteN(&sep, 1))
        return false;  
    }
  }

  if (packet.m_packetType == 0x14) // we invoked a remote method, keep it in call queue till result arrives
    m_methodCalls.push_back(ReadString(packet.m_body + 1));

  m_vecChannelsOut[packet.m_nChannel] = packet;
  m_vecChannelsOut[packet.m_nChannel].m_body = NULL;
  return true;
}

void CRTMP::Close()
{
  if (IsConnected())
    closesocket(m_socket);

  m_socket = INVALID_SOCKET;
  m_chunkSize = 128;
  m_nBWCheckCounter = 0;
  m_nBytesIn = 0;
  m_nBytesInSent = 0;

  for (int i=0; i<64; i++)
  {
    m_vecChannelsIn[i].Reset();
    m_vecChannelsIn[i].m_nChannel = i;
    m_vecChannelsOut[i].Reset();
    m_vecChannelsOut[i].m_nChannel = i;
  }

  m_bPlaying = false;
  m_nBufferSize = 0;
}

bool CRTMP::FillBuffer()
{
  time_t now = time(NULL);
  while (m_nBufferSize < RTMP_BUFFER_CACHE_SIZE && time(NULL) - now < 4)
  {
    int nBytes = recv(m_socket, m_pBuffer + m_nBufferSize, RTMP_BUFFER_CACHE_SIZE - m_nBufferSize, 0);
    if (nBytes > 0)
      m_nBufferSize += nBytes;
    else
    {
      CLog::Log(LOGDEBUG,"%s, read buffer returned %d. errno: %d", __FUNCTION__, nBytes, GetLastError());
      break;
    }
  }

  return true;
}
