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

#include "WsgiErrorStream.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    WsgiErrorStream::WsgiErrorStream()
      : m_request(NULL)
    { }

    WsgiErrorStream::~WsgiErrorStream()
    {
      m_request = NULL;
    }

    void WsgiErrorStream::write(const String& str)
    {
      if (str.empty())
        return;

      String msg = str;
      // remove a trailing \n
      if (msg.at(msg.size() - 1) == '\n')
        msg.erase(msg.size() - 1);

      if (m_request != NULL)
        CLog::Log(LOGERROR, "WSGI [%s]: %s", m_request->url.c_str(), msg.c_str());
      else
        CLog::Log(LOGERROR, "WSGI: %s", msg.c_str());
    }

    void WsgiErrorStream::writelines(const std::vector<String>& seq)
    {
      if (seq.empty())
        return;

      String msg = StringUtils::Join(seq, "");
      write(msg);
    }

#ifndef SWIG
    void WsgiErrorStream::SetRequest(HTTPPythonRequest* request)
    {
      if (m_request != NULL)
        return;

      m_request = request;
    }
#endif
  }
}
