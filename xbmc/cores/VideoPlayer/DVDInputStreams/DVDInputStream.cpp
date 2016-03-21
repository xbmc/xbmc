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

#include "DVDInputStream.h"
#include "URL.h"
#include "utils/log.h"

CDVDInputStream::CDVDInputStream(DVDStreamType streamType, CFileItem& fileitem)
{
  m_streamType = streamType;
  m_contentLookup = true;
  m_realtime = false;
  m_item = fileitem;
}

CDVDInputStream::~CDVDInputStream()
{

}

bool CDVDInputStream::Open()
{
  m_content = m_item.GetMimeType();
  m_contentLookup = m_item.ContentLookup();
  return true;
}

void CDVDInputStream::Close()
{

}

std::string CDVDInputStream::GetFileName()
{
  CURL url(m_item.GetPath());

  url.SetProtocolOptions("");
  return url.Get();
}

ProxyType CDVDInputStream::GetProxyType() const
{
  if (m_item.HasProperty("proxy.type"))
  {
    const std::string value = m_item.GetProperty("proxy.type").asString();
    if (value == "http")
      return PROXY_HTTP;
    else if (value == "socks4")
      return PROXY_SOCKS4;
    else if (value == "socks4a")
      return PROXY_SOCKS4A;
    else if (value == "socks5")
      return PROXY_SOCKS5;
    else if (value == "socks5-remote")
      return PROXY_SOCKS5_REMOTE;
    else
      CLog::Log(LOGERROR,"Invalid proxy type \"%s\"", value.c_str());
  }

  return PROXY_HTTP;
}

std::string CDVDInputStream::GetProxyHost() const
{
  return m_item.HasProperty("proxy.host") ?
    m_item.GetProperty("proxy.host").asString() : std::string();
}

uint16_t CDVDInputStream::GetProxyPort() const
{
  if (m_item.HasProperty("proxy.port"))
    return m_item.GetProperty("proxy.port").asInteger();

  // Select the standard port
  switch (GetProxyType()) {
  case PROXY_SOCKS4:
  case PROXY_SOCKS4A:
  case PROXY_SOCKS5:
  case PROXY_SOCKS5_REMOTE:
    return 1080;
  case PROXY_HTTP:
  default:
    return 3128;
  }
}

std::string CDVDInputStream::GetProxyUser() const
{
  return m_item.HasProperty("proxy.user") ?
    m_item.GetProperty("proxy.user").asString() : std::string();
}

std::string CDVDInputStream::GetProxyPassword() const
{
  return m_item.HasProperty("proxy.password") ?
    m_item.GetProperty("proxy.password").asString() : std::string();
}
