/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Edl/EdlParser.h"

#include <chrono>

namespace EDL
{

/*!
 * @brief Parser for media with chapters.
 *
 * Uses chapters from the url to create EDL cuts for the episode being played.
 * This parser implements IEdlParser directly (not CEdlFileParserBase)
 * as it doesn't read from files.
 */
class CChapterEdlParser : public IEdlParser
{
public:
  bool CanParse(const CFileItem& item) const override;
  CEdlParserResult Parse(const CFileItem& item,
                         float fps,
                         std::chrono::milliseconds duration) override;
};

} // namespace EDL
