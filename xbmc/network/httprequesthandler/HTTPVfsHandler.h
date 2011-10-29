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

#include "utils/StdString.h"

class CHTTPVfsHandler : public IHTTPRequestHandler
{
public:
  CHTTPVfsHandler() { };
  
  virtual IHTTPRequestHandler* GetInstance() { return new CHTTPVfsHandler(); }
  virtual bool CheckHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version);

#if (MHD_VERSION >= 0x00040001)
  virtual int HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version);
#else
  virtual int HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version);
#endif

  virtual std::string GetHTTPResponseFile() const { return m_path; }

  virtual int GetPriority() const { return 2; }

private:
  CStdString m_path;
};
