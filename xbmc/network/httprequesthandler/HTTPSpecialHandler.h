#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IHTTPRequestHandler.h"

class CHTTPSpecialHandler : public IHTTPRequestHandler
{
public:
  CHTTPSpecialHandler() { };

  virtual IHTTPRequestHandler* GetInstance() { return new CHTTPSpecialHandler(); }
  virtual bool CheckHTTPRequest(const HTTPRequest &request);
  virtual int HandleHTTPRequest(const HTTPRequest &request);

  virtual std::string GetHTTPResponseFile() const { return m_path; }
  static int ResolveUrl(const std::string &url, std::string &path);
  virtual void* GetHTTPResponseData() const { return (void *)m_response.c_str(); };
  virtual size_t GetHTTPResonseDataLength() const { return m_response.length(); }

  virtual int GetPriority() const { return 2; }

private:
  std::string m_path;
  std::string m_request;
  std::string m_response;
};
