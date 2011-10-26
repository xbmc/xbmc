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

#include <string>
#include <map>
#include <sys/socket.h>
#ifdef __APPLE__
#include "lib/libmicrohttpd/src/include/microhttpd.h"
#else
#include <microhttpd.h>
#endif

class CWebServer;

enum HTTPMethod
{
  UNKNOWN,
  POST,
  GET,
  HEAD
};

enum HTTPResponseType
{
  HTTPNone,
  HTTPError,
  HTTPRedirect,
  HTTPFileDownload,
  HTTPMemoryDownloadNoFreeNoCopy,
  HTTPMemoryDownloadNoFreeCopy,
  HTTPMemoryDownloadFreeNoCopy,
  HTTPMemoryDownloadFreeCopy
};

class IHTTPRequestHandler
{
public:
  virtual ~IHTTPRequestHandler() { }

  virtual IHTTPRequestHandler* GetInstance() = 0;
  virtual bool CheckHTTPRequest(struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version) = 0;

#if (MHD_VERSION >= 0x00040001)
  virtual int HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                                const char *upload_data, size_t *upload_data_size, void **con_cls) = 0;
#else
  virtual int HandleHTTPRequest(CWebServer *webserver, struct MHD_Connection *connection, const std::string &url, HTTPMethod method, const std::string &version,
                                const char *upload_data, unsigned int *upload_data_size, void **con_cls) = 0;
#endif
  
  virtual void* GetHTTPResponseData() const { return NULL; };
  virtual size_t GetHTTPResonseDataLength() const { return 0; }
  virtual std::string GetHTTPRedirectUrl() const { return ""; }
  virtual std::string GetHTTPResponseFile() const { return ""; }

  // The higher the more important
  virtual int GetPriority() const { return 0; }

  int GetHTTPResonseCode() const { return m_responseCode; }
  HTTPResponseType GetHTTPResponseType() const { return m_responseType; }
  const std::map<std::string, std::string>& GetHTTPResponseHeaderFields() const { return m_responseHeaderFields; };

protected:
  int m_responseCode;
  HTTPResponseType m_responseType;
  std::map<std::string, std::string> m_responseHeaderFields;
};
