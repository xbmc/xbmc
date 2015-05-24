/*
 *      Copyright (C) 2015 Team XBMC
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

#include "WsgiResponse.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    WsgiResponse::WsgiResponse()
      : m_called(false),
        m_status(MHD_HTTP_INTERNAL_SERVER_ERROR),
        m_responseHeaders(),
        m_body()
    { }

    WsgiResponse::~WsgiResponse()
    { }

    WsgiResponseBody* WsgiResponse::operator()(const String& status, const std::vector<WsgiHttpHeader>& response_headers, void* exc_info /* = NULL */)
    {
      if (m_called)
      {
        CLog::Log(LOGWARNING, "WsgiResponse: callable has already been called");
        return NULL;
      }

      m_called = true;

      // parse the status
      if (!status.empty())
      {
        std::vector<String> statusParts = StringUtils::Split(status, ' ', 2);
        if (statusParts.size() == 2 && StringUtils::IsNaturalNumber(statusParts.front()))
        {
          int64_t parsedStatus = strtol(statusParts.front().c_str(), NULL, 0);
          if (parsedStatus >= MHD_HTTP_OK && parsedStatus <= MHD_HTTP_NOT_EXTENDED)
            m_status = static_cast<int>(parsedStatus);
          else
            CLog::Log(LOGWARNING, "WsgiResponse: invalid status number %" PRId64 " in \"%s\" provided", parsedStatus, status.c_str());
        }
        else
          CLog::Log(LOGWARNING, "WsgiResponse: invalid status \"%s\" provided", status.c_str());
      }
      else
        CLog::Log(LOGWARNING, "WsgiResponse: empty status provided");

      // copy the response headers
      for (std::vector<WsgiHttpHeader>::const_iterator headerIt = response_headers.begin(); headerIt != response_headers.end(); ++headerIt)
        m_responseHeaders.insert(std::make_pair(headerIt->first(), headerIt->second()));

      return &m_body;
    }

#ifndef SWIG
    void WsgiResponse::Append(const std::string& data)
    {
      if (!data.empty())
        m_body.m_data.append(data);
    }

    bool WsgiResponse::Finalize(HTTPPythonRequest* request) const
    {
      if (request == NULL || !m_called)
        return false;

      // copy the response status
      request->responseStatus = m_status;

      // copy the response headers
      if (m_status >= MHD_HTTP_OK && m_status < MHD_HTTP_BAD_REQUEST)
        request->responseHeaders.insert(m_responseHeaders.begin(), m_responseHeaders.end());
      else
        request->responseHeadersError.insert(m_responseHeaders.begin(), m_responseHeaders.end());

      // copy the body
      request->responseData = m_body.m_data;

      return true;
    }
#endif
  }
}
