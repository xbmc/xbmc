/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WsgiErrorStream.h"

#include "network/httprequesthandler/python/HTTPPythonRequest.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

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
        CLog::Log(LOGERROR, "WSGI [{}]: {}", m_request->url, msg);
      else
        CLog::Log(LOGERROR, "WSGI: {}", msg);
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
