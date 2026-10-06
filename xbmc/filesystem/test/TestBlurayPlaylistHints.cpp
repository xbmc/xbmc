/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/IPlaylistHints.h"
#include "filesystem/bluray/BlurayPlaylistHints.h"
#include "filesystem/bluray/ProjectParser.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
ProjectInformation MakeProject(const std::vector<std::pair<unsigned int, std::string>>& named)
{
  ProjectInformation project;
  project.present = true;
  for (const auto& [playlist, name] : named)
    project.playlists.emplace(playlist,
                              ProjectPlaylistInformation{.playlist = playlist, .name = name});
  return project;
}
} // namespace

TEST(TestBlurayPlaylistHints, RolesFollowTheNamingConvention)
{
  EXPECT_EQ(GetProjectPlaylistRole("FPL_MainFeature"), PlaylistRole::FEATURE);
  EXPECT_EQ(GetProjectPlaylistRole("FPL_MainFeature_EXT_Narrative"), PlaylistRole::FEATURE);
  EXPECT_EQ(GetProjectPlaylistRole("SEG FPL_MainFeature"), PlaylistRole::FEATURE);
  EXPECT_EQ(GetProjectPlaylistRole("SEG_MainFeature"), PlaylistRole::FEATURE);
  EXPECT_EQ(GetProjectPlaylistRole("SEG_MainFeature_01"), PlaylistRole::UNKNOWN);

  EXPECT_EQ(GetProjectPlaylistRole("EPL_01"), PlaylistRole::EPISODE);
  EXPECT_EQ(GetProjectPlaylistRole("SEG_EPL_02"), PlaylistRole::EPISODE);

  EXPECT_EQ(GetProjectPlaylistRole("SF_Inside_Derry_102"), PlaylistRole::SPECIAL);
  EXPECT_EQ(GetProjectPlaylistRole("SEG_SF_BTV_Power_Of_Sound"), PlaylistRole::SPECIAL);
  EXPECT_EQ(GetProjectPlaylistRole("SEG SF_01_Finding"), PlaylistRole::SPECIAL);

  EXPECT_EQ(GetProjectPlaylistRole("WRN_Piracy"), PlaylistRole::FRONT_MATTER);
  EXPECT_EQ(GetProjectPlaylistRole("Studio_Logo"), PlaylistRole::FRONT_MATTER);
  EXPECT_EQ(GetProjectPlaylistRole("MPAA"), PlaylistRole::FRONT_MATTER);

  EXPECT_EQ(GetProjectPlaylistRole("MainMenu_BG"), PlaylistRole::MENU);
  EXPECT_EQ(GetProjectPlaylistRole("TMPL_Transition"), PlaylistRole::MENU);

  EXPECT_EQ(GetProjectPlaylistRole("Something_Else"), PlaylistRole::UNKNOWN);
}

TEST(TestBlurayPlaylistHints, EpisodesCarryTheirOrdinalAndPresentation)
{
  const CBlurayPlaylistHints hints{MakeProject(
      {{800u, "EPL_01"}, {801u, "EPL_01_Narrative"}, {802u, "SEG_EPL_12"}, {803u, "EPL_Bonus"}})};

  EXPECT_TRUE(hints.HasHints());
  const PlaylistHintMap& map{hints.GetHints()};

  ASSERT_TRUE(map.at(800u).ordinal.has_value());
  EXPECT_EQ(*map.at(800u).ordinal, 1u);
  EXPECT_TRUE(map.at(800u).basePresentation);

  ASSERT_TRUE(map.at(801u).ordinal.has_value());
  EXPECT_EQ(*map.at(801u).ordinal, 1u);
  EXPECT_FALSE(map.at(801u).basePresentation);

  ASSERT_TRUE(map.at(802u).ordinal.has_value());
  EXPECT_EQ(*map.at(802u).ordinal, 12u);

  // An episode the disc does not number offers nothing to match on
  EXPECT_EQ(map.at(803u).role, PlaylistRole::EPISODE);
  EXPECT_FALSE(map.at(803u).ordinal.has_value());
}

TEST(TestBlurayPlaylistHints, FeaturesCarryBasePresentation)
{
  const CBlurayPlaylistHints hints{MakeProject({{700u, "FPL_MainFeature"},
                                                {701u, "FPL_MainFeature_EXT"},
                                                {702u, "FPL_MainFeature_Narrative"}})};

  const PlaylistHintMap& map{hints.GetHints()};
  EXPECT_TRUE(map.at(700u).basePresentation);
  EXPECT_FALSE(map.at(701u).basePresentation);
  EXPECT_FALSE(map.at(702u).basePresentation);
}

TEST(TestBlurayPlaylistHints, SpecialsAreTitledInWords)
{
  const CBlurayPlaylistHints hints{
      MakeProject({{820u, "SF_Inside_Derry_102"}, {821u, "SEG_SF_Becoming_Pennywise"}})};

  const PlaylistHintMap& map{hints.GetHints()};
  EXPECT_EQ(map.at(820u).name, "SF_Inside_Derry_102");
  EXPECT_EQ(map.at(820u).title, "Inside Derry 102");
  EXPECT_EQ(map.at(821u).title, "Becoming Pennywise");
}

