/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPWebinterfaceAddonsHandler : public IHTTPRequestHandler
{
public:
  CHTTPWebinterfaceAddonsHandler() = default;
  ~CHTTPWebinterfaceAddonsHandler() override = default;

  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPWebinterfaceAddonsHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const override;

  MHD_RESULT HandleRequest() override;

  HttpResponseRanges GetResponseData() const override;

  int GetPriority() const override { return 4; }

protected:
  explicit CHTTPWebinterfaceAddonsHandler(const HTTPRequest &request)
    : IHTTPRequestHandler(request)
  { }

private:
  std::string m_responseData;
  CHttpResponseRange m_responseRange;
};
