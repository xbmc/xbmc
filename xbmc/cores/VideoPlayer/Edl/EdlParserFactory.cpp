/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EdlParserFactory.h"

#include "EdlParsers/BeyondTVParser.h"
#include "EdlParsers/ComskipParser.h"
#include "EdlParsers/EdlFileParser.h"
#include "EdlParsers/PvrEdlParser.h"
#include "EdlParsers/VideoReDoParser.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

using namespace EDL;

std::vector<std::unique_ptr<IEdlParser>> CEdlParserFactory::GetEdlParsersForItem(
    const CFileItem& item)
{
  std::vector<std::unique_ptr<IEdlParser>> parsers;

  const std::string& mediaFilePath = item.GetDynPath();

  // Check if item is on local drive or network share
  const bool isLocalOrLan = (URIUtils::IsHD(mediaFilePath) ||
                             URIUtils::IsOnLAN(mediaFilePath, LanCheckMode::ANY_PRIVATE_SUBNET)) &&
                            !URIUtils::IsInternetStream(mediaFilePath);

  if (isLocalOrLan)
  {
    // File-based parsers for local/LAN items
    parsers.emplace_back(std::make_unique<CVideoReDoParser>());
    parsers.emplace_back(std::make_unique<CEdlFileParser>());
    parsers.emplace_back(std::make_unique<CComskipParser>());
    parsers.emplace_back(std::make_unique<CBeyondTVParser>());
  }
  else
  {
    // Metadata-based parsers for other items (PVR, streams, etc.)
    parsers.emplace_back(std::make_unique<CPvrEdlParser>());
  }

  return parsers;
}
