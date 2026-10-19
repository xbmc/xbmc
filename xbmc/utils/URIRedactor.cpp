/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URIRedactor.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/StackDirectory.h"
#include "utils/StringUtils.h"
#include "utils/URIPolicy.h"

#include <array>
#include <vector>

namespace
{
struct ResourceEncoding
{
  const ResourceEncoding* parent;
  bool stackEscapes;
};

// Read only the wrapper structure, without allocating a decoded inline data body.
bool ReadResourceCharacter(std::string_view source,
                           size_t& position,
                           const ResourceEncoding* encoding,
                           char& character)
{
  if (!encoding)
  {
    if (position == source.size())
      return false;
    character = source[position++];
    return true;
  }
  if (!ReadResourceCharacter(source, position, encoding->parent, character))
    return false;
  if (encoding->stackEscapes)
  {
    if (character == ',')
    {
      const size_t afterComma = position;
      char next;
      if (!ReadResourceCharacter(source, position, encoding->parent, next) || next != ',')
        position = afterComma;
    }
    return true;
  }
  if (character == '+')
    character = ' ';
  else if (character == '%')
  {
    const size_t afterPercent = position;
    std::array<char, 3> escape{'%', 0, 0};
    if (ReadResourceCharacter(source, position, encoding->parent, escape[1]) &&
        ReadResourceCharacter(source, position, encoding->parent, escape[2]))
    {
      const auto decoded = CURL::Decode(std::string_view(escape.data(), escape.size()));
      if (decoded.size() == 1)
      {
        character = decoded.front();
        return true;
      }
    }
    position = afterPercent;
  }
  return true;
}

bool HasInlineDataResource(std::string_view location, const ResourceEncoding* encoding = nullptr);

bool HasInlineDataInStack(std::string_view body, const ResourceEncoding* encoding = nullptr)
{
  const ResourceEncoding memberEncoding{encoding, true};
  size_t memberStart = 0;
  size_t position = 0;
  std::array<char, 3> separator{};
  std::array<size_t, 3> separatorPositions{};
  while (position < body.size())
  {
    const size_t characterStart = position;
    char character;
    if (!ReadResourceCharacter(body, position, encoding, character))
      break;
    separator = {separator[1], separator[2], character};
    separatorPositions = {separatorPositions[1], separatorPositions[2], characterStart};
    if (separator == std::array<char, 3>{' ', ',', ' '})
    {
      if (HasInlineDataResource(body.substr(memberStart, separatorPositions[0] - memberStart),
                                &memberEncoding))
        return true;
      memberStart = position;
      separator = {};
    }
  }
  return HasInlineDataResource(body.substr(memberStart), &memberEncoding);
}

bool HasInlineDataResource(std::string_view location, const ResourceEncoding* encoding)
{
  size_t position = 0;
  char character;
  std::string prefix;
  while (ReadResourceCharacter(location, position, encoding, character))
  {
    if (std::string_view("/\\?#@ \t\r\n").find(character) != std::string_view::npos)
      return false;
    prefix += character;
    if (character == ':')
      break;
  }
  const auto scheme = URIPolicy::GetScheme(prefix);
  if (!scheme)
    return false;
  if (URIPolicy::GetPolicy(*scheme).redactBody)
    return true;

  CURL wrapper;
  wrapper.SetProtocol(std::string(*scheme));
  const bool stack = wrapper.IsProtocol("stack");
  if (!stack && !wrapper.IsProtocol("image") && !wrapper.HasParentInHostname())
    return false;
  if (!ReadResourceCharacter(location, position, encoding, character) ||
      (character != '/' && character != '\\'))
    return false;
  const char separator = character;
  if (!ReadResourceCharacter(location, position, encoding, character) || character != separator)
    return false;
  if (stack)
    return HasInlineDataInStack(location.substr(position), encoding);

  size_t hostStart = position;
  size_t hostEnd = position;
  bool userInfo = false;
  while (ReadResourceCharacter(location, position, encoding, character))
  {
    if (character == '/' || character == separator || character == '?' || character == '#')
      break;
    if (character == '@' && !userInfo)
    {
      hostStart = position;
      userInfo = true;
    }
    hostEnd = position;
  }
  const ResourceEncoding hostEncoding{encoding, false};
  return HasInlineDataResource(location.substr(hostStart, hostEnd - hostStart), &hostEncoding);
}
} // namespace

