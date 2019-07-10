/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

#include <map>
#include <stdint.h>
#include <string>

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
