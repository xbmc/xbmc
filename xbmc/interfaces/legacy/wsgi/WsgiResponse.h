#pragma once
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

#include <vector>

#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/Tuple.h"
#include "interfaces/legacy/wsgi/WsgiResponseBody.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

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
      virtual ~WsgiResponse();

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
      bool m_called;
      int m_status;
      std::multimap<std::string, std::string> m_responseHeaders;

      WsgiResponseBody m_body;
#endif
    };
  }
}
