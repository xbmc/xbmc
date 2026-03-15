/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscPlaylist.h"

#include "Util.h"
#include "utils/Crc32.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto PROFILE_ROOT = "special://masterprofile";
constexpr auto DISC_STATE_DIRECTORY = "games/discstate";
} // namespace

std::string CGameClientDiscPlaylist::GetDiscStateDirectory()
{
  return URIUtils::AddFileToFolder(PROFILE_ROOT, DISC_STATE_DIRECTORY);
}

std::string CGameClientDiscPlaylist::GetSafeBaseName(const std::string& gamePath)
{
  return CUtil::MakeLegalFileName(URIUtils::GetFileName(gamePath));
}

std::string CGameClientDiscPlaylist::GetStateSubdirectory(const std::string& gamePath)
{
  // Include a CRC of the full game path so identical filenames in different
  // locations do not clash
  return StringUtils::Format("{}_{:08x}", GetSafeBaseName(gamePath), Crc32::Compute(gamePath));
}

std::string CGameClientDiscPlaylist::GetStateFilePath(const std::string& gamePath,
                                                      std::string_view extension)
{
  const std::string safeFileName =
      GetSafeBaseName(URIUtils::ReplaceExtension(gamePath, std::string{extension}));
  return URIUtils::AddFileToFolder(GetDiscStateDirectory(), GetStateSubdirectory(gamePath),
                                   safeFileName);
}
