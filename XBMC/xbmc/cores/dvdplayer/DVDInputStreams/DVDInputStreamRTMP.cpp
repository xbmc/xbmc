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

#ifdef _LINUX
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <netinet/in.h>
#endif

#include "utils/HTTP.h"
#include "Application.h"

using namespace XFILE;

CDVDInputStreamRTMP::CDVDInputStreamRTMP() : CDVDInputStream(DVDSTREAM_TYPE_RTMP)
{
  m_eof = true;
  m_videoTS = 0;
  m_audioTS = 0;
  m_prevTagSize = 0;
  m_bSentHeader = false;
  m_leftOver = NULL;
  m_leftOverSize = 0;
  m_leftOverConsumed = 0;
}

CDVDInputStreamRTMP::~CDVDInputStreamRTMP()
{
  Close();
}

bool CDVDInputStreamRTMP::IsEOF()
{
  return m_eof;
}

bool CDVDInputStreamRTMP::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) return false;

  m_rtmp.SetPlayer(m_item.GetProperty("SWFPlayer")); 
  m_rtmp.SetPageUrl(m_item.GetProperty("PageURL")); 
  m_rtmp.SetPlayPath(m_item.GetProperty("PlayPath")); 
  m_rtmp.SetBufferMS(20000);

  if (!m_rtmp.Connect(strFile))
    return false;

  m_eof = false;
  return true;
}

// close file and reset everyting
void CDVDInputStreamRTMP::Close()
{
  CDVDInputStream::Close();
  m_rtmp.Close();
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
  while (buf_size > nRead && m_rtmp.GetNextMediaPacket(packet))
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
      
      unsigned int nTimeStamp = 0;
      if (packet.m_packetType == 0x08){ // audio
        nTimeStamp = m_audioTS;
        m_audioTS += packet.m_nInfoField1;
      }
      else if (packet.m_packetType == 0x09){ // video
        nTimeStamp = m_videoTS;
        m_videoTS += packet.m_nInfoField1;
      }
      
      ptr += RTMP_LIB::CRTMP::EncodeInt24(ptr, nTimeStamp);

      *ptr = (char)((nTimeStamp & 0xFF000000) >> 24);
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


  if (m_rtmp.IsConnected())
    return nRead;

  return -1;  
}

__int64 CDVDInputStreamRTMP::Seek(__int64 offset, int whence)
{
  int ret = 0;
  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

__int64 CDVDInputStreamRTMP::GetLength()
{
  return 0;
}

bool CDVDInputStreamRTMP::NextStream()
{
  return false;
}

