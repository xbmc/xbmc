/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URIPolicy.h"

#include "URI.h"
#include "URIParser.h"

#include <algorithm>

namespace URIPolicy
{
std::optional<std::string_view> GetScheme(std::string_view input)
{
  auto scheme = URIParser::GetScheme(input);
  if (scheme && scheme->size() == 1)
  {
#ifndef TARGET_WINDOWS
    if (input.substr(2, 2) == "//")
      return scheme;
#endif
    return std::nullopt;
  }
  return scheme;
}

bool IsURILocation(std::string_view input)
{
  const auto scheme = GetScheme(input);
  if (!scheme)
    return false;

  const auto prefix = input.substr(scheme->size() + 1, 2);
  return prefix == "//" || prefix == "\\\\" || SchemeEquals(*scheme, "file") ||
         SchemeEquals(*scheme, "http") || SchemeEquals(*scheme, "https") ||
         SchemeEquals(*scheme, "magnet") || SchemeEquals(*scheme, "data") ||
         SchemeEquals(*scheme, "urn") || SchemeEquals(*scheme, "mailto");
}

bool SchemeEquals(std::string_view left, std::string_view right)
{
  return std::ranges::equal(left, right,
                            [](char a, char b)
                            {
                              auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; };
                              return lower(a) == lower(b);
                            });
}

Policy GetPolicy(std::string_view scheme)
{
  if (SchemeEquals(scheme, "data"))
    return {false, true, false, true, false};
  if (SchemeEquals(scheme, "urn"))
    return {};
  if (SchemeEquals(scheme, "magnet"))
    return {true, true, false, false, false};
  if (scheme.empty())
    return {true, false, false, false, false};

  // Established schemes also permit path operations in authority-free URI form.
  constexpr std::string_view filesystemSchemes[] = {
      "http",       "https",    "ftp",     "ftps",        "sftp",     "ssh",       "smb",
      "nfs",        "afp",      "dav",     "davs",        "shout",    "shouts",    "rss",
      "rsss",       "rtsp",     "rtmp",    "rtmpt",       "rtmpe",    "rtmpte",    "rtmps",
      "mms",        "mmst",     "mmsh",    "udp",         "rtp",      "tcp",       "sdp",
      "upnp",       "file",     "special", "resource",    "stack",    "multipath", "virtualpath",
      "zip",        "rar",      "apk",     "xbt",         "archive",  "image",     "udf",
      "iso9660",    "bluray",   "cdda",    "dvd",         "plugin",   "script",    "addons",
      "sources",    "musicdb",  "videodb", "musicsearch", "pvr",      "library",   "favourites",
      "androidapp", "episodes", "pipe",    "win-lib",     "zeroconf", "events",    "sacd"};
  if (std::ranges::any_of(filesystemSchemes,
                          [scheme](auto candidate) { return SchemeEquals(scheme, candidate); }))
    return {true, false, true, false, true};
  // Third-party VFS protocols must retain their legacy projection before add-on startup.
  return {false, true, true, false, scheme.size() > 1};
}

bool IsLegacyURL(std::string_view input)
{
  const auto scheme = GetScheme(input);
  return scheme && input.substr(scheme->size() + 1, 2) == "//";
}

bool UsesLegacyURL(std::string_view input)
{
  const auto scheme = GetScheme(input);
  if (!scheme || (!GetPolicy(*scheme).legacy && scheme->size() != 1))
    return false;
  const auto prefix = input.substr(scheme->size() + 1, 2);
  return prefix == "//" || prefix == "\\\\";
}

bool SupportsPathOperations(const CURI& uri)
{
  if (!uri.IsValid() || !GetPolicy(uri.GetRawScheme()).filesystem)
    return false;
  return !SchemeEquals(uri.GetRawScheme(), "magnet") ||
         (!uri.HasAuthority() && uri.GetRawPath().starts_with('/'));
}

bool CanSetPath(const CURI& uri, std::string_view path)
{
  if (!uri.IsValid() || !GetPolicy(uri.GetRawScheme()).filesystem)
    return false;
  return !SchemeEquals(uri.GetRawScheme(), "magnet") ||
         (!uri.HasAuthority() && (path.empty() || path.starts_with('/')));
}
} // namespace URIPolicy
