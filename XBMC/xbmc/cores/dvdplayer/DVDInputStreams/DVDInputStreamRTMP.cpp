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
#include "FileItem.h"
#include "DVDInputStreamRTMP.h"
#include "VideoInfoTag.h"
#include "FileSystem/IFile.h"
#include "utils/SingleLock.h"
#include "utils/log.h"

#ifdef _LINUX
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <netinet/in.h>
#endif

#include "Application.h"

using namespace XFILE;

CDVDInputStreamRTMP::CDVDInputStreamRTMP() : CDVDInputStream(DVDSTREAM_TYPE_RTMP)
{
  m_rtmp = NULL;
  m_rtmp = new RTMP_LIB::CRTMP;
  m_eof = true;
  m_bPaused = false;
  m_prevTagSize = 0;
  m_bSentHeader = false;
  m_leftOver = NULL;
  m_leftOverSize = 0;
  m_leftOverConsumed = 0;
  m_sStreamPlaying = NULL;
}

CDVDInputStreamRTMP::~CDVDInputStreamRTMP()
{
  if (m_sStreamPlaying)
  {
    free(m_sStreamPlaying);
    m_sStreamPlaying = NULL;
  }

  Close();

  CSingleLock lock(m_RTMPSection);
  if (m_rtmp)
  {
    CLog::Log(LOGNOTICE,"Deleted CRTMP");
    delete m_rtmp;
    m_rtmp = NULL;
  }

  m_bPaused = false;
}

bool CDVDInputStreamRTMP::IsEOF()
{
  return m_eof;
}

bool CDVDInputStreamRTMP::Open(const char* strFile, const std::string& content)
{
  if (m_sStreamPlaying)
  {
    free(m_sStreamPlaying);
    m_sStreamPlaying = NULL;
  }

  if (!CDVDInputStream::Open(strFile, "video/x-flv")) return false;

  CSingleLock lock(m_RTMPSection);
  if (!m_rtmp) return false;

  m_rtmp->SetPlayer(m_item.GetProperty("SWFPlayer"));
  m_rtmp->SetPageUrl(m_item.GetProperty("PageURL"));
  m_rtmp->SetPlayPath(m_item.GetProperty("PlayPath"));
  m_rtmp->SetTcUrl(m_item.GetProperty("TcUrl"));
  if (!m_item.GetProperty("IsLive").compare("true"))
    m_rtmp->SetLive();
  m_rtmp->SetBufferMS(20000);

  if (!m_rtmp->Connect(strFile))
    return false;

  m_sStreamPlaying = (char*)calloc(strlen(strFile)+1,sizeof(char));
  strncpy(m_sStreamPlaying,strFile,strlen(strFile));
  m_eof = false;
  return true;
}

// close file and reset everyting
void CDVDInputStreamRTMP::Close()
{
  CSingleLock lock(m_RTMPSection);
  CDVDInputStream::Close();
  if (m_rtmp) m_rtmp->Close();
  m_eof = true;
}

