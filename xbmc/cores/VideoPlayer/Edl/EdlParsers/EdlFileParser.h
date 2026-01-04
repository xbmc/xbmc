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
 * @brief Parser for .edl format files
 *
 * EDL files contain lines with start time, end time, and action:
 * - Times can be HH:MM:SS.sss, frame numbers (#12345), or seconds (123.45)
 * - Actions: 0=cut, 1=mute, 2=scene marker, 3=commercial break
 */
class CEdlFileParser : public CEdlFileParserBase
{
public:
  CEdlParserResult Parse(const CFileItem& item, float fps) override;

protected:
  std::string GetEdlFilePath(const CFileItem& item) const override;
};

} // namespace EDL
