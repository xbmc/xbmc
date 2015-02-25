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

#include <map>
#include <string>

#include "network/httprequesthandler/IHTTPRequestHandler.h"
#include "network/httprequesthandler/python/HTTPPythonInvoker.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    class HttpRequest;
  }
}

class CHTTPModPythonInvoker : public CHTTPPythonInvoker
{
public:
  CHTTPModPythonInvoker(ILanguageInvocationHandler* invocationHandler, HTTPPythonRequest* request);
  virtual ~CHTTPModPythonInvoker();

  // implementations of CHTTPPythonInvoker
  virtual HTTPPythonRequest* GetRequest();

protected:
  // overrides of CPythonInvoker
  virtual std::map<std::string, PythonModuleInitialization> getModules() const;
  virtual const char* getInitializationScript() const;
  virtual void onPythonModuleInitialization(void* moduleDict);

private:
  XBMCAddon::xbmcmod_python::HttpRequest* m_httpRequest;
};
