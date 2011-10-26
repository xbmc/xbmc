#pragma once
/*
 *      Copyright (C) 2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IHTTPRequestHandler.h"
#include "addons/IAddon.h"

class CHTTPWebinterfaceHandler : public IHTTPRequestHandler
{
public:
  CHTTPWebinterfaceHandler() { };
  
  virtual IHTTPRequestHandler* GetInstance() { return new CHTTPWebinterfaceHandler(); }
  virtual bool CheckHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version);

#if (MHD_VERSION >= 0x00040001)
  virtual int HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                                const char *upload_data, size_t *upload_data_size, void **con_cls);
#else
  virtual int HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                                const char *upload_data, unsigned int *upload_data_size, void **con_cls);
#endif

  virtual std::string GetHTTPRedirectUrl() const { return m_url; }
  virtual std::string GetHTTPResponseFile() const { return m_url; }
  
  static int ResolveUrl(const std::string &url, std::string &path);
  static int ResolveUrl(const std::string &url, std::string &path, ADDON::AddonPtr &addon);

private:
  std::string m_url;
};
