#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include <string>
#include <map>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <microhttpd.h>

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

typedef struct HTTPRequest
{
  struct MHD_Connection *connection;
  std::string url;
  HTTPMethod method;
  std::string version;
  CWebServer *webserver;
} HTTPRequest;

class IHTTPRequestHandler
{
public:
  virtual ~IHTTPRequestHandler() { }

  virtual IHTTPRequestHandler* GetInstance() = 0;
  virtual bool CheckHTTPRequest(const HTTPRequest &request) = 0;
  virtual int HandleHTTPRequest(const HTTPRequest &request) = 0;
  
  virtual void* GetHTTPResponseData() const { return NULL; };
  virtual size_t GetHTTPResonseDataLength() const { return 0; }
  virtual std::string GetHTTPRedirectUrl() const { return ""; }
  virtual std::string GetHTTPResponseFile() const { return ""; }

  // The higher the more important
  virtual int GetPriority() const { return 0; }

  void AddPostField(const std::string &key, const std::string &value);
#if (MHD_VERSION >= 0x00040001)
  bool AddPostData(const char *data, size_t size);
#else
  bool AddPostData(const char *data, unsigned int size);
#endif

  int GetHTTPResonseCode() const { return m_responseCode; }
  HTTPResponseType GetHTTPResponseType() const { return m_responseType; }
  const std::multimap<std::string, std::string>& GetHTTPResponseHeaderFields() const { return m_responseHeaderFields; };

protected:
#if (MHD_VERSION >= 0x00040001)
  virtual bool appendPostData(const char *data, size_t size)
#else
  virtual bool appendPostData(const char *data, unsigned int size)
#endif
  { return true; }

  int m_responseCode;
  HTTPResponseType m_responseType;
  std::multimap<std::string, std::string> m_responseHeaderFields;

  std::map<std::string, std::string> m_postFields;
};
