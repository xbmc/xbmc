/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
 
#include <stdio.h>

#include "HttpResponse.h"

#define SPACE     " "
#define SEPARATOR ": "
#define LINEBREAK "\r\n"

#define HEADER_CONTENT_LENGTH "Content-Length"

std::map<HTTP::StatusCode, std::string> CHttpResponse::m_statusCodeText = CHttpResponse::createStatusCodes();

CHttpResponse::CHttpResponse(HTTP::Method method, HTTP::StatusCode status, HTTP::Version version /* = HTTPVersion1_1 */)
{
  m_method = method;
  m_status = status;
  m_version = version;

  m_content = NULL;
  m_contentLength = 0;
}

void CHttpResponse::AddHeader(const std::string &field, const std::string &value)
{
  if (field.empty())
    return;

  m_headers.push_back(std::pair<std::string, std::string>(field, value));
}

void CHttpResponse::SetContent(const char* data, unsigned int length)
{
  m_content = data;

  if (m_content == NULL)
    m_contentLength = 0;
  else
    m_contentLength = length;
}

unsigned int CHttpResponse::Create(char *&response)
{
  m_buffer.clear();

  m_buffer.append("HTTP/");
  switch (m_version)
  {
    case HTTP::Version1_0:
      m_buffer.append("1.0");
      break;

    case HTTP::Version1_1:
      m_buffer.append("1.1");
      break;

    default:
      return 0;
  }

  char statusBuffer[4];
  sprintf(statusBuffer, "%d", (int)m_status);
  m_buffer.append(SPACE);
  m_buffer.append(statusBuffer);

  m_buffer.append(SPACE);
  m_buffer.append(m_statusCodeText.find(m_status)->second);
  m_buffer.append(LINEBREAK);

  bool hasContentLengthHeader = false;
  for (unsigned int index = 0; index < m_headers.size(); index++)
  {
    m_buffer.append(m_headers[index].first);
    m_buffer.append(SEPARATOR);
    m_buffer.append(m_headers[index].second);
    m_buffer.append(LINEBREAK);

    if (m_headers[index].first.compare(HEADER_CONTENT_LENGTH) == 0)
      hasContentLengthHeader = true;
  }

  if (!hasContentLengthHeader && m_content != NULL && m_contentLength > 0)
  {
    m_buffer.append(HEADER_CONTENT_LENGTH);
    m_buffer.append(SEPARATOR);
    char lengthBuffer[11];
    sprintf(lengthBuffer, "%d", m_contentLength);
    m_buffer.append(lengthBuffer);
    m_buffer.append(LINEBREAK);
  }

  m_buffer.append(LINEBREAK);
  if (m_content != NULL && m_contentLength > 0)
    m_buffer.append(m_content, m_contentLength);

  response = (char *)m_buffer.c_str();
  return m_buffer.size();
}

std::map<HTTP::StatusCode, std::string> CHttpResponse::createStatusCodes()
{
  std::map<HTTP::StatusCode, std::string> map;
  map[HTTP::Continue]                      = "Continue";
  map[HTTP::SwitchingProtocols]            = "Switching Protocols";
  map[HTTP::Processing]                    = "Processing";
  map[HTTP::ConnectionTimedOut]            = "Connection timed out";
  map[HTTP::OK]                            = "OK";
  map[HTTP::Created]                       = "Created";
  map[HTTP::Accepted]                      = "Accepted";
  map[HTTP::NonAuthoritativeInformation]   = "Non-Authoritative Information";
  map[HTTP::NoContent]                     = "No Content";
  map[HTTP::ResetContent]                  = "Reset Content";
  map[HTTP::PartialContent]                = "Partial Content";
  map[HTTP::MultiStatus]                   = "Multi-Status";
  map[HTTP::MultipleChoices]               = "Multiple Choices";
  map[HTTP::MovedPermanently]              = "Moved Permanently";
  map[HTTP::Found]                         = "Found";
  map[HTTP::SeeOther]                      = "See Other";
  map[HTTP::NotModified]                   = "Not Modified";
  map[HTTP::UseProxy]                      = "Use Proxy";
  //map[HTTP::SwitchProxy]                 = "Switch Proxy";
  map[HTTP::TemporaryRedirect]             = "Temporary Redirect";
  map[HTTP::BadRequest]                    = "Bad Request";
  map[HTTP::Unauthorized]                  = "Unauthorized";
  map[HTTP::PaymentRequired]               = "Payment Required";
  map[HTTP::Forbidden]                     = "Forbidden";
  map[HTTP::NotFound]                      = "Not Found";
  map[HTTP::MethodNotAllowed]              = "Method Not Allowed";
  map[HTTP::NotAcceptable]                 = "Not Acceptable";
  map[HTTP::ProxyAuthenticationRequired]   = "Proxy Authentication Required";
  map[HTTP::RequestTimeout]                = "Request Time-out";
  map[HTTP::Conflict]                      = "Conflict";
  map[HTTP::Gone]                          = "Gone";
  map[HTTP::LengthRequired]                = "Length Required";
  map[HTTP::PreconditionFailed]            = "Precondition Failed";
  map[HTTP::RequestEntityTooLarge]         = "Request Entity Too Large";
  map[HTTP::RequestURITooLong]             = "Request-URI Too Long";
  map[HTTP::UnsupportedMediaType]          = "Unsupported Media Type";
  map[HTTP::RequestedRangeNotSatisfiable]  = "Requested range not satisfiable";
  map[HTTP::ExpectationFailed]             = "Expectation Failed";
  map[HTTP::ImATeapot]                     = "I'm a Teapot";
  map[HTTP::TooManyConnections]            = "There are too many connections from your internet address";
  map[HTTP::UnprocessableEntity]           = "Unprocessable Entity";
  map[HTTP::Locked]                        = "Locked";
  map[HTTP::FailedDependency]              = "Failed Dependency";
  map[HTTP::UnorderedCollection]           = "UnorderedCollection";
  map[HTTP::UpgradeRequired]               = "Upgrade Required";
  map[HTTP::InternalServerError]           = "Internal Server Error";
  map[HTTP::NotImplemented]                = "Not Implemented";
  map[HTTP::BadGateway]                    = "Bad Gateway";
  map[HTTP::ServiceUnavailable]            = "Service Unavailable";
  map[HTTP::GatewayTimeout]                = "Gateway Time-out";
  map[HTTP::HTTPVersionNotSupported]       = "HTTP Version not supported";
  map[HTTP::VariantAlsoNegotiates]         = "Variant Also Negotiates";
  map[HTTP::InsufficientStorage]           = "Insufficient Storage";
  map[HTTP::BandwidthLimitExceeded]        = "Bandwidth Limit Exceeded";
  map[HTTP::NotExtended]                   = "Not Extended";

  return map;
}
