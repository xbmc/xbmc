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

#include "DVDInputStreamFFmpeg.h"

#include "filesystem/CurlFile.h"
#include "playlists/PlayListM3U.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <limits.h>

using namespace XFILE;
using PLAYLIST::CPlayListM3U;

CDVDInputStreamFFmpeg::CDVDInputStreamFFmpeg(const CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_FFMPEG, fileitem)
  , m_can_pause(false)
  , m_can_seek(false)
  , m_aborted(false)
{

}

CDVDInputStreamFFmpeg::~CDVDInputStreamFFmpeg()
{
  Close();
}

bool CDVDInputStreamFFmpeg::IsEOF()
{
  if(m_aborted)
    return true;
  else
    return false;
}

bool CDVDInputStreamFFmpeg::Open()
{
  if (!CDVDInputStream::Open())
    return false;

  m_can_pause = true;
  m_can_seek  = true;
  m_aborted   = false;

  if(strnicmp(m_item.GetPath().c_str(), "udp://", 6) == 0 ||
     strnicmp(m_item.GetPath().c_str(), "rtp://", 6) == 0)
  {
    m_can_pause = false;
    m_can_seek = false;
    m_realtime = true;
  }

  if(strnicmp(m_item.GetPath().c_str(), "tcp://", 6) == 0)
  {
    m_can_pause = true;
    m_can_seek  = false;
  }
  return true;
}

// close file and reset everything
void CDVDInputStreamFFmpeg::Close()
{
  CDVDInputStream::Close();
}

int CDVDInputStreamFFmpeg::Read(uint8_t* buf, int buf_size)
{
  return -1;
}

int64_t CDVDInputStreamFFmpeg::GetLength()
{
  return 0;
}

int64_t CDVDInputStreamFFmpeg::Seek(int64_t offset, int whence)
{
  return -1;
}

std::string CDVDInputStreamFFmpeg::GetProxyType() const
{
  return m_item.HasProperty("proxy.type") ?
    m_item.GetProperty("proxy.type").asString() : std::string();
}

std::string CDVDInputStreamFFmpeg::GetProxyHost() const
{
  return m_item.HasProperty("proxy.host") ?
    m_item.GetProperty("proxy.host").asString() : std::string();
}

uint16_t CDVDInputStreamFFmpeg::GetProxyPort() const
{
  if (m_item.HasProperty("proxy.port"))
    return m_item.GetProperty("proxy.port").asInteger();

  // Select the standard port
  const std::string value = GetProxyType();
  if (value == "socks4" || value == "socks4a" ||
      value == "socks5" || value == "socks5-remote")
    return 1080;
  else
    return 3128;
}

std::string CDVDInputStreamFFmpeg::GetProxyUser() const
{
  return m_item.HasProperty("proxy.user") ?
    m_item.GetProperty("proxy.user").asString() : std::string();
}

std::string CDVDInputStreamFFmpeg::GetProxyPassword() const
{
  return m_item.HasProperty("proxy.password") ?
    m_item.GetProperty("proxy.password").asString() : std::string();
}

std::string CDVDInputStreamFFmpeg::GetFileName()
{
  CURL url = GetURL();
  // rtmp options
  if (url.IsProtocol("rtmp")  || url.IsProtocol("rtmpt")  ||
      url.IsProtocol("rtmpe") || url.IsProtocol("rtmpte") ||
      url.IsProtocol("rtmps"))
  {
    std::vector<std::string> opts = StringUtils::Split(url.Get(), " ");
    if (opts.size() > 0)
    {
      return opts.front();
    }
    return url.Get();
  }
  return CDVDInputStream::GetFileName();
}
