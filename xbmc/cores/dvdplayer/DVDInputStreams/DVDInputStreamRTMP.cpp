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

#include "AdvancedSettings.h"
#include <string>

using namespace XFILE;
using namespace std;

#ifdef HAS_EXTERNAL_LIBRTMP
static int RTMP_level=0;
extern "C" 
{
  static void CDVDInputStreamRTMP_Log(int level, const char *fmt, va_list args)
  {
    char buf[2048];

    if (level > RTMP_level)
      return;

    switch(level) 
    {
      default:
      case RTMP_LOGCRIT:    level = LOGFATAL;   break;
      case RTMP_LOGERROR:   level = LOGERROR;   break;
      case RTMP_LOGWARNING: level = LOGWARNING; break;
      case RTMP_LOGINFO:    level = LOGNOTICE;  break;
      case RTMP_LOGDEBUG:   level = LOGINFO;    break;
      case RTMP_LOGDEBUG2:  level = LOGDEBUG;   break;
    }

    vsnprintf(buf, sizeof(buf), fmt, args);
    CLog::Log(level, "%s", buf);
  }
}
#endif

CDVDInputStreamRTMP::CDVDInputStreamRTMP() : CDVDInputStream(DVDSTREAM_TYPE_RTMP)
{
#ifdef HAS_EXTERNAL_LIBRTMP
  if (m_libRTMP.Load())
  {
    CLog::Log(LOGINFO,"%s: Using external libRTMP",__FUNCTION__);
    RTMP_LogLevel level;

    m_libRTMP.LogSetCallback(CDVDInputStreamRTMP_Log);
    switch (g_advancedSettings.m_logLevel)
    {
      case LOG_LEVEL_DEBUG_SAMBA: level = RTMP_LOGDEBUG2; break;
      case LOG_LEVEL_DEBUG_FREEMEM:
      case LOG_LEVEL_DEBUG: level = RTMP_LOGDEBUG; break;
      case LOG_LEVEL_NORMAL: level = RTMP_LOGINFO; break;
      default: level = RTMP_LOGCRIT; break;
    }
    m_libRTMP.LogSetLevel(level);
    RTMP_level = level;
    
    m_libRTMP.Init(&m_rtmp);
  }
#else
  {
    CLog::Log(LOGINFO,"%s: Using internal libRTMP",__FUNCTION__);
    m_intRTMP = new RTMP_LIB::CRTMP;
    m_prevTagSize = 0;
    m_bSentHeader = false;
    m_leftOver = NULL;
    m_leftOverSize = 0;
    m_leftOverConsumed = 0;
  }
#endif
  m_eof = true;
  m_bPaused = false;
  m_sStreamPlaying = NULL;
}

CDVDInputStreamRTMP::~CDVDInputStreamRTMP()
{
  free(m_sStreamPlaying);
  m_sStreamPlaying = NULL;

  Close();

#ifndef HAS_EXTERNAL_LIBRTMP
  if (m_intRTMP)
  {
    CSingleLock lock(m_RTMPSection);
    CLog::Log(LOGNOTICE,"Deleted CRTMP");
    delete m_intRTMP;
    m_intRTMP = NULL;
  }
#endif

  m_bPaused = false;
}

bool CDVDInputStreamRTMP::IsEOF()
{
  return m_eof;
}

#ifdef HAS_EXTERNAL_LIBRTMP
#define  SetAVal(av, cstr)  av.av_val = (char *)cstr.c_str(); av.av_len = cstr.length()
#undef AVC
#define AVC(str)  {(char *)str,sizeof(str)-1}

/* librtmp option names are slightly different */
static const struct {
  const char *name;
  AVal key;
} options[] = {
  { "SWFPlayer", AVC("swfUrl") },
  { "PageURL",   AVC("pageUrl") },
  { "PlayPath",  AVC("playpath") },
  { "TcUrl",     AVC("tcUrl") },
  { "IsLive",    AVC("live") },
  { NULL }
};
#endif

