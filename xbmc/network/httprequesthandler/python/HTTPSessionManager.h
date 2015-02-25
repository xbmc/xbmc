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

#include <string>
#include <map>

#include "network/httprequesthandler/python/HTTPPythonInvoker.h"
#include "threads/CriticalSection.h"

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    class HttpRequest;
    class Session;
  }
}

class CHTTPSessionManager
{
public:
  static CHTTPSessionManager& Get();

  static bool IsValidSessionId(const std::string& sessionId);

  std::string GenerateSessionId(XBMCAddon::xbmcmod_python::HttpRequest* request);

  const XBMCAddon::xbmcmod_python::Session* Get(XBMCAddon::xbmcmod_python::HttpRequest* request, const std::string& sessionId = "");
  bool Save(XBMCAddon::xbmcmod_python::Session* session);
  void Remove(const std::string& sessionId, bool invalidate = false);

private:
  CHTTPSessionManager();
  CHTTPSessionManager(const CHTTPSessionManager&);
  ~CHTTPSessionManager();

  static std::string getSessionIdCookie(const std::string& sessionId, int maxAge = -1);
  static std::string getSessionIdCookieName();

  std::map<std::string, XBMCAddon::xbmcmod_python::Session*> m_sessions;
  CCriticalSection m_critSection;
};