int CDVDInputStreamRTMP::Read(BYTE* buf, int buf_size)
{
  int nRead = 0;
  if (m_leftOver)
  {
    int nToConsume = m_leftOverSize - m_leftOverConsumed;
    if (nToConsume > buf_size)
      nToConsume = buf_size;

    memcpy(buf, m_leftOver + m_leftOverConsumed, nToConsume);
    buf_size -= nToConsume;
    m_leftOverConsumed += nToConsume;
    nRead += nToConsume;

    if (m_leftOverConsumed == m_leftOverSize)
    {
      m_leftOverConsumed = m_leftOverSize = 0;
      delete [] m_leftOver;
      m_leftOver = NULL;
    }

    if (buf_size == 0)
      return nRead;
  }

  RTMP_LIB::RTMPPacket packet;

  CSingleLock lock(m_RTMPSection);
  while (buf_size > nRead && m_rtmp && m_rtmp->GetNextMediaPacket(packet))
  {

    if (!m_bSentHeader)
    {
      if (buf_size < 9)
        return -1;

      char header[] = { 'F', 'L', 'V', 0x01,
                      0x05, // video + audio
                      0x00, 0x00, 0x00, 0x09};
      memcpy(buf, header, 9);
      buf_size -= 9;
      nRead += 9;
      m_bSentHeader = true;
    }

    if (buf_size < 4)
      return nRead;

    // skip video info/command packets
    // if we keep these it chokes the dvdplayer
    if ( packet.m_packetType == 0x09 &&
         packet.m_nBodySize == 2 &&
         ( (*packet.m_body & 0xf0) == 0x50) )
    {
      continue;
    }

    // write footer of previous FLV tag
    if ( m_prevTagSize != -1 )
    {
      RTMP_LIB::CRTMP::EncodeInt32((char*)buf + nRead, m_prevTagSize);
      nRead += 4;
      buf_size -= 4;
    }

    char *ptr = (char*)buf + nRead;

    // audio (0x08), video (0x09) or metadata (0x12) packets :
    // construct 11 byte header then add rtmp packet's data
    if ( packet.m_packetType == 0x08 || packet.m_packetType == 0x09 || packet.m_packetType == 0x12 )
    {
      m_prevTagSize = 11 + packet.m_nBodySize;

      if (buf_size < 11)
        return nRead;

      *ptr = packet.m_packetType;
      ptr++;
      ptr += RTMP_LIB::CRTMP::EncodeInt24(ptr, packet.m_nBodySize);

      ptr += RTMP_LIB::CRTMP::EncodeInt24(ptr, packet.m_nInfoField1);
      *ptr = (char)((packet.m_nInfoField1 & 0xFF000000) >> 24);
      ptr++;

      ptr += RTMP_LIB::CRTMP::EncodeInt24(ptr, 0);

      nRead += 11;
      buf_size -= 11;

      if (buf_size == 0)
        return nRead;

    }
    else if (packet.m_packetType == 0x16)
    {
      // FLV tag(s) packet
      // contains it's own tagsize footer, don't write another
      m_prevTagSize = -1;
    }

    int nBodyLen = packet.m_nBodySize;
    if (nBodyLen > buf_size)
    {
      memcpy(ptr, packet.m_body, buf_size);
      nRead += buf_size;

      m_leftOver = new char[packet.m_nBodySize - buf_size];
      memcpy(m_leftOver, packet.m_body + buf_size, packet.m_nBodySize - buf_size);
      m_leftOverSize =  packet.m_nBodySize - buf_size;
      m_leftOverConsumed = 0;
      break;
    }
    else
    {
      memcpy(ptr, packet.m_body, nBodyLen);
      nRead += nBodyLen;
      buf_size -= nBodyLen;
    }

  }


  if (m_rtmp && m_rtmp->IsConnected())
    return nRead;

  return -1;
}

__int64 CDVDInputStreamRTMP::Seek(__int64 offset, int whence)
{
  if(whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1;
}

bool CDVDInputStreamRTMP::SeekTime(int iTimeInMsec)
{
  CLog::Log(LOGNOTICE, "RTMP Seek to %i requested", iTimeInMsec);
  if (m_rtmp->Seek((double)iTimeInMsec))
  {
    return true;
  }
  else
    return false;
}

__int64 CDVDInputStreamRTMP::GetLength()
{
  return -1;
}

bool CDVDInputStreamRTMP::NextStream()
{
  return false;
}

bool CDVDInputStreamRTMP::Pause(double dTime)
{
  if (m_bPaused)
  {
    CSingleLock lock(m_RTMPSection);
    /*m_rtmp = new RTMP_LIB::CRTMP;
    m_rtmp->Connect(m_sStreamPlaying, dTime);
    m_bPaused = false;
    CLog::Log(LOGNOTICE,"Created new CRTMP - %s : %f ms", m_sStreamPlaying, dTime);*/
    m_bPaused = false;
    m_rtmp->SendPause(m_bPaused, dTime);
  }
  else
  {
    CSingleLock lock(m_RTMPSection);
    /*Close();
    delete m_rtmp;
    m_bPaused = true;
    CLog::Log(LOGNOTICE,"Deleted CRTMP");*/
    m_bPaused = true;
    m_rtmp->SendPause(m_bPaused, dTime);
  }

  return true;
}

