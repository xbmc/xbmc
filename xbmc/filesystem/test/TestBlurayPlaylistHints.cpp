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

TEST(TestBlurayPlaylistHints, SpecialsAreTitledInWords)
{
  const CBlurayPlaylistHints hints{
      MakeProject({{820u, "SF_Inside_Derry_102"}, {821u, "SEG_SF_Becoming_Pennywise"}})};

  const PlaylistHintMap& map{hints.GetHints()};
  EXPECT_EQ(map.at(820u).name, "SF_Inside_Derry_102");
  EXPECT_EQ(map.at(820u).title, "Inside Derry 102");
  EXPECT_EQ(map.at(821u).title, "Becoming Pennywise");
}

TEST(TestBlurayPlaylistHints, ADiscWithoutAProjectOffersNothing)
{
  const CBlurayPlaylistHints hints{ProjectInformation{}};
  EXPECT_FALSE(hints.HasHints());
  EXPECT_TRUE(hints.GetHints().empty());
}
