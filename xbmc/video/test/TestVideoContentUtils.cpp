/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/VideoContentUtils.h"

#include <string_view>

#include <gtest/gtest.h>

using ::testing::ValuesIn;
using ::testing::WithParamInterface;
using KODI::VIDEO::DetermineContentForTVShows;
using KODI::VIDEO::TVShowEpisodePathResult;

namespace
{
struct TVShowContentTestData
{
  bool scraperSetOnThisPath;
  bool isShowNameFolder;
  TVShowEpisodePathResult queryResult;
  std::string_view expected;
};

class TestDetermineContentForTVShows : public testing::Test,
                                       public WithParamInterface<TVShowContentTestData>
{
};
} // namespace

// clang-format off
const TVShowContentTestData TVShowContentData[] = {
  // --- Direct episodes in path ----------------------------------
  // scraperSetOnThisPath / isShowNameFolder don't matter if episodesInThisPath is true
  {false, false, {.episodesInThisPath = true},  "episodes"},
  {true,  false, {.episodesInThisPath = true},  "episodes"},
  {false, true,  {.episodesInThisPath = true},  "episodes"},
  {true,  true,  {.episodesInThisPath = true},  "episodes"},

  // --- No direct episodes, no archive candidates ------------------------------
  // Scraper set directly on path, single-show mode OFF → source root (ie tvshows)
  {true,  false, {}, "tvshows"},
  // Scraper inherited from parent path → intermediate folder with no episodes (directly or via parent path) → seasons
  // Note this includes episode files not in the root of an archive (eg. /TV Shows/Show (2002)/Season 1 and 2/episodes.rar/Season 1/episode.mkv)
  {false, false, {}, "seasons"},
  // Scraper set directly, single-show mode ON and folder has no episodes (directly or via parent path) → seasons
  {true,  true,  {}, "seasons"},

  // --- Archive candidates: episode in root of archive ----------
  // rar:// URL with no filename component → episode is at root → episodes
  {false, false, {.candidatePaths = {"rar://D%3a%5cShow%5cshow.rar/"}},        "episodes"},
  {false, false, {.candidatePaths = {"zip://D%3a%5cShow%5cshow.zip/"}},        "episodes"},
  {false, false, {.candidatePaths = {"archive://D%3a%5cShow%5cshow.tar.gz/"}}, "episodes"},

  // --- Bluray candidates ----------
  // bluray:// URL → episodes
  {false, false, {.candidatePaths = {"bluray://udf%3a%2f%2fD%253a%255cShows%255cShow%255cDisc%2520S01E01-E05.iso%2f/BDMV/PLAYLIST/"}},"episodes"},
};
// clang-format on

TEST_P(TestDetermineContentForTVShows, ReturnsExpectedContent)
{
  const auto& [scraperSetOnThisPath, singleShowFolder, queryResult, expected] = GetParam();
  EXPECT_EQ(expected,
            DetermineContentForTVShows(scraperSetOnThisPath, singleShowFolder, queryResult));
}

INSTANTIATE_TEST_SUITE_P(VideoContentUtils,
                         TestDetermineContentForTVShows,
                         ValuesIn(TVShowContentData));
