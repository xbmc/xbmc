/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include <limits>

#include "IHTTPRequestHandler.h"
#include "network/WebServer.h"
#include "utils/StringUtils.h"

IHTTPRequestHandler::IHTTPRequestHandler()
  : m_request(),
    m_response(),
    m_postFields(),
    m_ranged(false)
{ }

IHTTPRequestHandler::IHTTPRequestHandler(const HTTPRequest &request)
  : m_request(request),
    m_response(),
    m_postFields(),
    m_ranged(false)
{
  m_response.type = HTTPError;
  m_response.status = MHD_HTTP_INTERNAL_SERVER_ERROR;
  m_response.totalLength = 0;
}

bool IHTTPRequestHandler::HasResponseHeader(const std::string &field) const
{
  if (field.empty())
    return false;

  return m_response.headers.find(field) != m_response.headers.end();
}

bool IHTTPRequestHandler::AddResponseHeader(const std::string &field, const std::string &value, bool allowMultiple /* = false */)
{
  if (field.empty() || value.empty())
    return false;

  if (!allowMultiple && HasResponseHeader(field))
    return false;

  m_response.headers.insert(std::make_pair(field, value));
  return true;
}

void IHTTPRequestHandler::AddPostField(const std::string &key, const std::string &value)
{
  if (key.empty())
    return;

  std::map<std::string, std::string>::iterator field = m_postFields.find(key);
  if (field == m_postFields.end())
    m_postFields[key] = value;
  else
    m_postFields[key].append(value);
}

#if (MHD_VERSION >= 0x00040001)
bool IHTTPRequestHandler::AddPostData(const char *data, size_t size)
#else
bool IHTTPRequestHandler::AddPostData(const char *data, unsigned int size)
#endif
{
  if (size > 0)
    return appendPostData(data, size);
  
  return true;
}

bool IHTTPRequestHandler::GetRequestedRanges(uint64_t totalLength)
{
  if (!m_ranged || m_request.webserver == NULL || m_request.connection == NULL)
    return false;

  m_request.ranges.Clear();
  if (totalLength == 0)
    return true;

  return m_request.webserver->GetRequestedRanges(m_request.connection, totalLength, m_request.ranges);
}

bool IHTTPRequestHandler::GetHostnameAndPort(std::string& hostname, uint16_t &port)
{
  if (m_request.webserver == NULL || m_request.connection == NULL)
    return false;

  std::string hostnameAndPort = m_request.webserver->GetRequestHeaderValue(m_request.connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_HOST);
  if (hostnameAndPort.empty())
    return false;

  size_t pos = hostnameAndPort.find(':');
  hostname = hostnameAndPort.substr(0, pos);
  if (hostname.empty())
    return false;

  if (pos != std::string::npos)
  {
    std::string strPort = hostnameAndPort.substr(pos + 1);
    if (!StringUtils::IsNaturalNumber(strPort))
      return false;

    unsigned long portL = strtoul(strPort.c_str(), NULL, 0);
    if (portL > std::numeric_limits<uint16_t>::max())
      return false;

    port = static_cast<uint16_t>(portL);
  }
  else
    port = 80;

  return true;
}