std::string CURIRedactor::GetWithoutUserDetails(const CURL& url, bool redact)
{
  if (redact && (url.m_uri ? HasInlineDataResource(url.m_uri->Get())
                           : (url.IsProtocol("stack")
                                  ? HasInlineDataInStack(url.GetFileName())
                                  : ((url.IsProtocol("image") || url.HasParentInHostname()) &&
                                     HasInlineDataResource(url.GetHostName())))))
    return url.GetProtocol() + ":[REDACTED]";
  if (url.m_uri)
  {
    const CURI& uri = *url.m_uri;
    if (redact && URIPolicy::UsesLegacyURL(uri.Get()) &&
        (url.IsProtocol("stack") || url.IsProtocol("image") || url.HasParentInHostname()))
      return GetWithoutUserDetails(CURL(uri.Get()), true);
    if (!uri.HasAuthority() || URIPolicy::GetPolicy(url.GetProtocol()).redactBody)
      return uri.Get();
    const auto authority = uri.GetRawAuthority();
    const auto at = authority.rfind('@');
    if (at == std::string_view::npos)
      return uri.Get();
    std::string result = uri.Get().substr(0, uri.GetComponents().authority->offset);
    if (redact)
      result += authority.substr(0, at).find(':') == std::string_view::npos ? "USERNAME@"
                                                                            : "USERNAME:PASSWORD@";
    result += uri.Get().substr(uri.GetComponents().authority->offset + at + 1);
    return result;
  }
  std::string strURL;

  if (url.IsProtocol("stack"))
  {
    CFileItemList items;
    XFILE::CStackDirectory dir;
    dir.GetDirectory(url, items);
    std::vector<std::string> newItems;
    for (int i = 0; i < items.Size(); ++i)
    {
      CURL member(items[i]->GetPath());
      items[i]->SetPath(GetWithoutUserDetails(member, redact));
      newItems.push_back(items[i]->GetPath());
    }
    dir.ConstructStackPath(newItems, strURL);
    return strURL;
  }

  unsigned int sizeneed = url.GetProtocol().length() + url.GetHostName().length() +
                          url.GetFileName().length() + url.GetOptions().length() +
                          url.GetProtocolOptions().length() + 10;

  if (redact && !url.GetUserName().empty())
  {
    sizeneed += sizeof("USERNAME");
    if (!url.GetPassWord().empty())
      sizeneed += sizeof(":PASSWORD@");
    if (!url.GetDomain().empty())
      sizeneed += sizeof("DOMAIN;");
  }

  strURL.reserve(sizeneed);

  if (url.GetProtocol().empty())
    return url.GetFileName();

  strURL = url.GetProtocol();
  strURL += "://";

  if (redact && !url.GetUserName().empty())
  {
    if (!url.GetDomain().empty())
      strURL += "DOMAIN;";
    strURL += "USERNAME";
    if (!url.GetPassWord().empty())
      strURL += ":PASSWORD";
    strURL += "@";
  }

  if (!url.GetHostName().empty())
  {
    std::string strHostName;

    if (url.HasParentInHostname() || url.IsProtocol("image"))
    {
      const CURL parent(url.GetHostName());
      strHostName = GetWithoutUserDetails(parent, false);
      // Keep image resource spelling when no credentials were removed.
      if (url.IsProtocol("image") && strHostName == parent.Get())
        strHostName = url.GetHostName();
    }
    else
      strHostName = url.GetHostName();

    if (url.HasEncodedHostname())
      strHostName = CURL::Encode(strHostName);

    if (url.HasPort())
    {
      if (strHostName.find(':') != strHostName.rfind(':'))
        strHostName = '[' + strHostName + ']';
      strURL += strHostName + StringUtils::Format(":{}", url.GetPort());
    }
    else
      strURL += strHostName;

    strURL += "/";
  }
  strURL += url.GetFileName();

  if (!url.GetOptions().empty())
    strURL += url.GetOptions();
  if (!url.GetProtocolOptions().empty())
    strURL += "|" + url.GetProtocolOptions();

  return strURL;
}

std::string CURIRedactor::GetRedacted(std::string_view path)
{
  const auto scheme = URIPolicy::GetScheme(path);
  if (scheme && HasInlineDataResource(path))
    return StringUtils::ToLower(std::string(*scheme)) + ":[REDACTED]";
  return GetWithoutUserDetails(CURL(std::string(path)), true);
}
