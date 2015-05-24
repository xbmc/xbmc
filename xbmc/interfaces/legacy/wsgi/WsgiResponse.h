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

#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/Tuple.h"
#include "interfaces/legacy/wsgi/WsgiResponseBody.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    typedef Tuple<String, String> WsgiHttpHeader;

    /**
     * Represents the start_response callable passed to a WSGI handler.
     */
    class WsgiResponse : public AddonClass
    {
    public:
      WsgiResponse();
      virtual ~WsgiResponse();

      /**
       * Callable implementation to initialize the response with the given
       * HTTP status and the HTTP response headers.
       */
      WsgiResponseBody* operator()(const String& status, const std::vector<WsgiHttpHeader>& response_headers, void* exc_info = NULL);

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


