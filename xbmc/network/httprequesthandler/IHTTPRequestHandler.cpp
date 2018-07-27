/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <limits>
#include <utility>

#include "IHTTPRequestHandler.h"
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPRequestHandlerUtils.h"
#include "utils/StringUtils.h"

static const std::string HTTPMethodHead = "HEAD";
static const std::string HTTPMethodGet = "GET";
static const std::string HTTPMethodPost = "POST";

HTTPMethod GetHTTPMethod(const char *method)
{
  if (HTTPMethodGet.compare(method) == 0)
    return GET;
  if (HTTPMethodPost.compare(method) == 0)
    return POST;
  if (HTTPMethodHead.compare(method) == 0)
    return HEAD;

  return UNKNOWN;
}

std::string GetHTTPMethod(HTTPMethod method)
{
  switch (method)
  {
  case HEAD:
    return HTTPMethodHead;

  case GET:
    return HTTPMethodGet;

  case POST:
    return HTTPMethodPost;

  case UNKNOWN:
    break;
  }

  return "";
}

IHTTPRequestHandler::IHTTPRequestHandler()
  : m_request(),
    m_response(),
    m_postFields()
{ }

IHTTPRequestHandler::IHTTPRequestHandler(const HTTPRequest &request)
  : m_request(request),
    m_response(),
    m_postFields()
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

bool IHTTPRequestHandler::AddPostData(const char *data, size_t size)
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

  return HTTPRequestHandlerUtils::GetRequestedRanges(m_request.connection, totalLength, m_request.ranges);
}

bool IHTTPRequestHandler::GetHostnameAndPort(std::string& hostname, uint16_t &port)
{
  if (m_request.webserver == NULL || m_request.connection == NULL)
    return false;

  std::string hostnameAndPort = HTTPRequestHandlerUtils::GetRequestHeaderValue(m_request.connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_HOST);
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
