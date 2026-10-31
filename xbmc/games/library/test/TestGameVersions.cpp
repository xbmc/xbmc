/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/library/GameNameParser.h"
#include "games/library/GameVersions.h"
#include "games/library/ReleasePolicy.h"

#include <initializer_list>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
std::vector<ParsedGameName> Names(std::initializer_list<std::string_view> files)
{
  std::vector<ParsedGameName> names;
  for (std::string_view file : files)
    names.emplace_back(CGameNameParser::Parse(file));
  return names;
}

CReleasePolicy PolicyFor(std::string_view region)
{
  return CReleasePolicy(CReleasePolicy::RegionPriority({GameRegion::Make(region)}));
}

using Groups = std::vector<std::vector<size_t>>;
} // namespace

TEST(TestGameVersions, ShowsTheVersionForThePlayersRegion)
{
  const auto names = Names({"Super Mario Bros. 3 (USA).nes", "Super Mario Bros. 3 (Europe).nes",
                            "Super Mario Bros. 3 (Japan).nes"});
  EXPECT_EQ(CGameVersions::Group(names, PolicyFor("GB")), (Groups{{1, 0, 2}}));
  EXPECT_EQ(CGameVersions::Group(names, PolicyFor("US")), (Groups{{0, 1, 2}}));
  EXPECT_EQ(CGameVersions::Group(names, PolicyFor("JP")), (Groups{{2, 1, 0}}));
}

TEST(TestGameVersions, KeepsDifferentGamesApart)
{
  const auto names = Names({"Tetris (World).gb", "Dr. Mario (World).gb"});
  EXPECT_TRUE(CGameVersions::Group(names, PolicyFor("GB")).empty());
}

TEST(TestGameVersions, KeepsTheSameNameOnAnotherPlatformApart)
{
  const auto names = Names({"Tetris (World).gb", "Tetris (USA).nes", "Tetris (Europe).nes"});
  EXPECT_EQ(CGameVersions::Group(names, PolicyFor("GB")), (Groups{{2, 1}}));
}

TEST(TestGameVersions, GroupsEachDiscOnItsOwn)
{
  const auto names =
      Names({"Final Fantasy VII (USA) (Disc 1).cue", "Final Fantasy VII (Europe) (Disc 1).cue",
             "Final Fantasy VII (Europe) (Disc 2).cue"});
  EXPECT_EQ(CGameVersions::Group(names, PolicyFor("GB")), (Groups{{1, 0}}));
}

TEST(TestGameVersions, KeepsTheSidesOfATapeApart)
{
  const auto names = Names({"Game (1985)(Publisher)(Tape 2 of 2 Side A).tap",
                            "Game (1985)(Publisher)(Tape 2 of 2 Side B).tap",
                            "Another Game (1986)(Publisher)(Tape 2)(Side A).tap",
                            "Another Game (1986)(Publisher)(Tape 2)(Side B).tap",
                            "Other Game (Europe) (Tape 1) (Side B).tap",
                            "Other Game (Europe) (Tape 2) (Side B).tap"});
  EXPECT_TRUE(CGameVersions::Group(names, PolicyFor("GB")).empty());
}

TEST(TestGameVersions, KeepsADataDiskApartFromItsGame)
{
  const auto names = Names({"Ashe (Game disk).d88", "Ashe (Data disk).d88"});
  EXPECT_TRUE(CGameVersions::Group(names, PolicyFor("JP")).empty());
}

TEST(TestGameVersions, ShowsAFinishedGoodDumpFirst)
{
  const auto names =
      Names({"Sonic the Hedgehog (Europe) (Beta).md", "Sonic the Hedgehog (USA, Europe) [b].md",
             "Sonic the Hedgehog (USA, Europe).md"});
  EXPECT_EQ(CGameVersions::Group(names, PolicyFor("GB")), (Groups{{2, 0, 1}}));
}

TEST(TestGameVersions, ReadsTheVersionFromTheName)
{
  const GameRelease hack =
      CGameVersions::Release(CGameNameParser::Parse("Super Mario World (USA) (Rev 1) [h1].sfc"));
  EXPECT_TRUE(hack.modified);
  EXPECT_EQ(hack.revision, "Rev 1");
  ASSERT_EQ(hack.regions.size(), 1u);
  EXPECT_EQ(hack.regions[0].ToString(), "US");

  EXPECT_EQ(CGameVersions::Release(CGameNameParser::Parse("Tetris (World) [!].gb")).dump,
            DumpStatus::VERIFIED);
  EXPECT_EQ(CGameVersions::Release(CGameNameParser::Parse("Tetris (World) [b].gb")).dump,
            DumpStatus::BAD);
}
