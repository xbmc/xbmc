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
 * @brief Parser for PVR EDL data
 *
 * Retrieves edit decision list data from PVR recordings and EPG items.
 * This parser implements IEdlParser directly (not CEdlFileParserBase)
 * as it doesn't read from files.
 */
class CPvrEdlParser : public IEdlParser
{
public:
  bool CanParse(const CFileItem& item) const override;
  CEdlParserResult Parse(const CFileItem& item, float fps) override;
};

} // namespace EDL
