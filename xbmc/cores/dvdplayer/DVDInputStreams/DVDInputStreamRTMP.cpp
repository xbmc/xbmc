/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#ifdef TARGET_WINDOWS
#include "system.h" // just for HAS_LIBRTMP
#endif

#ifdef HAS_LIBRTMP
#include "settings/AdvancedSettings.h"
#include "DVDInputStreamRTMP.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/Variant.h"

#include <string>

using namespace XFILE;
using namespace std;

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

CDVDInputStreamRTMP::CDVDInputStreamRTMP()
  : CDVDInputStream(DVDSTREAM_TYPE_RTMP)
  , m_canSeek(true)
  , m_canPause(true)
{
  if (m_libRTMP.Load())
  {
    CLog::Log(LOGINFO,"%s: Using external libRTMP",__FUNCTION__);
    RTMP_LogLevel level;

    m_libRTMP.LogSetCallback(CDVDInputStreamRTMP_Log);
    switch (g_advancedSettings.m_logLevel)
    {
      case LOG_LEVEL_DEBUG_FREEMEM:
      case LOG_LEVEL_DEBUG: level = RTMP_LOGDEBUG; break;
      case LOG_LEVEL_NORMAL: level = RTMP_LOGINFO; break;
      default: level = RTMP_LOGCRIT; break;
    }

    if (g_advancedSettings.CanLogComponent(LOGRTMP))
      level = RTMP_LOGDEBUG2;

    m_libRTMP.LogSetLevel(level);
    RTMP_level = level;
    
    m_rtmp = m_libRTMP.Alloc();
    m_libRTMP.Init(m_rtmp);
  }
  else
  {
    m_rtmp = NULL;
  }

  m_eof = true;
  m_bPaused = false;
  m_sStreamPlaying = NULL;
}

CDVDInputStreamRTMP::~CDVDInputStreamRTMP()
{
  free(m_sStreamPlaying);
  m_sStreamPlaying = NULL;

  Close();
  if (m_rtmp)
    m_libRTMP.Free(m_rtmp);
  m_rtmp = NULL;
  m_bPaused = false;
}

bool CDVDInputStreamRTMP::IsEOF()
{
  return m_eof;
}

#define  SetAVal(av, cstr)  av.av_val = (char *)cstr.c_str(); av.av_len = cstr.length()
#undef AVC
#define AVC(str)  {(char *)str,sizeof(str)-1}

/* librtmp option names are slightly different */
static const struct {
 const char *name;
 AVal key;
} options[] = {
 { "SWFPlayer", AVC("swfUrl") },
 { "PageURL", AVC("pageUrl") },
 { "PlayPath", AVC("playpath") },
 { "TcUrl", AVC("tcUrl") },
 { "IsLive", AVC("live") },
 { NULL }
};

bool CDVDInputStreamRTMP::Open(const char* strFile, const std::string& content)
{
  if (m_sStreamPlaying)
  {
    free(m_sStreamPlaying);
    m_sStreamPlaying = NULL;
  }

  if (!m_rtmp || !CDVDInputStream::Open(strFile, "video/x-flv"))
    return false;

  CSingleLock lock(m_RTMPSection);

  // libRTMP can and will alter strFile, so take a copy of it
  m_sStreamPlaying = (char*)calloc(strlen(strFile)+1,sizeof(char));
  strcpy(m_sStreamPlaying,strFile);

  if (!m_libRTMP.SetupURL(m_rtmp, m_sStreamPlaying))
    return false;

  /* Look for protocol options in the URL.
   * Options are added to the URL in space separated key=value pairs.
   * We are only interested in the "live" option to disable seeking,
   * the rest is handled by librtmp internally
   *
   * example URL suitable for use with RTMP_SetupURL():
   * "rtmp://flashserver:1935/ondemand/thefile swfUrl=http://flashserver/player.swf swfVfy=1 live=1"
   * details: https://rtmpdump.mplayerhq.hu/librtmp.3.html
   */
  std::string url = strFile;
  size_t iPosBlank = url.find(' ');
  if (iPosBlank != string::npos && (url.find("live=true") != string::npos || url.find("live=1") != string::npos))
  {
    m_canSeek = false;
    m_canPause = false;
  }
  CLog::Log(LOGDEBUG, "RTMP canseek: %s", m_canSeek ? "true" : "false");

  /* SetOpt and SetAVal copy pointers to the value. librtmp doesn't use the values until the Connect() call,
   * so value objects must stay allocated until then. To be extra safe, keep the values around until Close(),
   * in case librtmp needs them again.
   */
  m_optionvalues.clear();
  for (int i=0; options[i].name; i++)
  {
    std::string tmp = m_item.GetProperty(options[i].name).asString();
    if (!tmp.empty())
    {
      m_optionvalues.push_back(tmp);
      AVal av_tmp;
      SetAVal(av_tmp, m_optionvalues.back());
      m_libRTMP.SetOpt(m_rtmp, &options[i].key, &av_tmp);
    }
  }

  if (!m_libRTMP.Connect(m_rtmp, NULL) || !m_libRTMP.ConnectStream(m_rtmp, 0))
    return false;

  m_eof = false;

  return true;
}

// close file and reset everything
void CDVDInputStreamRTMP::Close()
{
  CSingleLock lock(m_RTMPSection);
  CDVDInputStream::Close();

  if (m_rtmp)
    m_libRTMP.Close(m_rtmp);

  m_optionvalues.clear();
  m_eof = true;
  m_bPaused = false;
}

int CDVDInputStreamRTMP::Read(uint8_t* buf, int buf_size)
{
  if (!m_rtmp)
    return -1;

  int i = m_libRTMP.Read(m_rtmp, (char *)buf, buf_size);
  if (i < 0)
    m_eof = true;

  return i;
}

int64_t CDVDInputStreamRTMP::Seek(int64_t offset, int whence)
{
  if (whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1;
}

bool CDVDInputStreamRTMP::SeekTime(int iTimeInMsec)
{
  CLog::Log(LOGNOTICE, "RTMP Seek to %i requested", iTimeInMsec);
  CSingleLock lock(m_RTMPSection);

  if (m_rtmp && m_libRTMP.SendSeek(m_rtmp, iTimeInMsec))
    return true;

  return false;
}

int64_t CDVDInputStreamRTMP::GetLength()
{
  return -1;
}

bool CDVDInputStreamRTMP::Pause(double dTime)
{
  CSingleLock lock(m_RTMPSection);

  m_bPaused = !m_bPaused;

  CLog::Log(LOGNOTICE, "RTMP Pause %s requested", m_bPaused ? "TRUE" : "FALSE");

  if (m_rtmp)
    m_libRTMP.Pause(m_rtmp, m_bPaused);

  return true;
}
#endif
