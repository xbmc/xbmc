/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/IPlaylistHints.h"
#include "filesystem/bluray/BlurayPlaylistHints.h"
#include "filesystem/bluray/ProjectParser.h"
#include "video/VideoInfoTag.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;
using namespace std::chrono_literals;

namespace
{
PlaylistInformation MakePlaylist(unsigned int playlist,
                                 std::chrono::milliseconds duration,
                                 std::vector<unsigned int> clips,
                                 unsigned int audioStreams = 2,
                                 unsigned int subtitleStreams = 2)
{
  PlaylistInformation info;
  info.playlist = playlist;
  info.duration = duration;
  info.clips = std::move(clips);
  info.chapters = {0ms, duration / 2};
  info.audioStreams.resize(audioStreams);
  info.pgStreams.resize(subtitleStreams);
  return info;
}

ClipInfo MakeClip(std::chrono::milliseconds duration, std::vector<unsigned int> playlists)
{
  return ClipInfo{.duration = duration, .playlists = std::move(playlists)};
}

KODI::VIDEO::EPISODE MakeEpisode(int season, int episode, unsigned int seconds, std::string title)
{
  KODI::VIDEO::EPISODE e{season, episode, 0, false};
  e.duration = seconds;
  e.strTitle = std::move(title);
  return e;
}

//! One playlist as the disc's authoring project names it
ProjectPlaylistInformation MakeNamed(unsigned int playlist,
                                     std::string name,
                                     std::chrono::milliseconds duration)
{
  ProjectPlaylistInformation info;
  info.playlist = playlist;
  info.name = std::move(name);
  info.presentation = "2D";
  info.frameRate = 23.976f;
  info.duration = duration;
  info.playItems = 1;
  return info;
}

//! The hints a disc carrying a project naming these playlists would offer the helper
std::shared_ptr<const IPlaylistHints> MakeProject(
    const std::vector<ProjectPlaylistInformation>& named)
{
  ProjectInformation project;
  project.present = true;
  for (const auto& info : named)
    project.playlists.emplace(info.playlist, info);
  return std::make_shared<CBlurayPlaylistHints>(project);
}

std::vector<unsigned int> GetPlaylists(const CFileItemList& items)
{
  std::vector<unsigned int> playlists;
  for (const auto& item : items)
    playlists.push_back(
        static_cast<unsigned int>(item->GetProperty("bluray_playlist").asInteger32(0)));
  return playlists;
}
} // namespace

class TestDiscDirectoryHelperProject : public testing::Test
{
};

TEST_F(TestDiscDirectoryHelperProject, Episodes_NamedByOrdinalAreUsedOverTheHeuristics)
{
  // The heuristics would offer 800 and 801 by duration alone. The project says which is which.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 2400, "One"), MakeEpisode(1, 2, 2400, "Two")};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 40min, {1u})},
                        {801u, MakePlaylist(801u, 40min, {2u})}};
  ClipMap clips{{1u, MakeClip(40min, {800u})}, {2u, MakeClip(40min, {801u})}};

  // The project numbers them the other way round to their playlists, so only its answer
  // produces this pairing
  helper.SetPlaylistHints(
      MakeProject({MakeNamed(801u, "EPL_01", 40min), MakeNamed(800u, "EPL_02", 40min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{801u});

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});
}