bool CDVDInputStreamRTMP::Open(const char* strFile, const std::string& content)
{
  if (m_sStreamPlaying)
  {
    free(m_sStreamPlaying);
    m_sStreamPlaying = NULL;
  }

  if (!CDVDInputStream::Open(strFile, "video/x-flv"))
    return false;

  CSingleLock lock(m_RTMPSection);
  
#ifdef HAS_EXTERNAL_LIBRTMP
  {
    if (!m_libRTMP.SetupURL(&m_rtmp, (char*)strFile))
      return false;

    for (int i=0; options[i].name; i++)
    {
      string tmp = m_item.GetProperty(options[i].name);
      if (!tmp.empty())
      {
        AVal av_tmp;
        SetAVal(av_tmp, tmp);
        m_libRTMP.SetOpt(&m_rtmp, &options[i].key, &av_tmp);
      }
    }

    if (!m_libRTMP.Connect(&m_rtmp, NULL) || !m_libRTMP.ConnectStream(&m_rtmp, 0))
      return false;
  }
#else
  if (m_intRTMP)
  {
    m_intRTMP->SetPlayer(m_item.GetProperty("SWFPlayer"));
    m_intRTMP->SetPageUrl(m_item.GetProperty("PageURL"));
    m_intRTMP->SetPlayPath(m_item.GetProperty("PlayPath"));
    m_intRTMP->SetTcUrl(m_item.GetProperty("TcUrl"));
    if (m_item.GetProperty("IsLive").Equals("true"))
      m_intRTMP->SetLive();
    m_intRTMP->SetBufferMS(20000);

    if (!m_intRTMP->Connect(strFile))
      return false;

  }
#endif
  m_sStreamPlaying = (char*)calloc(strlen(strFile)+1,sizeof(char));
  strcpy(m_sStreamPlaying,strFile);
  m_eof = false;

  return true;
}

// close file and reset everything
void CDVDInputStreamRTMP::Close()
{
  CSingleLock lock(m_RTMPSection);
  CDVDInputStream::Close();
#ifdef HAS_EXTERNAL_LIBRTMP
  m_libRTMP.Close(&m_rtmp);
#else
  if (m_intRTMP)
    m_intRTMP->Close();
#endif

  m_eof = true;
  m_bPaused = false;
}

int CDVDInputStreamRTMP::Read(BYTE* buf, int buf_size)
{
#ifdef HAS_EXTERNAL_LIBRTMP
  {
    int i = m_libRTMP.Read(&m_rtmp, (char *)buf, buf_size);
    if (i < 0)
      m_eof = true;

    return i;
  }
#else
  int nRead = 0;
  if (m_intRTMP)
  {
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
    while (buf_size > nRead && m_intRTMP && m_intRTMP->GetNextMediaPacket(packet))
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

    if (m_intRTMP && m_intRTMP->IsConnected())
      return nRead;
  }
  return -1;
#endif
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
  CSingleLock lock(m_RTMPSection);
#ifdef HAS_EXTERNAL_LIBRTMP
  {
    if (m_libRTMP.SendSeek(&m_rtmp, iTimeInMsec))
      return true;
  }
#else
  if (m_intRTMP)
  {
    if (m_intRTMP->Seek((double)iTimeInMsec))
      return true;
  }
#endif

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
  CSingleLock lock(m_RTMPSection);
#ifdef HAS_EXTERNAL_LIBRTMP
  {
    if (!m_bPaused)
      m_rtmp.m_pauseStamp = m_rtmp.m_channelTimestamp[m_rtmp.m_mediaChannel];
    m_bPaused = !m_bPaused;
    m_libRTMP.SendPause(&m_rtmp, m_bPaused, m_rtmp.m_pauseStamp);
  }
#else
  if (m_intRTMP)
  {
    m_bPaused = !m_bPaused;
    m_intRTMP->SendPause(m_bPaused, dTime);
  }
#endif

  return true;
}

