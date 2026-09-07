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
constexpr unsigned int ALL_EPISODES = static_cast<unsigned int>(-1);

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
                    MakeEpisode(1, 1, 2400, "Episode One"),
                    MakeEpisode(1, 2, 2400, "Episode Two")};

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

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
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

  // A disc holding more than one special offers none, so the user is asked instead
  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
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
