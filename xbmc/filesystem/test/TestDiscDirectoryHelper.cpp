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
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "video/Episode.h"

#include <array>
#include <chrono>
#include <numeric>
#include <ranges>
#include <set>
#include <string>

#include <gtest/gtest.h>

using ::testing::Test;
using namespace XFILE;
using namespace std::chrono_literals;

namespace
{
class AdvancedSettingsResetBase : public Test
{
public:
  AdvancedSettingsResetBase()
  {
    // Force all advanced settings to be reset to defaults
    const auto settings = CServiceBroker::GetSettingsComponent();
    const auto advancedSettings = settings->GetAdvancedSettings();
    m_oldMinimumEpisodePlaylistDuration = advancedSettings->m_minimumEpisodePlaylistDuration;
    advancedSettings->m_minimumEpisodePlaylistDuration = 10 * 60; // 10 minutes
  }

  ~AdvancedSettingsResetBase() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent();
    settings->GetAdvancedSettings()->m_minimumEpisodePlaylistDuration =
        m_oldMinimumEpisodePlaylistDuration;
  }

private:
  int m_oldMinimumEpisodePlaylistDuration{0};
};

class TestDiscDirectoryHelper : public AdvancedSettingsResetBase
{
};

PlaylistInformation MakePlaylist(unsigned int playlist,
                                 std::chrono::milliseconds duration,
                                 std::vector<unsigned int> clips,
                                 std::vector<std::chrono::milliseconds> chapterDurations,
                                 std::string languages = "")
{
  PlaylistInformation info;
  info.playlist = playlist;
  info.duration = duration;
  info.clips = std::move(clips);
  info.chapters = std::move(chapterDurations);
  info.languages = std::move(languages);
  return info;
}

ClipInfo MakeClip(std::chrono::milliseconds duration, std::vector<unsigned int> playlists)
{
  ClipInfo info;
  info.duration = duration;
  info.playlists = std::move(playlists);
  return info;
}

KODI::VIDEO::EPISODE MakeEpisode(int season,
                                 int episode,
                                 unsigned int durationSeconds = 0,
                                 std::string title = "Test Episode")
{
  KODI::VIDEO::EPISODE ep(season, episode);
  ep.duration = durationSeconds;
  ep.strTitle = std::move(title);
  return ep;
}

// Parses the 5-digit playlist number from a bluray path such as
// "bluray://test/BDMV/PLAYLIST/00801.mpls".
unsigned int GetPlaylistFromPath(const std::string& path)
{
  const auto pos = path.rfind('/');
  if (pos == std::string::npos || pos + 1 >= path.size())
    return 0;
  try
  {
    return static_cast<unsigned int>(std::stoul(path.substr(pos + 1)));
  }
  catch (...)
  {
    return 0;
  }
}

std::set<unsigned int> GetPlaylists(const CFileItemList& items)
{
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  return returned;
}

bool Validate(ClipMap& clips, PlaylistMap& playlists)
{
  // Check relationship between clips and playlists
  for (const auto& [playlistNumber, playlistInformation] : playlists)
  {
    std::chrono::milliseconds duration{0ms};

    if (playlistInformation.playlist != playlistNumber)
      return false; // Playlist number does not match key in map

    // Check that all clips in playlist are in clip map and reference the playlist
    for (const auto clip : playlistInformation.clips)
    {
      if (!clips.contains(clip))
        return false; // Clip in playlist not in clip map

      if (std::ranges::find(clips[clip].playlists, playlistNumber) == clips[clip].playlists.end())
        return false; // Playlist not referenced by clip

      duration += clips.at(clip).duration;
    }

    if (duration != playlistInformation.duration)
      return false; // Playlist duration does not match total of the clips(s)

    const auto chapter_duration{std::accumulate(playlistInformation.chapters.begin(),
                                                playlistInformation.chapters.end(), 0ms)};
    if (chapter_duration != playlistInformation.duration)
      return false; // Playlist duration does not match total of the chapter(s)
  }

  for (const auto& clipInfo : clips | std::views::values)
  {
    for (const auto playlist : clipInfo.playlists)
    {
      if (!playlists.contains(playlist))
        return false; // Playlist in clip not in playlist map
    }
  }

  return true;
}
} // namespace

//
// ---- GetEpisodePlaylists – no candidates ------------------------------------
//

TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_EmptyInputs)
{
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 5min, {1u}, {5min})}};
  ClipMap clips{{1u, MakeClip(5min, {800u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, {}, {}));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, {}));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, {}, playlists));
  EXPECT_EQ(items.Size(), 0);
}

TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_AllPlaylistsBelowMinEpisodeDuration)
{
  // A single playlist shorter than MIN_EPISODE_DURATION must not be chosen.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 5min, {1u}, {5min})}};
  ClipMap clips{{1u, MakeClip(5min, {800u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                             playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);
}

//
// ---- GetEpisodePlaylists – single episode disc ------------------------------
//

// Single episode on disc with no specials
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_OnePlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 60min, {1u}, {60min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);
}

// One playlist of > MIN_EPISODE_DURATION and multiple shorter playlists
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_MultiplePlaylists)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 60min, {1u}, {60min})},
                        {1u, MakePlaylist(1u, 5min, {2u}, {5min})},
                        {10u, MakePlaylist(10u, 5min, {3u}, {5min})},
                        {100u, MakePlaylist(100u, 5min, {4u}, {5min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})},
                {2u, MakeClip(5min, {1u})},
                {3u, MakeClip(5min, {10u})},
                {4u, MakeClip(5min, {100u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 4);
  const auto returned{GetPlaylists(items)};
  const std::set<unsigned int> expected{1u, 10u, 100u, 800u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Two playlists of > MIN_EPISODE_DURATION, one with a common playlist number, and multiple shorter playlists
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_MultiplePlaylists2)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 60min, {1u}, {60min})},
                        {1u, MakePlaylist(1u, 5min, {2u}, {5min})},
                        {10u, MakePlaylist(10u, 5min, {3u}, {5min})},
                        {100u, MakePlaylist(100u, 40min, {4u}, {40min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})},
                {2u, MakeClip(5min, {1u})},
                {3u, MakeClip(5min, {10u})},
                {4u, MakeClip(40min, {100u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Two playlists of > MIN_EPISODE_DURATION, one with a common playlist number, and multiple shorter playlists
// One of the other playlists is a special feature
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_MultiplePlaylists_WithSpecial)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 1800), // Special
                    MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 60min, {1u}, {60min})},
                        {1u, MakePlaylist(1u, 5min, {2u}, {5min})},
                        {10u, MakePlaylist(10u, 5min, {3u}, {5min})},
                        {100u, MakePlaylist(100u, 30min, {4u}, {30min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})},
                {2u, MakeClip(5min, {1u})},
                {3u, MakeClip(5min, {10u})},
                {4u, MakeClip(30min, {100u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // Episode

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 10u, 100u};
  EXPECT_TRUE(std::ranges::includes(
      returned, expected)); // Any of the 3 remaining playlists could be the special

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 2);
  returned = GetPlaylists(items);
  expected = {100u, 800u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

//
// ---- GetEpisodePlaylists – play-all playlist method -------------------------
//

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play - all playlist(clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// One other the other playlists is a special
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_WithSpecial)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(0, 1, 1800), // Special
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  EXPECT_TRUE(returned.contains(1u)); // Any of the 2 remaining playlists could be the special
  EXPECT_TRUE(returned.contains(10u));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  returned = GetPlaylists(items);
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an intro clip
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 128min, {6u, 1u, 2u, 3u}, {3min, 45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(3min, {100u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an ending clip
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips2)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 128min, {1u, 2u, 3u, 6u}, {45min, 42min, 38min, 3min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(3min, {100u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an ending clip, and is present at the end of each episode as well
// Example is Planet Earth (2006) S1D1
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips2a)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 7500500ms, {1u, 2u, 3u, 6u}, {45min, 42min, 38min, 500ms})},
      {800u, MakePlaylist(800u, 2700500ms, {1u, 6u}, {45min, 500ms})},
      {802u, MakePlaylist(802u, 2520500ms, {2u, 6u}, {42min, 500ms})},
      {804u, MakePlaylist(804u, 2280500ms, {3u, 6u}, {38min, 500ms})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(500ms, {100u, 800u, 802u, 804u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3; 806 = episode 4
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an intro clip, and is present at the start of each episode as well
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips2b)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 minutes
      MakeEpisode(1, 2, 2520), // 42 minutes
      MakeEpisode(1, 3, 2280), // 38 minutes
      MakeEpisode(1, 4, 2400), // 40 minutes
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {7u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {8u}, {5min})},
      {100u,
       MakePlaylist(100u, 9900500ms, {6u, 1u, 2u, 3u, 4u}, {500ms, 45min, 42min, 38min, 40min})},
      {800u, MakePlaylist(800u, 2700500ms, {6u, 1u}, {500ms, 45min})},
      {802u, MakePlaylist(802u, 2520500ms, {6u, 2u}, {500ms, 42min})},
      {804u, MakePlaylist(804u, 2280500ms, {6u, 3u}, {500ms, 38min})},
      {806u, MakePlaylist(806u, 2400500ms, {6u, 4u}, {500ms, 40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})},
      {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})},
      {4u, MakeClip(40min, {100u, 806u})},
      {6u, MakeClip(500ms, {100u, 800u, 802u, 804u, 806u})},
      {7u, MakeClip(5min, {1u})},
      {8u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 806);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 4); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u, 806u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u, 806u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 7);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u, 806u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an intro clip and clip 7 is an ending clip
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips3)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 131min, {6u, 1u, 2u, 3u, 7u}, {3min, 45min, 42min, 38min, 3min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(3min, {100u})},
      {7u, MakeClip(3min, {100u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Individual episode playlists are allowed to have short beginning/ending clips (for recap/credits etc.)
// First episode has credits, second has intro and credits, last has intro
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraIndividualClips)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 48min, {1u, 6u}, {45min, 3min})},
      {802u, MakePlaylist(802u, 48min, {7u, 2u, 8u}, {3min, 42min, 3min})},
      {804u, MakePlaylist(804u, 41min, {9u, 3u}, {3min, 38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(3min, {800u})},
      {7u, MakeClip(3min, {802u})},        {8u, MakeClip(3min, {802u})},
      {9u, MakeClip(3min, {804u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 804);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Note that clips in playlist 100 don't match the individual episodes' clips (800,802,804)
// Clip 900 needed otherwise a group could be made with 800,802,804 as 'exactly numEpisode playlists and no specials'
// So failure expected
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_Fail)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {6u}, {38min})},
      {900u, MakePlaylist(900u, 40min, {7u}, {40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u})},       {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(38min, {804u})},
      {7u, MakeClip(40min, {900u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u, 802u, 804u,
                                  900u}; // 100u included as not a valid play-all playlist
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 7);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u, 900u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an intro clip and clip 7 is an ending clip - but both are too long
// Clip 900 needed otherwise a group could be made with 800,802,804 as 'exactly numEpisode playlists and no specials'
// So failure expected
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_Fail2)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 155min, {6u, 1u, 2u, 3u, 7u}, {15min, 45min, 42min, 38min, 15min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
      {900u, MakePlaylist(900u, 40min, {8u}, {40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(15min, {100u})},
      {7u, MakeClip(15min, {100u})},       {8u, MakeClip(40min, {900u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u, 802u, 804u,
                                  900u}; // 100u included as not a valid play-all playlist
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 7);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u, 900u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Individual episode playlists are allowed to have short beginning/ending clips (for recap/credits etc.)
// First episode has credits, second has intro and credits, last has intro but the credits
// Note the credits on the middle episode are too long
// So failure is expected
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_Fail3)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400),
      MakeEpisode(1, 3, 2400),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 48min, {1u, 6u}, {45min, 3min})},
      {802u, MakePlaylist(802u, 60min, {7u, 2u, 8u}, {3min, 42min, 15min})},
      {804u, MakePlaylist(804u, 41min, {9u, 3u}, {3min, 38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(3min, {800u})},
      {7u, MakeClip(3min, {802u})},        {8u, MakeClip(15min, {802u})},
      {9u, MakeClip(3min, {804u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 4);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u, 802u,
                                  804u}; // 100u included as not a valid play-all playlist
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

//
// ---- GetEpisodePlaylists – relaxed play-all playlist method -------------------------
//

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601-608 = episodes 1-8
// Clip layout is more complex and fails PlayAllPlaylist method
// Similar to the Last Airbender (2005) Bluray S1D1
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_RelaxedPlayAllPlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400), MakeEpisode(1, 3, 2400), MakeEpisode(1, 4, 2400),
      MakeEpisode(1, 5, 2400), MakeEpisode(1, 6, 2400), MakeEpisode(1, 7, 2400),
      MakeEpisode(1, 8, 2400),
  };

  PlaylistMap playlists{
      {600u, MakePlaylist(600u, 19411s,
                          {1100u, 1000u, 1001u, 1002u, 1003u, 1004u, 1005u, 1006u, 1007u, 1008u,
                           1009u, 1010u, 1011u, 1012u, 1013u, 1014u},
                          {1s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s,
                           30s, 2400s, 30s, 2400s})},
      {601u, MakePlaylist(601u, 2401s, {1100u, 1000u}, {1s, 2400s})},
      {602u, MakePlaylist(602u, 2431s, {1100u, 1001u, 1002u}, {1s, 30s, 2400s})},
      {603u, MakePlaylist(603u, 2431s, {1100u, 1003u, 1004u}, {1s, 30s, 2400s})},
      {604u, MakePlaylist(604u, 2431s, {1100u, 1005u, 1006u}, {1s, 30s, 2400s})},
      {605u, MakePlaylist(605u, 2431s, {1100u, 1007u, 1008u}, {1s, 30s, 2400s})},
      {606u, MakePlaylist(606u, 2431s, {1100u, 1009u, 1010u}, {1s, 30s, 2400s})},
      {607u, MakePlaylist(607u, 2431s, {1100u, 1011u, 1012u}, {1s, 30s, 2400s})},
      {608u, MakePlaylist(608u, 2431s, {1100u, 1013u, 1014u}, {1s, 30s, 2400s})}};
  ClipMap clips{
      {1000u, MakeClip(2400s, {600u, 601u})},
      {1001u, MakeClip(30s, {600u, 602u})},
      {1002u, MakeClip(2400s, {600u, 602u})},
      {1003u, MakeClip(30s, {600u, 603u})},
      {1004u, MakeClip(2400s, {600u, 603u})},
      {1005u, MakeClip(30s, {600u, 604u})},
      {1006u, MakeClip(2400s, {600u, 604u})},
      {1007u, MakeClip(30s, {600u, 605u})},
      {1008u, MakeClip(2400s, {600u, 605u})},
      {1009u, MakeClip(30s, {600u, 606u})},
      {1010u, MakeClip(2400s, {600u, 606u})},
      {1011u, MakeClip(30s, {600u, 607u})},
      {1012u, MakeClip(2400s, {600u, 607u})},
      {1013u, MakeClip(30s, {600u, 608u})},
      {1014u, MakeClip(2400s, {600u, 608u})},
      {1100u, MakeClip(1s, {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 8> expectedPlaylists{601u, 602u, 603u, 604u,
                                                                 605u, 606u, 607u, 608u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(expectedPlaylists.size());
       ++episodeIndex)
  {
    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1);
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), expectedPlaylists[episodeIndex]);
  }

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 8, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 8); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601-607 = episodes 1-7
// Clip layout is more complex and fails PlayAllPlaylist method
// Fail as only 7 episodes and 9 playlists in the group
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_RelaxedPlayAllPlaylist_Fail)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400), MakeEpisode(1, 3, 2400), MakeEpisode(1, 4, 2400),
      MakeEpisode(1, 5, 2400), MakeEpisode(1, 6, 2400), MakeEpisode(1, 7, 2400),
  };

  PlaylistMap playlists{
      {600u, MakePlaylist(600u, 19411s,
                          {1100u, 1000u, 1001u, 1002u, 1003u, 1004u, 1005u, 1006u, 1007u, 1008u,
                           1009u, 1010u, 1011u, 1012u, 1013u, 1014u},
                          {1s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s,
                           30s, 2400s, 30s, 2400s})},
      {601u, MakePlaylist(601u, 2401s, {1100u, 1000u}, {1s, 2400s})},
      {602u, MakePlaylist(602u, 2431s, {1100u, 1001u, 1002u}, {1s, 30s, 2400s})},
      {603u, MakePlaylist(603u, 2431s, {1100u, 1003u, 1004u}, {1s, 30s, 2400s})},
      {604u, MakePlaylist(604u, 2431s, {1100u, 1005u, 1006u}, {1s, 30s, 2400s})},
      {605u, MakePlaylist(605u, 2431s, {1100u, 1007u, 1008u}, {1s, 30s, 2400s})},
      {606u, MakePlaylist(606u, 2431s, {1100u, 1009u, 1010u}, {1s, 30s, 2400s})},
      {607u, MakePlaylist(607u, 2431s, {1100u, 1011u, 1012u}, {1s, 30s, 2400s})},
      {608u, MakePlaylist(608u, 2431s, {1100u, 1013u, 1014u}, {1s, 30s, 2400s})}};
  ClipMap clips{
      {1000u, MakeClip(2400s, {600u, 601u})},
      {1001u, MakeClip(30s, {600u, 602u})},
      {1002u, MakeClip(2400s, {600u, 602u})},
      {1003u, MakeClip(30s, {600u, 603u})},
      {1004u, MakeClip(2400s, {600u, 603u})},
      {1005u, MakeClip(30s, {600u, 604u})},
      {1006u, MakeClip(2400s, {600u, 604u})},
      {1007u, MakeClip(30s, {600u, 605u})},
      {1008u, MakeClip(2400s, {600u, 605u})},
      {1009u, MakeClip(30s, {600u, 606u})},
      {1010u, MakeClip(2400s, {600u, 606u})},
      {1011u, MakeClip(30s, {600u, 607u})},
      {1012u, MakeClip(2400s, {600u, 607u})},
      {1013u, MakeClip(30s, {600u, 608u})},
      {1014u, MakeClip(2400s, {600u, 608u})},
      {1100u, MakeClip(1s, {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601-608 = episodes 1-8
// Clip layout is more complex and fails PlayAllPlaylist method
// Fail as the playlist durations don't add up to the total duration of the first (play-all) playlist
// Second playlist 602 is too long
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_RelaxedPlayAllPlaylist_Fail2)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400), MakeEpisode(1, 3, 2400), MakeEpisode(1, 4, 2400),
      MakeEpisode(1, 5, 2400), MakeEpisode(1, 6, 2400), MakeEpisode(1, 7, 2400),
      MakeEpisode(1, 8, 2400),
  };

  PlaylistMap playlists{
      {600u, MakePlaylist(600u, 19411s,
                          {1100u, 1000u, 1001u, 1022u, 1003u, 1004u, 1005u, 1006u, 1007u, 1008u,
                           1009u, 1010u, 1011u, 1012u, 1013u, 1014u},
                          {1s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s,
                           30s, 2400s, 30s, 2400s})},
      {601u, MakePlaylist(601u, 2401s, {1100u, 1000u}, {1s, 2400s})},
      {602u, MakePlaylist(602u, 4431s, {1100u, 1001u, 1002u}, {1s, 30s, 4400s})},
      {603u, MakePlaylist(603u, 2431s, {1100u, 1003u, 1004u}, {1s, 30s, 2400s})},
      {604u, MakePlaylist(604u, 2431s, {1100u, 1005u, 1006u}, {1s, 30s, 2400s})},
      {605u, MakePlaylist(605u, 2431s, {1100u, 1007u, 1008u}, {1s, 30s, 2400s})},
      {606u, MakePlaylist(606u, 2431s, {1100u, 1009u, 1010u}, {1s, 30s, 2400s})},
      {607u, MakePlaylist(607u, 2431s, {1100u, 1011u, 1012u}, {1s, 30s, 2400s})},
      {608u, MakePlaylist(608u, 2431s, {1100u, 1013u, 1014u}, {1s, 30s, 2400s})}};
  ClipMap clips{
      {1000u, MakeClip(2400s, {600u, 601u})},
      {1001u, MakeClip(30s, {600u, 602u})},
      {1002u, MakeClip(4400s, {602u})},
      {1003u, MakeClip(30s, {600u, 603u})},
      {1004u, MakeClip(2400s, {600u, 603u})},
      {1005u, MakeClip(30s, {600u, 604u})},
      {1006u, MakeClip(2400s, {600u, 604u})},
      {1007u, MakeClip(30s, {600u, 605u})},
      {1008u, MakeClip(2400s, {600u, 605u})},
      {1009u, MakeClip(30s, {600u, 606u})},
      {1010u, MakeClip(2400s, {600u, 606u})},
      {1011u, MakeClip(30s, {600u, 607u})},
      {1012u, MakeClip(2400s, {600u, 607u})},
      {1013u, MakeClip(30s, {600u, 608u})},
      {1014u, MakeClip(2400s, {600u, 608u})},
      {1022u, MakeClip(2400s, {600u})},
      {1100u, MakeClip(1s, {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601-603 = episodes 1-3; 604 = episodes 4 and 5; 605-608 = episodes 6-9
// Clip layout is more complex and fails PlayAllPlaylist method
// Nine episodes accounted for by eight playlists, all of the same duration bar the double episode
// Similar to the Last Airbender (2005) Bluray S2D2
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_RelaxedPlayAllPlaylist_WithDoubleEpisode2)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400), MakeEpisode(1, 3, 2400), MakeEpisode(1, 4, 2400),
      MakeEpisode(1, 5, 2400), MakeEpisode(1, 6, 2400), MakeEpisode(1, 7, 2400),
      MakeEpisode(1, 8, 2400), MakeEpisode(1, 9, 2400),
  };

  PlaylistMap playlists{
      {600u, MakePlaylist(600u, 21602s,
                          {1100u, 1000u, 1001u, 1002u, 1003u, 1010u, 1011u, 1012u, 1013u, 1014u},
                          {1s, 2400s, 2400s, 2400s, 30s, 4771s, 2400s, 2400s, 2400s, 2400s})},
      {601u, MakePlaylist(601u, 2401s, {1100u, 1000u}, {1s, 2400s})},
      {602u, MakePlaylist(602u, 2401s, {1100u, 1001u}, {1s, 2400s})},
      {603u, MakePlaylist(603u, 2401s, {1100u, 1002u}, {1s, 2400s})},
      {604u, MakePlaylist(604u, 4802s, {1100u, 1003u, 1010u}, {1s, 30s, 4771s})}, // Double episode
      {605u, MakePlaylist(605u, 2401s, {1100u, 1011u}, {1s, 2400s})},
      {606u, MakePlaylist(606u, 2401s, {1100u, 1012u}, {1s, 2400s})},
      {607u, MakePlaylist(607u, 2401s, {1100u, 1013u}, {1s, 2400s})},
      {608u, MakePlaylist(608u, 2401s, {1100u, 1014u}, {1s, 2400s})},
      {1004u, MakePlaylist(1004u, 1s, {1100u}, {1s})},
      {1006u, MakePlaylist(1006u, 30s, {1003u}, {30s})},
  };
  ClipMap clips{
      {1000u, MakeClip(2400s, {600u, 601u})},
      {1001u, MakeClip(2400s, {600u, 602u})},
      {1002u, MakeClip(2400s, {600u, 603u})},
      {1003u, MakeClip(30s, {600u, 604u, 1006u})},
      {1010u, MakeClip(4771s, {600u, 604u})},
      {1011u, MakeClip(2400s, {600u, 605u})},
      {1012u, MakeClip(2400s, {600u, 606u})},
      {1013u, MakeClip(2400s, {600u, 607u})},
      {1014u, MakeClip(2400s, {600u, 608u})},
      {1100u, MakeClip(1s, {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u, 1004u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 9> expectedPlaylists{601u, 602u, 603u, 604u, 604u,
                                                                 605u, 606u, 607u, 608u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(expectedPlaylists.size());
       ++episodeIndex)
  {
    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1);
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), expectedPlaylists[episodeIndex]);
  }

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 9, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(),
            8); // 604 is only returned once as it's the same playlist for episodes 4 and 5
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 11);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u, 1004u, 1006u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601 = episode 1; 602 = episodes 2-5 (a feature length cut of a
// four part story)
// The same four episodes are also present individually as playlists 251-254, but those are English
// only and are not referenced by the play-all playlist, so the 60x group is preferred
// Similar to Avatar the Last Airbender (2005) Bluray S3D3
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_RelaxedPlayAllPlaylist_WithFourEpisodePlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(3, 17, 1500), // 25 minutes
      MakeEpisode(3, 18, 1380), MakeEpisode(3, 19, 1380),
      MakeEpisode(3, 20, 1380), MakeEpisode(3, 21, 1440),
  };

  PlaylistMap playlists{
      {250u, MakePlaylist(250u, 1474s, {1093u, 1062u}, {1s, 1473s})},
      {251u, MakePlaylist(251u, 1464s, {1096u}, {1464s})},
      {252u, MakePlaylist(252u, 1471s, {1097u}, {1471s})},
      {253u, MakePlaylist(253u, 1464s, {1098u}, {1464s})},
      {254u, MakePlaylist(254u, 1472s, {1099u}, {1472s})},
      {255u, MakePlaylist(255u, 1127s, {1100u}, {1127s})},
      {256u, MakePlaylist(256u, 678s, {1101u}, {678s})},
      {257u, MakePlaylist(257u, 2191s, {1102u}, {2191s})},
      {600u, MakePlaylist(600u, 6952s, {1093u, 1062u, 1086u, 1095u}, {1s, 1473s, 47s, 5431s})},
      {601u, MakePlaylist(601u, 1474s, {1093u, 1062u}, {1s, 1473s})},
      {602u, MakePlaylist(602u, 5479s, {1093u, 1086u, 1095u}, {1s, 47s, 5431s})}, // Four episodes
      {1601u, MakePlaylist(1601u, 1473s, {1062u}, {1473s})},
      {1602u, MakePlaylist(1602u, 47s, {1086u}, {47s})},
      {1617u, MakePlaylist(1617u, 1s, {1093u}, {1s})},
      {1619u, MakePlaylist(1619u, 5431s, {1095u}, {5431s})},
  };
  ClipMap clips{
      {1062u, MakeClip(1473s, {250u, 600u, 601u, 1601u})},
      {1086u, MakeClip(47s, {600u, 602u, 1602u})},
      {1093u, MakeClip(1s, {250u, 600u, 601u, 602u, 1617u})},
      {1095u, MakeClip(5431s, {600u, 602u, 1619u})},
      {1096u, MakeClip(1464s, {251u})},
      {1097u, MakeClip(1471s, {252u})},
      {1098u, MakeClip(1464s, {253u})},
      {1099u, MakeClip(1472s, {254u})},
      {1100u, MakeClip(1127s, {255u})},
      {1101u, MakeClip(678s, {256u})},
      {1102u, MakeClip(2191s, {257u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 5> expectedPlaylists{601u, 602u, 602u, 602u, 602u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(expectedPlaylists.size());
       ++episodeIndex)
  {
    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1);
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), expectedPlaylists[episodeIndex]);
  }

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 5, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(),
            2); // 602 is only returned once as it's the same playlist for episodes 18-21
  const auto returned{GetPlaylists(items)};
  const std::set<unsigned int> expected{601u, 602u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As GetEpisodePlaylists_RelaxedPlayAllPlaylist_WithDoubleEpisode2 but only eight episodes on disc
// Fail as the playlists' multiples account for nine episodes, not eight
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_RelaxedPlayAllPlaylist_WithDoubleEpisode_Fail)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2400), // 40 minutes
      MakeEpisode(1, 2, 2400), MakeEpisode(1, 3, 2400), MakeEpisode(1, 4, 2400),
      MakeEpisode(1, 5, 2400), MakeEpisode(1, 6, 2400), MakeEpisode(1, 7, 2400),
      MakeEpisode(1, 8, 2400),
  };

  PlaylistMap playlists{
      {600u, MakePlaylist(600u, 21602s,
                          {1100u, 1000u, 1001u, 1002u, 1003u, 1010u, 1011u, 1012u, 1013u, 1014u},
                          {1s, 2400s, 2400s, 2400s, 30s, 4771s, 2400s, 2400s, 2400s, 2400s})},
      {601u, MakePlaylist(601u, 2401s, {1100u, 1000u}, {1s, 2400s})},
      {602u, MakePlaylist(602u, 2401s, {1100u, 1001u}, {1s, 2400s})},
      {603u, MakePlaylist(603u, 2401s, {1100u, 1002u}, {1s, 2400s})},
      {604u, MakePlaylist(604u, 4802s, {1100u, 1003u, 1010u}, {1s, 30s, 4771s})}, // Double episode
      {605u, MakePlaylist(605u, 2401s, {1100u, 1011u}, {1s, 2400s})},
      {606u, MakePlaylist(606u, 2401s, {1100u, 1012u}, {1s, 2400s})},
      {607u, MakePlaylist(607u, 2401s, {1100u, 1013u}, {1s, 2400s})},
      {608u, MakePlaylist(608u, 2401s, {1100u, 1014u}, {1s, 2400s})},
      {1004u, MakePlaylist(1004u, 1s, {1100u}, {1s})},
      {1006u, MakePlaylist(1006u, 30s, {1003u}, {30s})},
  };
  ClipMap clips{
      {1000u, MakeClip(2400s, {600u, 601u})},
      {1001u, MakeClip(2400s, {600u, 602u})},
      {1002u, MakeClip(2400s, {600u, 603u})},
      {1003u, MakeClip(30s, {600u, 604u, 1006u})},
      {1010u, MakeClip(4771s, {600u, 604u})},
      {1011u, MakeClip(2400s, {600u, 605u})},
      {1012u, MakeClip(2400s, {600u, 606u})},
      {1013u, MakeClip(2400s, {600u, 607u})},
      {1014u, MakeClip(2400s, {600u, 608u})},
      {1100u, MakeClip(1s, {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u, 1004u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 9);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 11);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u, 1004u, 1006u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

//
// ---- GetEpisodePlaylists – multi-episode disc, group method -----------------
//

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// Note episode 1 is significantly longer (but this is allowed as the group playlists = number of episodes)
// (Similar to Firefly S1D1 US Bluray where episode 1 (DVD Order) Serenity is significantly longer)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_LongEpisode)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 8700), // 145 min
      MakeEpisode(1, 2, 2700), // 45 min
      MakeEpisode(1, 3, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 142min, {1u}, {142min})},
      {801u, MakePlaylist(801u, 45min, {2u}, {45min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(142min, {800u})}, {2u, MakeClip(45min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},     {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// One of the playlists is a special
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_WithSpecial)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(0, 1, 1800), // Special
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 10u};
  EXPECT_TRUE(std::ranges::includes(
      returned, expected)); // Any of the 2 remaining playlists could be the special

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// The group is 800-803. The episodes are mapped to the start of the group
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_LongerGroup)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
      {803u, MakePlaylist(803u, 38min, {7u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},   {7u, MakeClip(38min, {803u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// There is an additional 3 playlist group but the playlist 1 is too short so the group is ignored
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_TwoGroupsOneInvalid)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {2u, MakePlaylist(2u, 45min, {6u}, {45min})},
      {3u, MakePlaylist(3u, 45min, {7u}, {45min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},   {6u, MakeClip(45min, {2u})},
      {7u, MakeClip(45min, {3u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {2u, 3u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 7);
  returned = GetPlaylists(items);
  expected = {1u, 2u, 3u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 801 = episode 1; 802 = episode 2
// There is an additional group at 851-852 using the same clips
// (Example - The Last of Us S1D1 UK UHD)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_TwoEpisodes_GroupMethod_TwoGroupsBothValid)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {2u, MakePlaylist(2u, 30min, {5u}, {30min})},
      {3u, MakePlaylist(3u, 30min, {6u}, {30min})},
      {801u, MakePlaylist(801u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {851u, MakePlaylist(851u, 45min, {1u}, {45min})},
      {852u, MakePlaylist(852u, 42min, {2u}, {42min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {801u, 851u})}, {2u, MakeClip(42min, {802u, 852u})},
      {4u, MakeClip(5min, {1u})},          {5u, MakeClip(30min, {2u})},
      {6u, MakeClip(30min, {3u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {2u, 3u, 801u, 802u, 851u, 852u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 7);
  returned = GetPlaylists(items);
  expected = {1u, 2u, 3u, 801u, 802u, 851u, 852u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// Playlist 801 is too small, so failure expected
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_Fail)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 5min, {2u}, {5min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(5min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// The group is 800-803. The episodes should be mapped to the start of the group but
// playlist 801 is long. This is allowed when group playlists = number of episodes but
// as there are 4 playlists in the group they must be within 20% of the desired episode.
// So failure expected
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_Fail2)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 142min, {2u}, {142min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
      {803u, MakePlaylist(803u, 40min, {7u}, {40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(142min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},    {7u, MakeClip(40min, {803u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 4);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// There is no play-all playlist, nor any consecutive groups of playlists (of the correct number)
// There are only n playlists of the appropriate length, so the assumption is these map to episodes
// in ascending numerical order.
// (Example Twisted Metal S1D1 UK UHD)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_FiveEpisodes_GroupMethod_ExactNumberOfPlaylists)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700), MakeEpisode(1, 3, 2700),
      MakeEpisode(1, 4, 2700), MakeEpisode(1, 5, 2700),
  };

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 40min, {1u, 2u}, {40min})},
      {1u, MakePlaylist(1u, 42min, {3u, 2u}, {42min})},
      {2u, MakePlaylist(2u, 45min, {7u, 2u}, {45min})},
      {7u, MakePlaylist(7u, 47min, {10u, 2u}, {47min})},
      {8u, MakePlaylist(8u, 48min, {11u, 2u}, {48min})},
  };
  ClipMap clips{
      {1u, MakeClip(40min, {0u})},  {2u, MakeClip(0min, {0u, 1u, 2u, 7u, 8u})},
      {3u, MakeClip(42min, {1u})},  {7u, MakeClip(45min, {2u})},
      {10u, MakeClip(47min, {7u})}, {11u, MakeClip(48min, {8u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 0);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 2);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 7);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 8);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 5, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 5); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// There is no play-all playlist, nor any consecutive groups of playlists (of the correct number)
// There are only n playlists of the appropriate l length, so the assumption is these map to episodes
// in ascending numerical order.
// Playlist 1 is long, so the playlist collection is rejected
// So failure expected
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_FiveEpisodes_GroupMethod_ExactNumberOfPlaylists_Fail)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700), MakeEpisode(1, 3, 2700),
      MakeEpisode(1, 4, 2700), MakeEpisode(1, 5, 2700),
  };

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 40min, {1u, 2u}, {40min})},
      {1u, MakePlaylist(1u, 142min, {3u, 2u}, {142min})},
      {2u, MakePlaylist(2u, 45min, {7u, 2u}, {45min})},
      {7u, MakePlaylist(7u, 47min, {10u, 2u}, {47min})},
      {8u, MakePlaylist(8u, 48min, {11u, 2u}, {48min})},
  };
  ClipMap clips{
      {1u, MakeClip(40min, {0u})},  {2u, MakeClip(0min, {0u, 1u, 2u, 7u, 8u})},
      {3u, MakeClip(142min, {1u})}, {7u, MakeClip(45min, {2u})},
      {10u, MakeClip(47min, {7u})}, {11u, MakeClip(48min, {8u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// There is no play-all playlist, nor any consecutive groups of playlists (of the correct number)
// There are n long playlists but there is a valid group of shorter playlists that is the same length as the number of episodes
// (Example It Welcome to Derry S1D1 UK UHD)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_LongNumberOfPlaylists)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 2700), // 45 min
                    MakeEpisode(1, 2, 2700), MakeEpisode(1, 3, 2700)};

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 42min, {1u}, {42min})},
      {811u, MakePlaylist(811u, 2min, {7u}, {2min})},
      {812u, MakePlaylist(812u, 3min, {8u}, {3min})},
      {817u, MakePlaylist(817u, 47min, {2u}, {47min})},
      {818u, MakePlaylist(818u, 48min, {3u}, {48min})},
      {820u, MakePlaylist(820u, 12min, {4u}, {12min})},
      {821u, MakePlaylist(821u, 13min, {5u}, {13min})},
      {822u, MakePlaylist(822u, 14min, {6u}, {14min})},
  };
  ClipMap clips{
      {1u, MakeClip(42min, {800u})}, {2u, MakeClip(47min, {817u})}, {3u, MakeClip(48min, {818u})},
      {4u, MakeClip(12min, {820u})}, {5u, MakeClip(13min, {821u})}, {6u, MakeClip(14min, {822u})},
      {7u, MakeClip(2min, {811u})},  {8u, MakeClip(3min, {812u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 817u);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 818u);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 817u, 818u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {800u, 817u, 818u, 820u, 821u, 822u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {800u, 811u, 812u, 817u, 818u, 820u, 821u, 822u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// There is no play-all playlist, nor any consecutive groups of playlists (of the correct number)
// There are n long playlists but there is a valid group of shorter playlists that is the same length as the number of episodes
// In this case the next longest playlist (after the longest 3) is too close in length to the longest playlists, so could be an episode as well
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_ThreeEpisodes_GroupMethod_LongNumberOfPlaylists_Fail)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 2700), // 45 min
                    MakeEpisode(1, 2, 2700), MakeEpisode(1, 3, 2700)};

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 42min, {1u}, {42min})},
      {811u, MakePlaylist(811u, 2min, {7u}, {2min})},
      {812u, MakePlaylist(812u, 3min, {8u}, {3min})},
      {817u, MakePlaylist(817u, 47min, {2u}, {47min})},
      {818u, MakePlaylist(818u, 48min, {3u}, {48min})},
      {820u, MakePlaylist(820u, 12min, {4u}, {12min})},
      {821u, MakePlaylist(821u, 13min, {5u}, {13min})},
      {822u, MakePlaylist(822u, 44min, {6u}, {44min})},
  };
  ClipMap clips{
      {1u, MakeClip(42min, {800u})}, {2u, MakeClip(47min, {817u})}, {3u, MakeClip(48min, {818u})},
      {4u, MakeClip(12min, {820u})}, {5u, MakeClip(13min, {821u})}, {6u, MakeClip(44min, {822u})},
      {7u, MakeClip(2min, {811u})},  {8u, MakeClip(3min, {812u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 6);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected = {800u, 817u, 818u, 820u, 821u, 822u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {800u, 811u, 812u, 817u, 818u, 820u, 821u, 822u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episodes 3 and 4
// 802 is a little shorter than 800 and 801 added together, as the intro/recap/credits that each
// single episode playlist carries appear only once
// (Example The Expanse S1D2 R1 Bluray - episodes 9 and 10 are combined into a single playlist)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_FourEpisodesOneDouble_GroupMethod)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
      MakeEpisode(1, 4, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 85min, {3u}, {85min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(85min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 802);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(
      items.Size(),
      3); // All episode playlists (802 is only returned once as it's the same playlist for episodes 3 and 4)
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episodes 3 and 4
// Playlist 802 is only 1.5x the average episode length so fail expected
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_FourEpisodesOneDouble_GroupMethod_Fail)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700),
      MakeEpisode(1, 3, 2700),
      MakeEpisode(1, 4, 2700),
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 5min, {4u}, {5min})},
      {10u, MakePlaylist(10u, 5min, {5u}, {5min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 57min, {3u}, {57min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(57min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 3);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  EXPECT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

//
// ---- GetMoviePlaylists -------------------------------------------------------
//

TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_EmptyInputs)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{1u, MakePlaylist(1u, 120min, {1u}, {120min})}};
  ClipMap clips{{1u, MakeClip(120min, {1u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, {}, {}));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, {}));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, {}, playlists));
  EXPECT_EQ(items.Size(), 0);
}

// Single playlist above MIN_MOVIE_DURATION (30min)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_SinglePlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{{1u, MakePlaylist(1u, 120min, {1u}, {120min})}};
  ClipMap clips{{1u, MakeClip(120min, {1u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);
}

// All playlists below MIN_MOVIE_DURATION (30min) → false for SINGLE/MAIN, true for ALL
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_AllPlaylistsTooShort)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 20min, {1u}, {20min})},
      {2u, MakePlaylist(2u, 5min, {2u}, {5min})},
  };
  ClipMap clips{
      {1u, MakeClip(20min, {1u})},
      {2u, MakeClip(5min, {2u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  // ALL job skips the duration filter, so short playlists are included
  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// SINGLE returns only the longest; MAIN returns all within 70% of longest.
// Playlists below MIN_MOVIE_DURATION are filtered before either job applies.
// (Example: theatrical + extended cut + short bonus feature)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_MultiplePlaylists)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  // 120min = longest
  //  90min = 75% of longest → included in MAIN (>= 70% threshold of 84min)
  //  80min = 67% of longest → excluded in MAIN (< 84min)
  //   5min = too short      → filtered by MIN_MOVIE_DURATION for SINGLE/MAIN
  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u}, {120min})},
      {2u, MakePlaylist(2u, 90min, {2u}, {90min})},
      {3u, MakePlaylist(3u, 80min, {3u}, {80min})},
      {4u, MakePlaylist(4u, 5min, {4u}, {5min})},
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(90min, {2u})},
      {3u, MakeClip(80min, {3u})},
      {4u, MakeClip(5min, {4u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u); // Longest only

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {1u, 2u, 3u, 4u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Known main playlist (e.g. from disc.inf)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_KnownMainPlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  // Playlist 2 is the known main even though playlist 4 is the longest
  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 125min, {1u}, {125min})},
      {2u, MakePlaylist(2u, 110min, {2u}, {110min})},
      {3u, MakePlaylist(3u, 125min, {3u}, {125min})},
      {4u, MakePlaylist(4u, 135min, {4u}, {135min})},
      {5u, MakePlaylist(5u, 15min, {5u}, {15min})},
  };
  ClipMap clips{
      {1u, MakeClip(125min, {1u})}, {2u, MakeClip(110min, {2u})}, {3u, MakeClip(125min, {3u})},
      {4u, MakeClip(135min, {4u})}, {5u, MakeClip(15min, {5u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 2, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 2u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, 2, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 4);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 2u, 3u, 4u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 2u); // Known main playlist is first
  EXPECT_EQ(GetPlaylistFromPath(items[1]->GetPath()), 4u); // Then descending duration
  EXPECT_EQ(GetPlaylistFromPath(items[2]->GetPath()),
            1u); // When duration equal sort by ascending playlist number
  EXPECT_EQ(GetPlaylistFromPath(items[3]->GetPath()), 3u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, 2, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 2u, 3u, 4u, 5u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 2u); // Known main playlist is first
  EXPECT_EQ(GetPlaylistFromPath(items[1]->GetPath()), 4u); // Then descending duration
  EXPECT_EQ(GetPlaylistFromPath(items[2]->GetPath()),
            1u); // When duration equal sort by ascending playlist number
  EXPECT_EQ(GetPlaylistFromPath(items[3]->GetPath()), 3u);
  EXPECT_EQ(GetPlaylistFromPath(items[4]->GetPath()), 5u);
}

// Known main playlist not present in the map → falls back to standard selection
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_KnownMainPlaylist_NotFound)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u}, {120min})},
      {2u, MakePlaylist(2u, 90min, {2u}, {90min})},
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(90min, {2u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 99, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 99, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, 99, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  returned = GetPlaylists(items);
  expected = {1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// When any playlist has > 1 chapter, playlists with only 1 chapter are discarded
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_ChapterFiltering)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u}, {60min, 60min})}, // 2 chapters
      {2u, MakePlaylist(2u, 115min, {2u}, {115min})}, // 1 chapter
      {3u, MakePlaylist(3u, 5min, {3u}, {5min})}, // Too short
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(115min, {2u})},
      {3u, MakeClip(5min, {3u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  // Playlist 2 (1 chapter) is discarded because playlist 1 has > 1 chapter
  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  const auto returned{GetPlaylists(items)};
  const std::set<unsigned int> expected = {1u, 2u, 3u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// The main playlist is exempt from chapter filtering even when it has only 1 chapter
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_ChapterFiltering_MainPlaylistPreserved)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  // Playlist 1: 2 chapters. mainPlaylist (2): 1 chapter — would normally be erased by the
  // chapter filter, but is preserved because it is the mainPlaylist.
  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u}, {60min, 60min})}, // 2 chapters
      {2u, MakePlaylist(2u, 80min, {2u}, {80min})}, // 1 chapter, mainPlaylist
      {3u, MakePlaylist(3u, 5min, {3u}, {5min})}, // Too short
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(80min, {2u})},
      {3u, MakeClip(5min, {3u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 2, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 2u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, 2, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected = {1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, 2, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {1u, 2u, 3u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}
