#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "network/httprequesthandler/HTTPFileHandler.h"

class CHTTPImageHandler : public CHTTPFileHandler
{
public:
  CHTTPImageHandler() { }
  ~CHTTPImageHandler() override { }

  IHTTPRequestHandler* Create(const HTTPRequest &request) const override { return new CHTTPImageHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const override;

  int GetPriority() const override { return 5; }
  int GetMaximumAgeForCaching() const override { return 60 * 60 * 24 * 7; }

protected:
  explicit CHTTPImageHandler(const HTTPRequest &request);
};
