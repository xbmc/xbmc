#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#include <stdint.h>

#include <map>
#include <string>

#include "XBDateTime.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

typedef struct HTTPPythonRequest
{
  struct MHD_Connection *connection;
  std::string hostname;
  uint16_t port;
  std::string url;
  std::string path;
  std::string file;
  HTTPMethod method;
  std::string version;
  std::multimap<std::string, std::string> headerValues;
  std::map<std::string, std::string> getValues;
  std::map<std::string, std::string> postValues;
  std::string requestContent;
  CDateTime requestTime;
  CDateTime lastModifiedTime;

  HTTPResponseType responseType;
  int responseStatus;
  std::string responseContentType;
  std::string responseData;
  size_t responseLength;
  std::multimap<std::string, std::string> responseHeaders;
  std::multimap<std::string, std::string> responseHeadersError;
} HTTPPythonRequest;