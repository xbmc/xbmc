#pragma once
/*
 *      Copyright (C) 2012-present Team Kodi
 *      http://kodi.tv
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

#include "network/httprequesthandler/HTTPFileHandler.h"

class CHTTPImageHandler : public CHTTPFileHandler
{
public:
  CHTTPImageHandler() = default;
  ~CHTTPImageHandler() override = default;

  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPImageHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const override;

  int GetPriority() const override { return 5; }
  int GetMaximumAgeForCaching() const override { return 60 * 60 * 24 * 7; }

protected:
  explicit CHTTPImageHandler(const HTTPRequest &request);
};
