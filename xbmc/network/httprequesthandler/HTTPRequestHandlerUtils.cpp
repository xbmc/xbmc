/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPRequestHandlerUtils.h"

#include "utils/StringUtils.h"

#include <algorithm>
#include <map>

namespace
{
constexpr const char* HTTPSchemeInsecure = "http";
constexpr const char* HTTPSchemeSecure = "https";
constexpr const char* HTTPHeaderForwardedProtocol = "X-Forwarded-Proto";
} // namespace

std::string HTTPRequestHandlerUtils::GetRequestScheme(struct MHD_Connection* connection)
{
  if (connection == nullptr)
    return HTTPSchemeInsecure;

  std::string forwarded =
      GetRequestHeaderValue(connection, MHD_HEADER_KIND, HTTPHeaderForwardedProtocol);
  if (!forwarded.empty())
  {
    // a chain of proxies appends to the header, so the client's own scheme comes first
    forwarded.resize(std::min(forwarded.find(','), forwarded.size()));
    StringUtils::Trim(forwarded);
    StringUtils::ToLower(forwarded);

    // the header is whatever the client sent, so anything unrecognised is dropped
    if (forwarded == HTTPSchemeInsecure || forwarded == HTTPSchemeSecure)
      return forwarded;
  }

  // MHD only answers this for a connection it is serving over TLS
  if (MHD_get_connection_info(connection, MHD_CONNECTION_INFO_PROTOCOL) != nullptr)
    return HTTPSchemeSecure;

  return HTTPSchemeInsecure;
}

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
