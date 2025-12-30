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
 * @brief Parser for Comskip format files (.txt)
 *
 * Comskip files contain frame-based commercial break markers with a header
 * indicating processing completion and frame rate. Format:
 * - Line 1: "FILE PROCESSING COMPLETE <frames> FRAMES AT <fps*100>"
 * - Line 2: "-------------"
 * - Lines 3+: "<start_frame> <end_frame>"
 */
class CComskipParser : public CEdlFileParserBase
{
public:
  CEdlParserResult Parse(const CFileItem& item, float fps) override;

protected:
  std::string GetEdlFilePath(const CFileItem& item) const override;
};

} // namespace EDL
