/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/Tuple.h"
#include "interfaces/legacy/wsgi/WsgiResponseBody.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

#include <vector>

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    typedef Tuple<String, String> WsgiHttpHeader;

    /// \defgroup python_xbmcwsgi_WsgiResponse WsgiResponse
    /// \ingroup python_xbmcwsgi
    /// @{
    /// @brief **Represents the start_response callable passed to a WSGI handler.**
    ///
    /// \python_class{ WsgiResponse() }
    ///
    ///-------------------------------------------------------------------------
    ///
    class WsgiResponse : public AddonClass
    {
    public:
      WsgiResponse();
      ~WsgiResponse() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcwsgi_WsgiInputStreamIterator
      /// \python_func{ operator(status, response_headers[, exc_info]) }
      ///------------------------------------------------------------------------
      ///
      /// Callable implementation to initialize the response with the given
      /// HTTP status and the HTTP response headers.
      ///
      /// @param status               an HTTP status string like 200 OK or 404
      ///                             Not Found.
      /// @param response_headers     a list of (header_name, header_value)
      ///                             tuples. It must be a Python list. Each
      ///                             header_name must be a valid HTTP header
      ///                             field-name (as
      /// @param exc_info             [optional] python sys.exc_info() tuple.
      ///                             This argument should be supplied by the
      ///                             application only if start_response is
      ///                             being called by an error
      /// @return                     The write() method \ref python_xbmcwsgi_WsgiResponseBody "WsgiResponseBody"
      ///
      operator(...);
#else
      WsgiResponseBody* operator()(const String& status, const std::vector<WsgiHttpHeader>& response_headers, void* exc_info = NULL);
#endif

#ifndef SWIG
      void Append(const std::string& data);

      bool Finalize(HTTPPythonRequest* request) const;

    private:
      bool m_called = false;
      int m_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
      std::multimap<std::string, std::string> m_responseHeaders;

      WsgiResponseBody m_body;
#endif
    };
  }
}
