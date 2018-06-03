#pragma once
/*
*      Copyright (C) 2015-present Team Kodi
*      This file is part of Kodi - https://kodi.tv
*
*  Kodi is free software: you can redistribute it and/or modify it
*  under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  Kodi is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
*
*/

#include <string>

#include "interfaces/python/PythonInvoker.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

class CHTTPPythonInvoker : public CPythonInvoker
{
public:
  ~CHTTPPythonInvoker() override;

  virtual HTTPPythonRequest* GetRequest() = 0;

protected:
  CHTTPPythonInvoker(ILanguageInvocationHandler* invocationHandler, HTTPPythonRequest* request);

  // overrides of CPythonInvoker
  void onAbort() override;
  void onError(const std::string& exceptionType = "", const std::string& exceptionValue = "", const std::string& exceptionTraceback = "") override;

  HTTPPythonRequest* m_request;
  bool m_internalError;
};
