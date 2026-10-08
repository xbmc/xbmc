/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/SpecialProtocol.h"
#include "games/GameUtils.h"
#include "utils/URIUtils.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI::GAME;

TEST(TestGameUtils, GamesWithTheSameFileNameHaveTheirOwnFolders)
{
  const std::string folder = CGameUtils::GetGameFolder("/games/gb/Frogger (USA).gb");

  EXPECT_EQ(URIUtils::GetDirectory(folder), "special://profile/games/");
  EXPECT_EQ(URIUtils::GetFileName(folder), "Frogger (USA).gb_c3ca570b");
  EXPECT_NE(folder, CGameUtils::GetGameFolder("/games/other/Frogger (USA).gb"));
}

TEST(TestGameUtils, EveryFormOfAGamesPathFindsTheSameFolder)
{
  const std::string special = "special://home/games/Frogger (USA).gb";

  EXPECT_EQ(CGameUtils::GetGameFolder(special),
            CGameUtils::GetGameFolder(CSpecialProtocol::TranslatePath(special)));
  EXPECT_EQ(CGameUtils::GetGameFolder("file:///games/gb/Frogger (USA).gb"),
            CGameUtils::GetGameFolder("/games/gb/Frogger (USA).gb"));
  EXPECT_EQ(CGameUtils::GetGameFolder("smb://user:secret@nas/games/Frogger (USA).gb"),
            CGameUtils::GetGameFolder("smb://nas/games/Frogger (USA).gb"));
}

TEST(TestGameUtils, ALongNameStaysWithinTheFileSystemLimit)
{
  // Three bytes a character in UTF-8, so the cut has to land between them
  std::string title;
  for (int i = 0; i < 120; ++i)
    title += "\xe3\x81\x82";

  const std::string first =
      URIUtils::GetFileName(CGameUtils::GetGameFolder("/games/" + title + "1.d88"));
  const std::string second =
      URIUtils::GetFileName(CGameUtils::GetGameFolder("/games/" + title + "2.d88"));

  EXPECT_LE(first.size(), 255u);
  EXPECT_EQ((first.size() - 9) % 3, 0u);
  EXPECT_NE(first, second);
}
