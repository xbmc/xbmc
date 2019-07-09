/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/httprequesthandler/HTTPFileHandler.h"

#include <string>

class CHTTPVfsHandler : public CHTTPFileHandler
{
public:
  CHTTPVfsHandler() = default;
  ~CHTTPVfsHandler() override = default;

  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPVfsHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const override;

  int GetPriority() const override { return 5; }

protected:
  explicit CHTTPVfsHandler(const HTTPRequest &request);
};
