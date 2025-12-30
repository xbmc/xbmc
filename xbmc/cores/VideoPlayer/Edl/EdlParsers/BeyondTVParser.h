/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Edl/EdlParser.h"

namespace EDL
{

/*!
 * @brief Parser for Beyond TV chapter files (.<ext>.chapters.xml)
 *
 * Beyond TV creates XML files with commercial break regions.
 * The file extension follows the pattern: video.ext.chapters.xml
 *
 * XML structure:
 * <cutlist>
 *   <Region>
 *     <start comment="HH:MM:SS.sss">timestamp</start>
 *     <end comment="HH:MM:SS.sss">timestamp</end>
 *   </Region>
 * </cutlist>
 *
 * Timestamps need to be divided by 10000 to get milliseconds.
 */
class CBeyondTVParser : public CEdlFileParserBase
{
public:
  CEdlParserResult Parse(const CFileItem& item, float fps) override;

protected:
  std::string GetEdlFilePath(const CFileItem& item) const override;
};

} // namespace EDL
