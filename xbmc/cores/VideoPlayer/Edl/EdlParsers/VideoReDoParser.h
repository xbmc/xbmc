/*
 *  Copyright (C) 2005-2026 Team Kodi
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
 * @brief Parser for VideoReDo project files (.Vprj)
 *
 * VideoReDo files use XML-like tags but aren't proper XML.
 * Version 2 files are supported with:
 * - <Cut>start:end tags for cut edits (times in 1/10000 ms)
 * - <SceneMarker N>time tags for scene markers
 *
 * @see http://www.videoredo.com/
 */
class CVideoReDoParser : public CEdlFileParserBase
{
public:
  CEdlParserResult Parse(const CFileItem& item, float fps) override;

protected:
  std::string GetEdlFilePath(const CFileItem& item) const override;
};

} // namespace EDL
