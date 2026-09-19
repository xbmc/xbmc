/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/cheats/CheatUtils.h"

#include <gtest/gtest.h>

using namespace KODI::GAME;

TEST(TestCheatUtils, CheatFileNamePreservesTheExactGameBasename)
{
  EXPECT_EQ(CCheatUtils::GetCheatFileName("/games/Nintendo Game Boy/Frogger (USA).gb"),
            "Frogger (USA).cht");
  EXPECT_EQ(CCheatUtils::GetCheatFileName("/games/Game (Rev. 1).gb"), "Game (Rev. 1).cht");
  EXPECT_EQ(CCheatUtils::GetCheatFileName("game"), "game.cht");
  EXPECT_EQ(CCheatUtils::GetCheatFileName("zip://%2fgames%2fFrogger.zip/Frogger (USA).gb"),
            "Frogger (USA).cht");
}

TEST(TestCheatUtils, RememberedSelectionsDistinguishGamesWithTheSameBasename)
{
  const auto first = CCheatUtils::GetSelectionFileName("/games/gb/Frogger (USA).gb");
  EXPECT_EQ(first, "Frogger (USA).gb_c3ca570b.xml");
  EXPECT_NE(first, CCheatUtils::GetSelectionFileName("/games/other/Frogger (USA).gb"));
  EXPECT_EQ(first.find('/'), std::string::npos);
  EXPECT_TRUE(first.ends_with(".xml"));
}
