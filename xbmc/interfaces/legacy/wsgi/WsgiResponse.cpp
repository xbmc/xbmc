/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WsgiResponse.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <inttypes.h>
#include <utility>

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    WsgiResponse::WsgiResponse()
      : m_responseHeaders(),
        m_body()
    { }

    WsgiResponse::~WsgiResponse() = default;

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
            CLog::Log(LOGWARNING, "WsgiResponse: invalid status number {} in \"{}\" provided",
                      parsedStatus, status);
        }
        else
          CLog::Log(LOGWARNING, "WsgiResponse: invalid status \"{}\" provided", status);
      }
      else
        CLog::Log(LOGWARNING, "WsgiResponse: empty status provided");

      // copy the response headers
      for (const auto& headerIt : response_headers)
        m_responseHeaders.insert({headerIt.first(), headerIt.second()});

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
