/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/python/PythonInvoker.h"
#include "network/httprequesthandler/python/HTTPPythonInvoker.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

#include <map>
#include <string>

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    class WsgiResponse;
  }
}

class CHTTPPythonWsgiInvoker : public CHTTPPythonInvoker
{
public:
  CHTTPPythonWsgiInvoker(ILanguageInvocationHandler* invocationHandler, HTTPPythonRequest* request);
  ~CHTTPPythonWsgiInvoker() override;

  // implementations of CHTTPPythonInvoker
  HTTPPythonRequest* GetRequest() override;

protected:
  // overrides of CPythonInvoker
  void executeScript(FILE* fp, const std::string& script, PyObject* moduleDict) override;
  std::map<std::string, PythonModuleInitialization> getModules() const override;
  const char* getInitializationScript() const override;

private:
  static std::map<std::string, std::string> createCgiEnvironment(const HTTPPythonRequest* httpRequest, ADDON::AddonPtr addon);
  static void addWsgiEnvironment(HTTPPythonRequest* request, void* environment);

  XBMCAddon::xbmcwsgi::WsgiResponse* m_wsgiResponse;
};