TEST(TestBlurayPlaylistHints, ExtrasAreNamedWithoutTheDiscsCodes)
{
  struct Extra
  {
    std::string name;
    std::string title;
    ExtraGroup group{ExtraGroup::NONE};
    bool playAll{false};
  };

  // Names as found on discs
  const std::vector<Extra> extras{
      {.name = "SF_01_DayZero", .title = "Day Zero"},
      {.name = "SF_01_Day", .title = "Day"},
      {.name = "SF_Trailer1", .title = "Trailer 1"},
      {.name = "SF_02_90s_Action_Reimagined_NCR", .title = "90s Action Reimagined"},
      {.name = "SF_02_The_Anatomy_Of_A_Crash", .title = "The Anatomy Of A Crash"},
      {.name = "SF_05_Prod_05_02VideoGraphics", .title = "Prod Video Graphics"},
      {.name = "SF_06_PostProd_03_DSMontage", .title = "Post Prod DS Montage"},
      {.name = "SF_03_25YearsLater", .title = "25 Years Later"},
      {.name = "SF_06_06_BTS_1912Morph",
       .title = "1912 Morph",
       .group = ExtraGroup::BEHIND_THE_SCENES},
      {.name = "SF_CH3_SL12_InterviewCameron", .title = "Interview Cameron"},
      {.name = "SF_BTS", .group = ExtraGroup::BEHIND_THE_SCENES},
      {.name = "SF_05_BTS_HostedByJonLandau",
       .title = "Hosted By Jon Landau",
       .group = ExtraGroup::BEHIND_THE_SCENES},
      {.name = "SF_CAST_01_01_ChrisPratt", .title = "Chris Pratt", .group = ExtraGroup::CAST},
      {.name = "SF_DS_06_02_TakeOffShoes",
       .title = "Take Off Shoes",
       .group = ExtraGroup::DELETED_SCENES},
      {.name = "SF_DS_01", .title = "01", .group = ExtraGroup::DELETED_SCENES},
      {.name = "SF_MV_10_01_Toretto", .title = "Toretto", .group = ExtraGroup::MUSIC_VIDEOS},
      {.name = "SF_TRLR_15_04_Ride", .title = "Ride", .group = ExtraGroup::TRAILERS},
      {.name = "SF_COMM_05_01_LetRun", .title = "Let Run", .group = ExtraGroup::COMMERCIALS},
      {.name = "SF_PROMO_10_00_PlayAll", .group = ExtraGroup::PROMOS, .playAll = true},
      {.name = "SF_DS_PlayAll", .group = ExtraGroup::DELETED_SCENES, .playAll = true},
      {.name = "SF_02_DS_PLAYALL", .group = ExtraGroup::DELETED_SCENES, .playAll = true},
      {.name = "SF_MakingOf_PlayAll", .title = "Making Of", .playAll = true},
      {.name = "SF_101_IgnitingPlayAll_NCR", .title = "Igniting", .playAll = true},
      {.name = "SF_03_NE_00_PlayAll", .title = "NE", .playAll = true},
  };

  std::vector<std::pair<unsigned int, std::string>> named;
  for (unsigned int playlist{800}; const Extra& extra : extras)
    named.emplace_back(playlist++, extra.name);
  const CBlurayPlaylistHints hints{MakeProject(named)};

  for (unsigned int playlist{800}; const Extra& extra : extras)
  {
    const PlaylistHint& hint{hints.GetHints().at(playlist++)};
    EXPECT_EQ(hint.extraTitle, extra.title) << extra.name;
    EXPECT_EQ(hint.extraGroup, extra.group) << extra.name;
    EXPECT_EQ(hint.playAll, extra.playAll) << extra.name;
    EXPECT_TRUE(hint.basePresentation) << extra.name;
  }
}

TEST(TestBlurayPlaylistHints, CopiesOfAnExtraShareItsTitle)
{
  // A segment, the same with a slate, dubbed or subtitled
  const std::vector<std::pair<std::string, std::string>> copies{
      {"SEG SF_01_Finding", "Finding"},
      {"SEG_SF_LU_02_01_Inspiration", "Inspiration"},
      {"SF_01_Finding_JPN", "Finding"},
      {"SF_01_Reinventing_Modern_Action_Hero_NCR_JPN", "Reinventing Modern Action Hero"},
      {"SF_CAST_01_01_ChrisPratt_Binge", "Chris Pratt"},
      {"SEG_SF_02_DS_01_Raptors_SLATE", "Raptors"},
      {"SF_Gallery_Photographs_Carpenters_M_nld", "Gallery Photographs Carpenters"},
      {"SF_Gallery_Photographs_Carpenters_M_spa_CS", "Gallery Photographs Carpenters"},
      {"SF_Feat_48_lang3", "Feat"},
  };

  std::vector<std::pair<unsigned int, std::string>> named;
  for (unsigned int playlist{800}; const auto& [name, title] : copies)
    named.emplace_back(playlist++, name);
  const CBlurayPlaylistHints hints{MakeProject(named)};

  for (unsigned int playlist{800}; const auto& [name, title] : copies)
  {
    const PlaylistHint& hint{hints.GetHints().at(playlist++)};
    EXPECT_EQ(hint.extraTitle, title) << name;
    EXPECT_FALSE(hint.basePresentation) << name;
  }
}

TEST(TestBlurayPlaylistHints, ADiscWithoutAProjectOffersNothing)
{
  const CBlurayPlaylistHints hints{ProjectInformation{}};
  EXPECT_FALSE(hints.HasHints());
  EXPECT_TRUE(hints.GetHints().empty());
}
