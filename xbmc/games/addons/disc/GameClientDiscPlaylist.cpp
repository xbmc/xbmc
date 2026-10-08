/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscPlaylist.h"

#include "games/GameUtils.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto DISC_STATE_NAME = "discstate";
} // namespace

std::string CGameClientDiscPlaylist::GetStateFilePath(const std::string& gamePath,
                                                      std::string_view extension)
{
  return URIUtils::AddFileToFolder(CGameUtils::GetGameFolder(gamePath),
                                   DISC_STATE_NAME + std::string{extension});
}
