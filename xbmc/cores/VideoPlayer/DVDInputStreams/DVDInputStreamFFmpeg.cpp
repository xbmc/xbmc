/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStreamFFmpeg.h"

#include "playlists/PlayListM3U.h"
#include "utils/StringUtils.h"


using namespace XFILE;

CDVDInputStreamFFmpeg::CDVDInputStreamFFmpeg(const CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_FFMPEG, fileitem)
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

  m_aborted = false;

  if(strnicmp(m_item.GetDynPath().c_str(), "udp://", 6) == 0 ||
     strnicmp(m_item.GetDynPath().c_str(), "rtp://", 6) == 0)
  {
    m_realtime = true;
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
    return static_cast<uint16_t>(m_item.GetProperty("proxy.port").asInteger());

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
