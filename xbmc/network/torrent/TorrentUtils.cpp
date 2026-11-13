/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TorrentUtils.h"

#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace NETWORK;

std::pair<std::string, std::string> CTorrentUtils::SplitMagnetURL(const CURL& url)
{
  const std::string path = url.GetRawPath();
  CURL urlCopy(url);
  if (!url.IsProtocol("magnet") || (!path.empty() && !url.SupportsPathOperations()) ||
      !urlCopy.SetURIPath(""))
    return {};

  std::string filename;
  if (!path.empty())
    filename = URIUtils::DecodePathEscapes(std::string_view(path).substr(1));
#ifdef TARGET_WINDOWS
  StringUtils::Replace(filename, '/', '\\');
#endif

  return {"magnet:" + urlCopy.GetSchemeSpecificPart(), std::move(filename)};
}

bool CTorrentUtils::SetMagnetPath(CURL& url, std::string path)
{
#ifdef TARGET_WINDOWS
  StringUtils::Replace(path, '\\', '/');
#endif
  return url.IsProtocol("magnet") && url.SetURIPath("/" + URIUtils::URLEncode(path, "/-._~"));
}
