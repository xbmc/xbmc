/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/IPlaylistHints.h"
#include "filesystem/bluray/BlurayPlaylistHints.h"
#include "filesystem/bluray/ProjectParser.h"
#include "language/LangInfo.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
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

TEST_F(TestDiscDirectoryHelperProject, Movie_SingleTitleKeepsTheHeuristicsCopyOfThePlainFeature)
{
  // Seen on Avatar: Fire and Ash, where FPL_MainFeature_eng translates on screen what
  // FPL_MainFeature leaves out, and the heuristics choose it as the fuller copy
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u}, 3)},
                        {1661u, MakePlaylist(1661u, 120min, {1u}, 2)}};
  ClipMap clips{{1u, MakeClip(120min, {800u, 1661u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature_eng", 120min),
                                       MakeNamed(1661u, "FPL_MainFeature", 120min)}));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{800u});
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

TEST_F(TestDiscDirectoryHelperProject, Extras_TheExtrasTheDiscNamesAreListed)
{
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {810u, MakePlaylist(810u, 20min, {2u})},
                        {811u, MakePlaylist(811u, 2min, {3u})},
                        {820u, MakePlaylist(820u, 30s, {4u})},
                        {830u, MakePlaylist(830u, 1min, {5u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})},
                {2u, MakeClip(20min, {810u})},
                {3u, MakeClip(2min, {811u})},
                {4u, MakeClip(30s, {820u})},
                {5u, MakeClip(1min, {830u})}};

  // Nothing but the disc says which playlists are extras
  EXPECT_FALSE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_TRUE(items.IsEmpty());

  helper.SetPlaylistHints(
      MakeProject({MakeNamed(800u, "FPL_MainFeature", 120min),
                   MakeNamed(810u, "SF_01_Making", 20min), MakeNamed(811u, "SF_02_Trailer", 2min),
                   MakeNamed(820u, "WRN_Piracy", 30s), MakeNamed(830u, "TMPL Main Menu", 1min)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{810u, 811u}));
}

