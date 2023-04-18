/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPRequestHandlerUtils.h"

#include "utils/StringUtils.h"

#include <map>

std::string HTTPRequestHandlerUtils::GetRequestHeaderValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, const std::string &key)
{
  if (connection == nullptr)
    return "";

  const char* value = MHD_lookup_connection_value(connection, kind, key.c_str());
  if (value == nullptr)
    return "";

  if (StringUtils::EqualsNoCase(key, MHD_HTTP_HEADER_CONTENT_TYPE))
  {
    // Work around a bug in firefox (see https://bugzilla.mozilla.org/show_bug.cgi?id=416178)
    // by cutting of anything that follows a ";" in a "Content-Type" header field
    std::string strValue(value);
    size_t pos = strValue.find(';');
    if (pos != std::string::npos)
      strValue.resize(pos);

    return strValue;
  }

  return value;
}

int HTTPRequestHandlerUtils::GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::map<std::string, std::string> &headerValues)
{
  if (connection == nullptr)
    return -1;

  return MHD_get_connection_values(connection, kind, FillArgumentMap, &headerValues);
}

int HTTPRequestHandlerUtils::GetRequestHeaderValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, std::multimap<std::string, std::string> &headerValues)
{
  if (connection == nullptr)
    return -1;

  return MHD_get_connection_values(connection, kind, FillArgumentMultiMap, &headerValues);
}

bool HTTPRequestHandlerUtils::GetRequestedRanges(struct MHD_Connection *connection, uint64_t totalLength, CHttpRanges &ranges)
{
  ranges.Clear();

  if (connection == nullptr)
    return false;

  return ranges.Parse(GetRequestHeaderValue(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_RANGE), totalLength);
}

MHD_RESULT HTTPRequestHandlerUtils::FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
  if (cls == nullptr || key == nullptr)
    return MHD_NO;

  std::map<std::string, std::string> *arguments = reinterpret_cast<std::map<std::string, std::string>*>(cls);
  arguments->insert(std::make_pair(key, value != nullptr ? value : ""));

  return MHD_YES;
}

MHD_RESULT HTTPRequestHandlerUtils::FillArgumentMultiMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
  if (cls == nullptr || key == nullptr)
    return MHD_NO;

  std::multimap<std::string, std::string> *arguments = reinterpret_cast<std::multimap<std::string, std::string>*>(cls);
  arguments->insert(std::make_pair(key, value != nullptr ? value : ""));

  return MHD_YES;
}
