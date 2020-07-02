/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/IAddon.h"
#include "addons/Webinterface.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPPythonHandler : public IHTTPRequestHandler
{
public:
  CHTTPPythonHandler();
  ~CHTTPPythonHandler() override = default;

  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPPythonHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const override;
  bool CanHandleRanges() const override { return false; }
  bool CanBeCached() const override { return false; }
  bool GetLastModifiedDate(CDateTime &lastModified) const override;

  MHD_RESULT HandleRequest() override;

  HttpResponseRanges GetResponseData() const override { return m_responseRanges; }

  std::string GetRedirectUrl() const override { return m_redirectUrl; }

  int GetPriority() const override { return 3; }

protected:
  explicit CHTTPPythonHandler(const HTTPRequest &request);

  bool appendPostData(const char *data, size_t size) override;

private:
  std::string m_scriptPath;
  ADDON::AddonPtr m_addon;
  CDateTime m_lastModified;

  std::string m_requestData;
  std::string m_responseData;
  HttpResponseRanges m_responseRanges;

  std::string m_redirectUrl;
};