TEST_F(TestDiscDirectoryHelperProject, Episodes_PlainPresentationLeadsItsGroup)
{
  // EPL_01 is the episode as it is meant to be watched; EPL_01_Narrative is the described one.
  // Both are offered, the plain one first, whatever their playlist numbers.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 2400, "One")};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 40min, {1u}, 1, 0)},
                        {900u, MakePlaylist(900u, 40min, {1u}, 2, 2)}};
  ClipMap clips{{1u, MakeClip(40min, {800u, 900u})}};

  helper.SetPlaylistHints(
      MakeProject({MakeNamed(800u, "EPL_01_Narrative", 40min), MakeNamed(900u, "EPL_01", 40min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  EXPECT_EQ(GetPlaylists(items).front(), 900u);
}

TEST_F(TestDiscDirectoryHelperProject, Episodes_FullestPresentationLeadsWhereBothArePlain)
{
  // A disc names an episode twice - EPL_01 and SEG_EPL_01 - from the same clip with the same
  // audio, differing only in how many subtitles they expose. The fuller one leads.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 2400, "One")};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 40min, {1u}, 2, 2)},
                        {900u, MakePlaylist(900u, 40min, {1u}, 2, 3)}};
  ClipMap clips{{1u, MakeClip(40min, {800u, 900u})}};

  helper.SetPlaylistHints(
      MakeProject({MakeNamed(800u, "EPL_01", 40min), MakeNamed(900u, "SEG_EPL_01", 40min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  EXPECT_EQ(GetPlaylists(items).front(), 900u);
}

TEST_F(TestDiscDirectoryHelperProject, Episodes_UnmatchedCountLeavesTheHeuristicsAlone)
{
  // The project numbers one episode where the disc is said to hold two, so the two lists cannot
  // be paired and nothing is overridden.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 2400, "One"), MakeEpisode(1, 2, 2400, "Two")};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 40min, {1u})},
                        {801u, MakePlaylist(801u, 40min, {2u})}};
  ClipMap clips{{1u, MakeClip(40min, {800u})}, {2u, MakeClip(40min, {801u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "EPL_01", 40min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  // Whatever the heuristics chose, the project did not replace it with its single named episode
  EXPECT_FALSE(items.IsEmpty());
}

TEST_F(TestDiscDirectoryHelperProject, Specials_MatchedWhenTheEpisodesAreNot)
{
  // The disc names one of the two episodes on it, so its numbering cannot be trusted for them.
  // It still names the extra outright, and that does not depend on the episodes.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 600, "Becoming Pennywise"),
                    MakeEpisode(1, 1, 2400, "Episode One"), MakeEpisode(1, 2, 2400, "Episode Two")};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 40min, {1u})},
                        {801u, MakePlaylist(801u, 40min, {2u})},
                        {820u, MakePlaylist(820u, 10min, {3u})},
                        {821u, MakePlaylist(821u, 10min, {4u})}};
  ClipMap clips{{1u, MakeClip(40min, {800u})},
                {2u, MakeClip(40min, {801u})},
                {3u, MakeClip(10min, {820u})},
                {4u, MakeClip(10min, {821u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "EPL_01", 40min),
                                       MakeNamed(820u, "SF_Becoming_Pennywise", 10min),
                                       MakeNamed(821u, "SF_Fear_The_Other", 10min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{820u});
}

TEST_F(TestDiscDirectoryHelperProject, Specials_MatchedByTitle)
{
  // Nothing on the disc tells one extra from another, so the name the project gave them is
  // matched against the title the scraper knows.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 600, "Becoming Pennywise"),
                    MakeEpisode(0, 2, 600, "Fear the Other")};

  PlaylistMap playlists{{820u, MakePlaylist(820u, 10min, {1u})},
                        {821u, MakePlaylist(821u, 10min, {2u})}};
  ClipMap clips{{1u, MakeClip(10min, {820u})}, {2u, MakeClip(10min, {821u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(820u, "SF_Fear_The_Other", 10min),
                                       MakeNamed(821u, "SF_Becoming_Pennywise", 10min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{821u});

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{820u});
}

TEST_F(TestDiscDirectoryHelperProject, Specials_EpisodeNumberInTheNameIsMatched)
{
  // A disc numbers a featurette after the episode it accompanies, running season and episode
  // together, where a scraper numbers it plainly.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 360, "Inside Derry #2"),
                    MakeEpisode(0, 2, 360, "Inside Derry #3")};

  PlaylistMap playlists{{820u, MakePlaylist(820u, 6min, {1u})},
                        {821u, MakePlaylist(821u, 6min, {2u})}};
  ClipMap clips{{1u, MakeClip(6min, {820u})}, {2u, MakeClip(6min, {821u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(820u, "SF_Inside_Derry_102", 6min),
                                       MakeNamed(821u, "SF_Inside_Derry_103", 6min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{820u});

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{821u});
}

TEST_F(TestDiscDirectoryHelperProject, Specials_EachPlaylistIsSpokenForOnce)
{
  // "Inside Derry #1" resembles the featurette for episode 2 nearly as much as the one it means.
  // The surer pairing is settled first, leaving the closer call only what is still free.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 360, "Inside Derry #1"),
                    MakeEpisode(0, 2, 360, "Inside Derry #2")};

  PlaylistMap playlists{{820u, MakePlaylist(820u, 6min, {1u})},
                        {822u, MakePlaylist(822u, 6min, {2u})}};
  ClipMap clips{{1u, MakeClip(6min, {820u})}, {2u, MakeClip(6min, {822u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(820u, "SF_Inside_Derry_102", 6min),
                                       MakeNamed(822u, "SF_Inside_Derry_Extended_101", 6min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{820u});

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{822u});
}

TEST_F(TestDiscDirectoryHelperProject, Specials_ADifferentNumberIsNotAMatch)
{
  // Only the first of the two has been scraped, so the one it means is not yet spoken for. The
  // featurette for episode 2 resembles it closely enough to pass for it, and must not be taken.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 360, "Inside Derry 1"), MakeEpisode(0, 2, 360, "")};

  PlaylistMap playlists{{820u, MakePlaylist(820u, 6min, {1u})},
                        {822u, MakePlaylist(822u, 6min, {2u})}};
  ClipMap clips{{1u, MakeClip(6min, {820u})}, {2u, MakeClip(6min, {822u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(820u, "SF_Inside_Derry_102", 6min),
                                       MakeNamed(822u, "SF_Inside_Derry_Extended_101", 6min)}));

  // The heuristics' candidates stand, flagged so that a scan does not guess
  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  for (const auto& item : items)
    EXPECT_TRUE(item->GetProperty(MULTIPLE_SPECIALS_PROPERTY).asBoolean(false));
}

TEST_F(TestDiscDirectoryHelperProject, Specials_APaddedNumberIsTheSameNumber)
{
  // The scraper pads its numbers where the disc does not, or the other way about. Either way the
  // two are talking about the same episode's featurette.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 360, "Inside Derry #02"),
                    MakeEpisode(0, 2, 360, "Inside Derry #03")};

  PlaylistMap playlists{{820u, MakePlaylist(820u, 6min, {1u})},
                        {821u, MakePlaylist(821u, 6min, {2u})}};
  ClipMap clips{{1u, MakeClip(6min, {820u})}, {2u, MakeClip(6min, {821u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(820u, "SF_Inside_Derry_102", 6min),
                                       MakeNamed(821u, "SF_Inside_Derry_103", 6min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{820u});
  EXPECT_FALSE(items[0]->GetProperty(MULTIPLE_SPECIALS_PROPERTY).asBoolean(false));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{821u});
}

TEST_F(TestDiscDirectoryHelperProject, Specials_UntitledLeavesTheMatchingAlone)
{
  // A special the scraper has not named yet cannot be told from any other, so nothing is claimed
  // on its behalf.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 360, ""), MakeEpisode(0, 2, 360, "")};

  PlaylistMap playlists{{820u, MakePlaylist(820u, 6min, {1u})},
                        {821u, MakePlaylist(821u, 6min, {2u})}};
  ClipMap clips{{1u, MakeClip(6min, {820u})}, {2u, MakeClip(6min, {821u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(820u, "SF_Fear_The_Other", 6min),
                                       MakeNamed(821u, "SF_Becoming_Pennywise", 6min)}));

  // The heuristics' candidates stand, flagged so that a scan does not guess
  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  for (const auto& item : items)
    EXPECT_TRUE(item->GetProperty(MULTIPLE_SPECIALS_PROPERTY).asBoolean(false));
}

TEST_F(TestDiscDirectoryHelperProject, Specials_OneOfEachNeedsNoTitle)
{
  // Where the disc is said to hold one special and the project names one thing that could be it,
  // there is no choice to get wrong.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 360, "")};

  PlaylistMap playlists{{820u, MakePlaylist(820u, 6min, {1u})},
                        {830u, MakePlaylist(830u, 20min, {2u})}};
  ClipMap clips{{1u, MakeClip(6min, {820u})}, {2u, MakeClip(20min, {830u})}};

  // 830 is the longer, so the heuristics would offer it too. Only the project says which
  // of the two is the extra.
  helper.SetPlaylistHints(MakeProject({MakeNamed(820u, "SF_Only_Extra", 6min)}));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{820u});
}

TEST_F(TestDiscDirectoryHelperProject, Movie_NamedFeatureLeadsAndTheHeuristicsEditionsFollow)
{
  // Two cuts of the movie. The heuristics offer the longer first. The disc names the shorter as
  // the feature, so it leads, but the other cut is still an edition and stays on offer.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 110min, {2u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}, {2u, MakeClip(110min, {801u})}};

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{800u, 801u}));

  helper.SetPlaylistHints(MakeProject({MakeNamed(801u, "FPL_MainFeature", 110min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{801u, 800u}));

  // A single title is the disc's alone
  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{801u});
}

TEST_F(TestDiscDirectoryHelperProject, Movie_ASegmentNamedAsTheFeatureIsNotAnotherVersion)
{
  // Seen on a disc naming a seconds-long playlist SEG_MainFeature alongside the feature
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 5s, {2u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}, {2u, MakeClip(5s, {801u})}};

  helper.SetPlaylistHints(MakeProject(
      {MakeNamed(800u, "FPL_MainFeature", 120min), MakeNamed(801u, "SEG_MainFeature", 5s)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});
}

TEST_F(TestDiscDirectoryHelperProject, Movie_AShortFilmTheDiscNamesIsOffered)
{
  // Below the minimum the heuristics accept for a movie
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 20min, {1u})}};
  ClipMap clips{{1u, MakeClip(20min, {800u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 20min)}));

  for (const GetTitle job : {GetTitle::SINGLE, GetTitle::MAIN})
  {
    EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, job, clips, playlists));
    EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});
  }
}

TEST_F(TestDiscDirectoryHelperProject, Movie_AnExtraTheDiscNamesIsNotAnotherVersion)
{
  // A 90 minute extra is long enough for the heuristics to take it for another cut of a 120
  // minute movie. The disc says what it is, so it does not follow the feature into the versions.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 90min, {2u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}, {2u, MakeClip(90min, {801u})}};

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{800u, 801u}));

  helper.SetPlaylistHints(MakeProject(
      {MakeNamed(800u, "FPL_MainFeature", 120min), MakeNamed(801u, "SF_Making_Of", 90min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});
}

TEST_F(TestDiscDirectoryHelperProject, Movie_ASingAlongNamedAsAnExtraIsAnotherVersion)
{
  // Seen on Snow White, whose sing-along is SF_SA_01_00_PlayMovie and 2s longer than the feature
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 6529s, {1u})},
                        {801u, MakePlaylist(801u, 6531s, {2u})}};
  ClipMap clips{{1u, MakeClip(6529s, {800u})}, {2u, MakeClip(6531s, {801u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 6529s),
                                       MakeNamed(801u, "SF_SA_01_00_PlayMovie", 6531s)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{800u, 801u}));
}

TEST_F(TestDiscDirectoryHelperProject, Movie_ACopyOfTheNamedFeatureIsNotAnotherVersion)
{
  // Two playlists of the same clip, differing only in how many subtitles they expose. The
  // heuristics keep the fuller one. The disc names the other, and the named one stands for both.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u}, 2, 3)},
                        {801u, MakePlaylist(801u, 120min, {1u}, 2, 2)}};
  ClipMap clips{{1u, MakeClip(120min, {800u, 801u})}};

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});

  helper.SetPlaylistHints(MakeProject({MakeNamed(801u, "FPL_MainFeature", 120min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{801u});
}

TEST_F(TestDiscDirectoryHelperProject, Movie_SingleTitlePrefersThePlainFeatureOverAnExtendedCut)
{
  // The disc names both the plain feature and an extended cut. The heuristics would pick the
  // longer as the single title, but a single title is the plain feature where one is on offer.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 130min, {2u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}, {2u, MakeClip(130min, {801u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 120min),
                                       MakeNamed(801u, "FPL_MainFeature_EXT", 130min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});
}

TEST_F(TestDiscDirectoryHelperProject, Movie_SingleTitleKeepsTheDiscInfMainPlaylist)
{
  // disc.inf names playlist 801 as the main title. The project names 800 as the feature
  // instead, but for a single title disc.inf wins, so the main title stays as it is.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 110min, {2u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}, {2u, MakeClip(110min, {801u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 120min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 801, GetTitle::SINGLE, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{801u});
}

TEST_F(TestDiscDirectoryHelperProject, Movie_MainTitleLeadsWithTheDiscInfMainPlaylist)
{
  // disc.inf names playlist 801 as the main title, a shorter edition the project does not
  // name. With several editions on offer the disc.inf main title leads, ahead of the
  // project's named feature.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 90min, {2u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}, {2u, MakeClip(90min, {801u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 120min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 801, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{801u, 800u}));
}

TEST_F(TestDiscDirectoryHelperProject, RootOptionsSortBelowThePlaylists)
{
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 100min, {1u})},
                        {801u, MakePlaylist(801u, 120min, {2u})}};
  ClipMap clips{{1u, MakeClip(100min, {800u})}, {2u, MakeClip(120min, {801u})}};

  // By duration for the simple menu, and a sort that puts folders first when browsing
  for (const SortBy sortBy : {SortBy::TIME, SortBy::LABEL})
  {
    CFileItemList items;
    EXPECT_TRUE(
        helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
    CDiscDirectoryHelper::AddRootOptions(url, items, CDiscDirectoryHelper::AllTitles::MOVIES,
                                         AddMenuAndAllTitlesOptions::ADD_ALL_TITLES |
                                             AddMenuAndAllTitlesOptions::ADD_MENU);
    items.Sort(sortBy, SortOrder::DESCENDING);

    ASSERT_EQ(items.Size(), 4);
    EXPECT_TRUE(items[0]->HasProperty("bluray_playlist"));
    EXPECT_TRUE(items[1]->HasProperty("bluray_playlist"));
    EXPECT_TRUE(items[2]->IsFolder()); // All titles
    EXPECT_FALSE(items[3]->IsFolder()); // Menu
  }
}

TEST_F(TestDiscDirectoryHelperProject, Movie_TheNameLeadsTheDescription)
{
  // The label gives the playlist number, so the description does not repeat it
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature_eng", 120min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_TRUE(items[0]->GetLabel2().starts_with("FPL_MainFeature_eng - ")) << items[0]->GetLabel2();
}

TEST_F(TestDiscDirectoryHelperProject, NoProjectLeavesTheHeuristicsUntouched)
{
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 2400, "One")};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 40min, {1u})}};
  ClipMap clips{{1u, MakeClip(40min, {800u})}};

  // No SetPlaylistHints call at all
  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});
}
