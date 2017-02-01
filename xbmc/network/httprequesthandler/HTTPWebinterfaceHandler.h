#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "addons/IAddon.h"
#include "network/httprequesthandler/HTTPFileHandler.h"

class CHTTPWebinterfaceHandler : public CHTTPFileHandler
{
public:
  CHTTPWebinterfaceHandler() { }
  virtual ~CHTTPWebinterfaceHandler() { }
  
  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) { return new CHTTPWebinterfaceHandler(request); }
  virtual bool CanHandleRequest(const HTTPRequest &request);

  static int ResolveUrl(const std::string &url, std::string &path);
  static int ResolveUrl(const std::string &url, std::string &path, ADDON::AddonPtr &addon);
  static bool ResolveAddon(const std::string &url, ADDON::AddonPtr &addon);
  static bool ResolveAddon(const std::string &url, ADDON::AddonPtr &addon, std::string &addonPath);

protected:
  explicit CHTTPWebinterfaceHandler(const HTTPRequest &request);
};
