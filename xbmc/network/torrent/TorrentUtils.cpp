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

using namespace KODI;
using namespace NETWORK;

std::pair<std::string, std::string> CTorrentUtils::SplitMagnetURL(const CURL& url)
{
  std::string filename = url.GetFileName();

  CURL urlCopy(url);
  urlCopy.SetFileName("");
  std::string magnetUri = urlCopy.Get();

  // Remove any protocol slashes from the magnet URI
  if (StringUtils::StartsWith(magnetUri, "magnet://"))
    magnetUri.erase(std::strlen("magnet:"), 2);

  return std::make_pair(magnetUri, filename);
}