TEST_F(TestDiscDirectoryHelperProject, Extras_AnExtraLongEnoughForAnEditionIsAnExtra)
{
  // The heuristics would take the 90 minute extra for another cut of the 120 minute movie
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 90min, {2u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})}, {2u, MakeClip(90min, {801u})}};

  helper.SetPlaylistHints(MakeProject(
      {MakeNamed(800u, "FPL_MainFeature", 120min), MakeNamed(801u, "SF_Making_Of", 90min)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{801u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_ASingAlongNamedAsAnExtraIsNotAnExtra)
{
  // Seen on Snow White (2025), whose sing-along is another version of the movie
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 6529s, {1u})},
                        {801u, MakePlaylist(801u, 6531s, {2u})},
                        {802u, MakePlaylist(802u, 10min, {3u})}};
  ClipMap clips{
      {1u, MakeClip(6529s, {800u})}, {2u, MakeClip(6531s, {801u})}, {3u, MakeClip(10min, {802u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 6529s),
                                       MakeNamed(801u, "SF_SA_01_00_PlayMovie", 6531s),
                                       MakeNamed(802u, "SF_01_Making", 10min)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{802u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_ASingAlongTakenForACopyOfTheMovieIsNotAnExtra)
{
  // Seen on Mufasa (2024), whose sing-along plays the feature's clip, so is no version either
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{801u, MakePlaylist(801u, 7085s, {1u})},
                        {1628u, MakePlaylist(1628u, 7085s, {1u})},
                        {1630u, MakePlaylist(1630u, 156s, {2u})}};
  ClipMap clips{{1u, MakeClip(7085s, {801u, 1628u})}, {2u, MakeClip(156s, {1630u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(801u, "FPL_MainFeature", 7085s),
                                       MakeNamed(1628u, "SF_00_Feature_SingAlong", 7085s),
                                       MakeNamed(1630u, "SF_01_SS_01_Milele", 156s)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{1630u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_AShortExtraIsNotTakenForAPlaceholderFeature)
{
  // Seen on the bonus disc of Aliens (1986), which names a 5 second placeholder as its feature
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 5s, {1u})},
                        {1965u, MakePlaylist(1965u, 12s, {2u})}};
  ClipMap clips{{1u, MakeClip(5s, {800u})}, {2u, MakeClip(12s, {1965u})}};

  helper.SetPlaylistHints(MakeProject(
      {MakeNamed(800u, "FPL_MainFeature", 5s), MakeNamed(1965u, "SF_CH16_SL27_MiniAPC", 12s)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{1965u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_ACopyrightCardIsNotAnExtra)
{
  // Seen on The Running Man (2025), whose extras end on a six second copyright card
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 120min, {1u})},
                        {801u, MakePlaylist(801u, 10min, {2u})},
                        {802u, MakePlaylist(802u, 6s, {3u})}};
  ClipMap clips{
      {1u, MakeClip(120min, {800u})}, {2u, MakeClip(10min, {801u})}, {3u, MakeClip(6s, {802u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 120min),
                                       MakeNamed(801u, "SF_01_HuntBegins", 10min),
                                       MakeNamed(802u, "SF_Copyright", 6s)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{801u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_CopiesOfAnExtraAreListedOnce)
{
  // Seen on Aquaman and the Lost Kingdom (2023), which also offers each extra as a segment and
  // dubbed into Japanese
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{126u, MakePlaylist(126u, 1284s, {11u})},
                        {800u, MakePlaylist(800u, 124min, {1u})},
                        {811u, MakePlaylist(811u, 1284s, {11u})},
                        {819u, MakePlaylist(819u, 1284s, {19u})}};
  ClipMap clips{{1u, MakeClip(124min, {800u})},
                {11u, MakeClip(1284s, {126u, 811u})},
                {19u, MakeClip(1284s, {819u})}};

  helper.SetPlaylistHints(MakeProject(
      {MakeNamed(126u, "SEG SF_01_Finding", 1284s), MakeNamed(800u, "FPL_MainFeature", 124min),
       MakeNamed(811u, "SF_01_Finding", 1284s), MakeNamed(819u, "SF_01_Finding_JPN", 1284s)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{811u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_NumberedExtrasAreNotCopies)
{
  // The numbers are left out of the titles, so both trailers are called "Trailer"
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 110min, {1u})},
                        {811u, MakePlaylist(811u, 140s, {2u})},
                        {812u, MakePlaylist(812u, 145s, {3u})}};
  ClipMap clips{
      {1u, MakeClip(110min, {800u})}, {2u, MakeClip(140s, {811u})}, {3u, MakeClip(145s, {812u})}};

  helper.SetPlaylistHints(
      MakeProject({MakeNamed(800u, "FPL_MainFeature", 110min),
                   MakeNamed(811u, "SF_Trailer_1", 140s), MakeNamed(812u, "SF_Trailer_2", 145s)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{811u, 812u}));
}

TEST_F(TestDiscDirectoryHelperProject, Extras_TheCopyWithoutAnythingAddedIsListed)
{
  // Seen on Eraser (1996), which names no plain presentation of its extras - a segment, one marked
  // _NCR and that dubbed into Japanese
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 115min, {1u})},
                        {819u, MakePlaylist(819u, 360s, {2u})},
                        {821u, MakePlaylist(821u, 361s, {3u})},
                        {822u, MakePlaylist(822u, 361s, {4u})}};
  ClipMap clips{{1u, MakeClip(115min, {800u})},
                {2u, MakeClip(360s, {819u})},
                {3u, MakeClip(361s, {821u})},
                {4u, MakeClip(361s, {822u})}};

  helper.SetPlaylistHints(
      MakeProject({MakeNamed(800u, "FPL_MainFeature", 115min),
                   MakeNamed(819u, "SEG_SF_01_Reinventing_Modern_Action_Hero", 360s),
                   MakeNamed(821u, "SF_01_Reinventing_Modern_Action_Hero_NCR", 361s),
                   MakeNamed(822u, "SF_01_Reinventing_Modern_Action_Hero_NCR_JPN", 361s)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{821u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_ASlateAnExtraPlaysIsNotAnExtra)
{
  // Seen on Nope (2022), whose deleted scenes come with a slate, without one, and the slate alone
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{800u, MakePlaylist(800u, 130min, {1u})},
                        {1248u, MakePlaylist(1248u, 120s, {2u, 3u, 4u})},
                        {1253u, MakePlaylist(1253u, 114s, {2u, 3u})},
                        {1258u, MakePlaylist(1258u, 12s, {2u})}};
  ClipMap clips{{1u, MakeClip(130min, {800u})},
                {2u, MakeClip(12s, {1248u, 1253u, 1258u})},
                {3u, MakeClip(102s, {1248u, 1253u})},
                {4u, MakeClip(6s, {1248u})}};

  helper.SetPlaylistHints(MakeProject({MakeNamed(800u, "FPL_MainFeature", 130min),
                                       MakeNamed(1248u, "SF_DS_01_01_Hiker", 120s),
                                       MakeNamed(1253u, "SF_DS_01_01_Hiker_Binge", 114s),
                                       MakeNamed(1258u, "SEG_SF_DS_01_01_Hiker_Slate", 12s)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), std::vector<unsigned int>{1248u});
}

TEST_F(TestDiscDirectoryHelperProject, Extras_PlayAllIsListedWithWhatItPlays)
{
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{700u, MakePlaylist(700u, 100min, {1u})},
                        {711u, MakePlaylist(711u, 2min, {2u})},
                        {712u, MakePlaylist(712u, 4min, {3u})},
                        {718u, MakePlaylist(718u, 6min, {2u, 3u})}};
  ClipMap clips{{1u, MakeClip(100min, {700u})},
                {2u, MakeClip(2min, {711u, 718u})},
                {3u, MakeClip(4min, {712u, 718u})}};

  helper.SetPlaylistHints(
      MakeProject({MakeNamed(700u, "FPL_MainFeature", 100min), MakeNamed(711u, "SF_DS_01", 2min),
                   MakeNamed(712u, "SF_DS_02", 4min), MakeNamed(718u, "SF_DS_PlayAll", 6min)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));
  EXPECT_EQ(GetPlaylists(items), (std::vector<unsigned int>{711u, 712u, 718u}));
}

class TestDiscDirectoryHelperProjectTitles : public TestDiscDirectoryHelperProject
{
protected:
  void SetUp() override
  {
    ASSERT_TRUE(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Load(
        g_langInfo.GetLanguagePath(), "resource.language.en_gb"));
  }

  void TearDown() override { CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Clear(); }
};

TEST_F(TestDiscDirectoryHelperProjectTitles, Extras_AreTitledByWhatTheyAre)
{
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 120min, {1u})}, {810u, MakePlaylist(810u, 20min, {2u})},
      {811u, MakePlaylist(811u, 2min, {3u})},   {812u, MakePlaylist(812u, 2min, {3u})},
      {813u, MakePlaylist(813u, 20min, {2u})},  {814u, MakePlaylist(814u, 20min, {2u})},
      {815u, MakePlaylist(815u, 7min, {4u})},   {816u, MakePlaylist(816u, 3min, {5u})}};
  ClipMap clips{{1u, MakeClip(120min, {800u})},
                {2u, MakeClip(20min, {810u, 813u, 814u})},
                {3u, MakeClip(2min, {811u, 812u})},
                {4u, MakeClip(7min, {815u})},
                {5u, MakeClip(3min, {816u})}};

  helper.SetPlaylistHints(MakeProject(
      {MakeNamed(800u, "FPL_MainFeature", 120min), MakeNamed(810u, "SF_01_GagReel", 20min),
       MakeNamed(811u, "SF_DS_06_02_TakeOffShoes", 2min), MakeNamed(812u, "SF_DS_PlayAll", 2min),
       MakeNamed(813u, "SF_MakingOf_PlayAll", 20min), MakeNamed(814u, "SF_03_NE_00_PlayAll", 20min),
       MakeNamed(815u, "SF_BTS", 7min), MakeNamed(816u, "SF_CAST_01_01_ChrisPratt", 3min)}));

  EXPECT_TRUE(helper.GetMovieExtraPlaylists(url, items, allTitles, -1, clips, playlists));

  const std::vector<std::string> titles{"Gag Reel",
                                        "Deleted scene: Take Off Shoes",
                                        "Deleted scenes (play all)",
                                        "Making Of (play all)",
                                        "NE (play all)",
                                        "Behind the scenes",
                                        "Cast: Chris Pratt"};
  ASSERT_EQ(items.Size(), static_cast<int>(titles.size()));
  for (int i = 0; i < items.Size(); ++i)
  {
    EXPECT_EQ(items[i]->GetProperty(EXTRA_TITLE_PROPERTY).asString(), titles[i]);
    EXPECT_TRUE(items[i]->GetLabel2().starts_with(titles[i] + " - ")) << items[i]->GetLabel2();
  }
}
