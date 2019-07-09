/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "network/httprequesthandler/HTTPFileHandler.h"

#include <string>

class CHTTPWebinterfaceHandler : public CHTTPFileHandler
{
public:
  CHTTPWebinterfaceHandler() = default;
  ~CHTTPWebinterfaceHandler() override = default;

  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPWebinterfaceHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const override;

  static int ResolveUrl(const std::string &url, std::string &path);
  static int ResolveUrl(const std::string &url, std::string &path, ADDON::AddonPtr &addon);
  static bool ResolveAddon(const std::string &url, ADDON::AddonPtr &addon);
  static bool ResolveAddon(const std::string &url, ADDON::AddonPtr &addon, std::string &addonPath);

protected:
  explicit CHTTPWebinterfaceHandler(const HTTPRequest &request);
};
