/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CheatUtils.h"

#include "Util.h"
#include "utils/Crc32.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace KODI::GAME;

std::string CCheatUtils::GetCheatFileName(const std::string& gamePath)
{
  return URIUtils::ReplaceExtension(URIUtils::GetFileName(gamePath), ".cht");
}

std::string CCheatUtils::GetSelectionFileName(const std::string& gamePath)
{
  return StringUtils::Format("{}_{:08x}.xml",
                             CUtil::MakeLegalFileName(URIUtils::GetFileName(gamePath)),
                             Crc32::Compute(gamePath));
}
