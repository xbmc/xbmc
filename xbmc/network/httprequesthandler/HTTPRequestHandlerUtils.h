/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/httprequesthandler/IHTTPRequestHandler.h"

#include <stdint.h>
#include <string>

class HTTPRequestHandlerUtils
{
public:
  static std::string GetRequestHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const std::string &key);
  static int GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::map<std::string, std::string> &headerValues);
  static int GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::multimap<std::string, std::string> &headerValues);

  static bool GetRequestedRanges(struct MHD_Connection *connection, uint64_t totalLength, CHttpRanges &ranges);

private:
  HTTPRequestHandlerUtils() = delete;

  static int FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
  static int FillArgumentMultiMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
};
