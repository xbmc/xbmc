/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Edl/EdlParser.h"

#include <cstdint>

struct EpisodeInformation;
using EpisodeFileMap = std::multimap<std::string, EpisodeInformation, std::less<>>;

namespace EDL
{

/*!
 * @brief Parser for multi-episode files.
 *
 * Uses episode bookmarks from the video database to create EDL cuts for the episode being played.
 * This parser implements IEdlParser directly (not CEdlFileParserBase)
 * as it doesn't read from files.
 */
class CMultipleEpisodeEdlParser : public IEdlParser
{
public:
  bool CanParse(const CFileItem& item) const override;
  CEdlParserResult Parse(const CFileItem& item, float fps, int64_t duration) override;
  static CEdlParserResult Process(const CFileItem& item,
                                  float fps,
                                  int64_t duration,
                                  const EpisodeFileMap& fileMap);
};

} // namespace EDL
