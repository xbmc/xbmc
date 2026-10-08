/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/GameUtils.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI::GAME;

TEST(TestGameUtils, GamesWithTheSameFileNameHaveTheirOwnFolders)
{
  const std::string folder = CGameUtils::GetGameFolder("/games/gb/Frogger (USA).gb");

  EXPECT_EQ(URIUtils::GetDirectory(folder), "special://profile/games/");
  EXPECT_EQ(URIUtils::GetFileName(folder), "Frogger (USA).gb_c3ca570b");
  EXPECT_NE(folder, CGameUtils::GetGameFolder("/games/other/Frogger (USA).gb"));
}
