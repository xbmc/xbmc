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

#include "XBDateTime.h"
#include "addons/IAddon.h"
#include "addons/Webinterface.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPPythonHandler : public IHTTPRequestHandler
{
public:
  CHTTPPythonHandler();
  virtual ~CHTTPPythonHandler() { }
  
  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) { return new CHTTPPythonHandler(request); }
  virtual bool CanHandleRequest(const HTTPRequest &request);
  virtual bool CanHandleRanges() const { return false; }
  virtual bool CanBeCached() const { return false; }
  virtual bool GetLastModifiedDate(CDateTime &lastModified) const;

  virtual int HandleRequest();

  virtual HttpResponseRanges GetResponseData() const { return m_responseRanges; }

  virtual std::string GetRedirectUrl() const { return m_redirectUrl; }

  virtual int GetPriority() const { return 3; }

protected:
  explicit CHTTPPythonHandler(const HTTPRequest &request);

#if (MHD_VERSION >= 0x00040001)
  virtual bool appendPostData(const char *data, size_t size);
#else
  virtual bool appendPostData(const char *data, unsigned int size);
#endif

private:
  std::string m_scriptPath;
  ADDON::AddonPtr m_addon;
  ADDON::WebinterfaceType m_type;
  CDateTime m_lastModified;

  std::string m_requestData;
  std::string m_responseData;
  HttpResponseRanges m_responseRanges;

  std::string m_redirectUrl;
};
