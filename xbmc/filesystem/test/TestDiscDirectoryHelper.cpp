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
#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/LanguageTag.h"
#include "utils/StringUtils.h"
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
  }
};

class TestDiscDirectoryHelper : public AdvancedSettingsResetBase
{
};

// A playlist holds where each of its chapters starts, not duration
// Chapters are given here as durations, as a disc describes them and as
// the .mpls parser logs them, and turned into the start times a playlist
// actually holds.
std::vector<std::chrono::milliseconds> MakeChapterStarts(
    const std::vector<std::chrono::milliseconds>& chapterDurations)
{
  std::vector<std::chrono::milliseconds> starts;
  starts.reserve(chapterDurations.size());
  std::chrono::milliseconds start{0ms};
  for (const std::chrono::milliseconds duration : chapterDurations)
  {
    starts.emplace_back(start);
    start += duration;
  }
  return starts;
}

PlaylistInformation MakePlaylist(unsigned int playlist,
                                 std::chrono::milliseconds duration,
                                 std::vector<unsigned int> clips,
                                 const std::vector<std::chrono::milliseconds>& chapterDurations,
                                 std::string languages = "",
                                 std::vector<AudioStreamInfo> audioStreams = {},
                                 std::vector<SubtitleStreamInfo> pgStreams = {},
                                 int height = 0)
{
  PlaylistInformation info;
  info.playlist = playlist;
  info.duration = duration;
  info.clips = std::move(clips);
  info.chapters = MakeChapterStarts(chapterDurations);
  info.languages = std::move(languages);
  info.audioStreams = std::move(audioStreams);
  info.pgStreams = std::move(pgStreams);
  if (height > 0)
  {
    VideoStreamInfo videoStream;
    videoStream.height = height;
    info.videoStreams.emplace_back(videoStream);
  }
  return info;
}

// A channel count of zero means unknown, as it is when a clip's stream map is unavailable
AudioStreamInfo MakeAudioStream(std::string codecName, std::string language, int channels = 0)
{
  AudioStreamInfo info;
  info.valid = true;
  info.codecName = std::move(codecName);
  info.language = KODI::UTILS::CLanguageTag::Parse(language);
  info.channels = channels;
  return info;
}

SubtitleStreamInfo MakeSubtitleStream(std::string language)
{
  SubtitleStreamInfo info;
  info.valid = true;
  info.language = KODI::UTILS::CLanguageTag::Parse(language);
  return info;
}

// For tests only interested in how many streams a playlist exposes
std::vector<AudioStreamInfo> MakeAudioStreams(size_t count)
{
  return std::vector<AudioStreamInfo>(count);
}

std::vector<SubtitleStreamInfo> MakeSubtitleStreams(size_t count)
{
  return std::vector<SubtitleStreamInfo>(count);
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

    // Chapters are where each starts within the playlist, so they run from its beginning, in order,
    // and all begin before it ends
    if (!playlistInformation.chapters.empty())
    {
      if (playlistInformation.chapters.front() != 0ms)
        return false; // First chapter does not start at the beginning of the playlist

      if (!std::ranges::is_sorted(playlistInformation.chapters) ||
          std::ranges::adjacent_find(playlistInformation.chapters) !=
              playlistInformation.chapters.end())
        return false; // Chapters are not in ascending order of where they start

      if (playlistInformation.chapters.back() >= playlistInformation.duration)
        return false; // A chapter starts at or after the end of the playlist
    }
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

  PlaylistMap playlists{{800u, MakePlaylist(800u, 4min, {1u}, {4min})}};
  ClipMap clips{{1u, MakeClip(4min, {800u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, {}, {}));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, {}));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, {}, playlists));
  ASSERT_EQ(items.Size(), 0);
}

TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_AllPlaylistsBelowMinEpisodeDuration)
{
  // A single playlist shorter than MIN_EPISODE_DURATION must not be chosen.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(800u, 4min, {1u}, {4min})}};
  ClipMap clips{{1u, MakeClip(4min, {800u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                             playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 1);
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 1);
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
                        {1u, MakePlaylist(1u, 4min, {2u}, {4min})},
                        {10u, MakePlaylist(10u, 4min, {3u}, {4min})},
                        {100u, MakePlaylist(100u, 4min, {4u}, {4min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})},
                {2u, MakeClip(4min, {1u})},
                {3u, MakeClip(4min, {10u})},
                {4u, MakeClip(4min, {100u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
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
                        {1u, MakePlaylist(1u, 4min, {2u}, {4min})},
                        {10u, MakePlaylist(10u, 4min, {3u}, {4min})},
                        {100u, MakePlaylist(100u, 40min, {4u}, {40min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})},
                {2u, MakeClip(4min, {1u})},
                {3u, MakeClip(4min, {10u})},
                {4u, MakeClip(40min, {100u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
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
                        {1u, MakePlaylist(1u, 6min, {2u}, {6min})},
                        {10u, MakePlaylist(10u, 6min, {3u}, {6min})},
                        {100u, MakePlaylist(100u, 30min, {4u}, {30min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})},
                {2u, MakeClip(6min, {1u})},
                {3u, MakeClip(6min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 6min, {4u}, {6min})},
      {10u, MakePlaylist(10u, 6min, {5u}, {6min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(6min, {1u})},
      {5u, MakeClip(6min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  returned = GetPlaylists(items);
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 128min, {6u, 1u, 2u, 3u}, {3min, 45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(3min, {100u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 128min, {1u, 2u, 3u, 6u}, {45min, 42min, 38min, 3min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(3min, {100u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 7500500ms, {1u, 2u, 3u, 6u}, {45min, 42min, 38min, 500ms})},
      {800u, MakePlaylist(800u, 2700500ms, {1u, 6u}, {45min, 500ms})},
      {802u, MakePlaylist(802u, 2520500ms, {2u, 6u}, {42min, 500ms})},
      {804u, MakePlaylist(804u, 2280500ms, {3u, 6u}, {38min, 500ms})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(500ms, {100u, 800u, 802u, 804u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 4min, {7u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {8u}, {4min})},
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
      {7u, MakeClip(4min, {1u})},
      {8u, MakeClip(4min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 4); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u, 806u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u, 806u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 7);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 131min, {6u, 1u, 2u, 3u, 7u}, {3min, 45min, 42min, 38min, 3min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(3min, {100u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 48min, {1u, 6u}, {45min, 3min})},
      {802u, MakePlaylist(802u, 48min, {7u, 2u, 8u}, {3min, 42min, 3min})},
      {804u, MakePlaylist(804u, 41min, {9u, 3u}, {3min, 38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(3min, {800u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {6u}, {38min})},
      {900u, MakePlaylist(900u, 40min, {7u}, {40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u})},       {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(38min, {804u})},
      {7u, MakeClip(40min, {900u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u, 802u, 804u,
                                  900u}; // 100u included as not a valid play-all playlist
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 7);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 155min, {6u, 1u, 2u, 3u, 7u}, {15min, 45min, 42min, 38min, 15min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {804u, MakePlaylist(804u, 38min, {3u}, {38min})},
      {900u, MakePlaylist(900u, 40min, {8u}, {40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(15min, {100u})},
      {7u, MakeClip(15min, {100u})},       {8u, MakeClip(40min, {900u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u, 802u, 804u,
                                  900u}; // 100u included as not a valid play-all playlist
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 7);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {100u, MakePlaylist(100u, 125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(800u, 48min, {1u, 6u}, {45min, 3min})},
      {802u, MakePlaylist(802u, 60min, {7u, 2u, 8u}, {3min, 42min, 15min})},
      {804u, MakePlaylist(804u, 41min, {9u, 3u}, {3min, 38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(4min, {1u})},
      {5u, MakeClip(4min, {10u})},         {6u, MakeClip(3min, {800u})},
      {7u, MakeClip(3min, {802u})},        {8u, MakeClip(15min, {802u})},
      {9u, MakeClip(3min, {804u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{100u, 800u, 802u,
                                  804u}; // 100u included as not a valid play-all playlist
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 100u, 800u, 802u, 804u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// The disc gathers its four featurettes into a play all playlist (355) as well as its four episodes
// (800-803, which have no play all playlist of their own).
// (Example The Expanse (2015) S3D3 UK Bluray)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_OfExtrasIgnored)
{
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(0, 24), // Featurettes, of unknown length
      MakeEpisode(0, 25),       MakeEpisode(0, 26),       MakeEpisode(0, 27),
      MakeEpisode(3, 10, 2520), // 42 minutes
      MakeEpisode(3, 11, 2640), MakeEpisode(3, 12, 2520), MakeEpisode(3, 13, 2520),
  };

  PlaylistMap playlists{
      {351u, MakePlaylist(351u, 2200s, {301u, 302u}, {2196s, 4s})}, // The featurettes
      {352u, MakePlaylist(352u, 378s, {304u, 305u}, {374s, 4s})},
      {353u, MakePlaylist(353u, 381s, {307u, 308u}, {377s, 4s})},
      {354u, MakePlaylist(354u, 345s, {50609u, 50610u}, {341s, 4s})},
      {355u, MakePlaylist(355u, 3292s, {301u, 304u, 307u, 50609u, 50610u}, // Their play all
                          {2196s, 374s, 377s, 341s, 4s})},
      {800u, MakePlaylist(800u, 2532s, {800u}, {2532s})}, // The episodes
      {801u, MakePlaylist(801u, 2667s, {801u}, {2667s})},
      {802u, MakePlaylist(802u, 2529s, {802u}, {2529s})},
      {803u, MakePlaylist(803u, 2495s, {50606u}, {2495s})},
      // The disc also offers each featurette's clip on its own
      {301u, MakePlaylist(301u, 2196s, {301u}, {2196s})},
      {304u, MakePlaylist(304u, 374s, {304u}, {374s})},
      {307u, MakePlaylist(307u, 377s, {307u}, {377s})},
      {1106u, MakePlaylist(1106u, 341s, {50609u}, {341s})},
      {1107u, MakePlaylist(1107u, 4s, {50610u}, {4s})},
  };
  ClipMap clips{
      {301u, MakeClip(2196s, {301u, 351u, 355u})},
      {302u, MakeClip(4s, {351u})},
      {304u, MakeClip(374s, {304u, 352u, 355u})},
      {305u, MakeClip(4s, {352u})},
      {307u, MakeClip(377s, {307u, 353u, 355u})},
      {308u, MakeClip(4s, {353u})},
      {50609u, MakeClip(341s, {354u, 355u, 1106u})},
      {50610u, MakeClip(4s, {354u, 355u, 1107u})},
      {800u, MakeClip(2532s, {800u})},
      {801u, MakeClip(2667s, {801u})},
      {802u, MakeClip(2529s, {802u})},
      {50606u, MakeClip(2495s, {803u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  // The episodes follow the featurettes in episodesOnDisc, so are at indexes 4 to 7
  static constexpr std::array<unsigned int, 4> EXPECTED_PLAYLISTS{800u, 801u, 802u, 803u};
  for (int episode = 0; episode < static_cast<int>(EXPECTED_PLAYLISTS.size()); ++episode)
  {
    CDiscDirectoryHelper helper;
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episode + 4, episodes, clips, playlists))
        << "episode " << episode + 10;
    ASSERT_EQ(items.Size(), 1) << "episode " << episode + 10;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), EXPECTED_PLAYLISTS[episode]);
  }

  CDiscDirectoryHelper helper;

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 8, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 4); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 13);
  returned = GetPlaylists(items);
  expected = {301u, 304u, 307u, 351u, 352u, 353u, 354u, 355u, 800u, 801u, 802u, 803u, 1106u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 14);
  returned = GetPlaylists(items);
  expected = {301u, 304u, 307u, 351u, 352u, 353u, 354u, 355u, 800u, 801u, 802u, 803u, 1106u, 1107u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As GetEpisodePlaylists_PlayAllPlaylist_OfExtrasIgnored, but the extras gathered into playlist 201
// are of similar lengths to one another, so being near-equal does not tell them from episodes.
// (Example The Last of Us S2D1 Bluray)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_OfSimilarLengthExtrasIgnored)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(2, 1, 3540), // 59 minutes
      MakeEpisode(2, 2, 3420),
      MakeEpisode(2, 3, 3420),
  };

  PlaylistMap playlists{
      // A play all playlist of the three extras, which are of similar lengths to one another
      {201u, MakePlaylist(201u, 1813s, {71u, 72u, 70u}, {533s, 548s, 732s})},
      {202u, MakePlaylist(202u, 533s, {71u}, {533s})},
      {203u, MakePlaylist(203u, 548s, {72u}, {548s})},
      {204u, MakePlaylist(204u, 732s, {70u}, {732s})},
      {207u, MakePlaylist(207u, 673s, {68u}, {673s})},
      {801u, MakePlaylist(801u, 3529s, {63u}, {3529s})}, // The episodes
      {802u, MakePlaylist(802u, 3373s, {65u}, {3373s})},
      {803u, MakePlaylist(803u, 3369s, {66u}, {3369s})},
  };
  ClipMap clips{
      {63u, MakeClip(3529s, {801u})},      {65u, MakeClip(3373s, {802u})},
      {66u, MakeClip(3369s, {803u})},      {68u, MakeClip(673s, {207u})},
      {70u, MakeClip(732s, {201u, 204u})}, {71u, MakeClip(533s, {201u, 202u})},
      {72u, MakeClip(548s, {201u, 203u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 3> EXPECTED_PLAYLISTS{801u, 802u, 803u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED_PLAYLISTS.size());
       ++episodeIndex)
  {
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips, playlists))
        << "episode index " << episodeIndex;
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), EXPECTED_PLAYLISTS[episodeIndex]);
  }

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {201u, 202u, 203u, 204u, 207u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {201u, 202u, 203u, 204u, 207u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

//
// ---- GetEpisodePlaylists – relaxed play-all playlist method -------------------------
//

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601-608 = episodes 1-8
// Clip layout is more complex and fails PlayAllPlaylist method
// Similar to the Avatar The Last Airbender (2005) Bluray S1D1
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 8); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601-608 = episodes 1-8
// Playlist 250 has the same duration and clips as episode playlist 605, but a different
// set of languages - so both should be offered for episode 5
// Similar to Avatar The Last Airbender (2005) Bluray S2D2
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_RelaxedPlayAllPlaylist_IdenticalPlaylist)
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
      {250u, MakePlaylist(250u, 2431s, {1100u, 1007u, 1008u}, {1s, 30s, 2400s}, "eng")},
      {600u, MakePlaylist(600u, 19411s,
                          {1100u, 1000u, 1001u, 1002u, 1003u, 1004u, 1005u, 1006u, 1007u, 1008u,
                           1009u, 1010u, 1011u, 1012u, 1013u, 1014u},
                          {1s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s, 30s, 2400s,
                           30s, 2400s, 30s, 2400s})},
      {601u, MakePlaylist(601u, 2401s, {1100u, 1000u}, {1s, 2400s})},
      {602u, MakePlaylist(602u, 2431s, {1100u, 1001u, 1002u}, {1s, 30s, 2400s})},
      {603u, MakePlaylist(603u, 2431s, {1100u, 1003u, 1004u}, {1s, 30s, 2400s})},
      {604u, MakePlaylist(604u, 2431s, {1100u, 1005u, 1006u}, {1s, 30s, 2400s})},
      {605u, MakePlaylist(605u, 2431s, {1100u, 1007u, 1008u}, {1s, 30s, 2400s}, "eng,spa,fra")},
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
      {1007u, MakeClip(30s, {250u, 600u, 605u})},
      {1008u, MakeClip(2400s, {250u, 600u, 605u})},
      {1009u, MakeClip(30s, {600u, 606u})},
      {1010u, MakeClip(2400s, {600u, 606u})},
      {1011u, MakeClip(30s, {600u, 607u})},
      {1012u, MakeClip(2400s, {600u, 607u})},
      {1013u, MakeClip(30s, {600u, 608u})},
      {1014u, MakeClip(2400s, {600u, 608u})},
      {1100u, MakeClip(1s, {250u, 600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  // Episode 5 (playlist 605) has two playlists - 605 and its identical (different languages) twin 250
  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  // Most languages first
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 605u);
  EXPECT_EQ(GetPlaylistFromPath(items[1]->GetPath()), 250u);

  // Other episodes are unaffected
  static constexpr std::array<unsigned int, 8> expectedPlaylists{601u, 602u, 603u, 604u,
                                                                 605u, 606u, 607u, 608u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(expectedPlaylists.size());
       ++episodeIndex)
  {
    if (episodeIndex == 4)
      continue; // Two playlists, checked above

    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1);
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), expectedPlaylists[episodeIndex]);
  }

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 9); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{250u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 10);
  returned = GetPlaylists(items);
  expected = {250u, 600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 10);
  returned = GetPlaylists(items);
  expected = {250u, 600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

namespace
{
// The disc of the GetEpisodePlaylists_IdenticalPlaylist_* tests below.
// Playlist 600 = play-all; 601-603 = episodes 1-3
// Playlist 650 has the same duration and clip as episode playlist 602, differing only in the
// streams it offers. episodeAudioStreams overrides those of 602 where a test needs them to differ.
PlaylistMap MakeIdenticalPlaylistDisc(PlaylistInformation twin,
                                      std::vector<AudioStreamInfo> episodeAudioStreams = {})
{
  if (episodeAudioStreams.empty())
    episodeAudioStreams = {MakeAudioStream("truehd", "eng"), MakeAudioStream("ac3", "spa"),
                           MakeAudioStream("ac3", "fra")};

  return PlaylistMap{
      {600u, MakePlaylist(600u, 7200s, {1000u, 1001u, 1002u}, {2400s, 2400s, 2400s})},
      {601u, MakePlaylist(601u, 2400s, {1000u}, {2400s})},
      {602u,
       MakePlaylist(602u, 2400s, {1001u}, {2400s}, "eng,spa,fra", std::move(episodeAudioStreams),
                    {MakeSubtitleStream("eng"), MakeSubtitleStream("spa"),
                     MakeSubtitleStream("fra"), MakeSubtitleStream("jpn")})},
      {603u, MakePlaylist(603u, 2400s, {1002u}, {2400s})},
      {650u, std::move(twin)}};
}

ClipMap MakeIdenticalPlaylistDiscClips()
{
  return ClipMap{{1000u, MakeClip(2400s, {600u, 601u})},
                 {1001u, MakeClip(2400s, {600u, 602u, 650u})},
                 {1002u, MakeClip(2400s, {600u, 603u})}};
}

Episodes MakeThreeEpisodes()
{
  return Episodes{MakeEpisode(1, 1, 2400), // 40 minutes
                  MakeEpisode(1, 2, 2400), MakeEpisode(1, 3, 2400)};
}
} // namespace

// Playlist 250 offers only streams that episode playlist 602 also offers - and lists them in a
// different order - so it is a reduced presentation of the same content rather than an alternative,
// and only 602 should be offered for episode 2
// Similar to Dune Prophecy (2024) Bluray S1D3
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_ReducedStreamsIgnored)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(
      MakePlaylist(650u, 2400s, {1001u}, {2400s}, "eng", {MakeAudioStream("truehd", "eng")},
                   {MakeSubtitleStream("jpn"), MakeSubtitleStream("eng")}))};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 602u);

  // Every episode has a single playlist
  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  const std::set<unsigned int> expected{601u, 602u, 603u};
  EXPECT_EQ(GetPlaylists(items), expected);
}

// Playlist 250 offers a Japanese audio stream that episode playlist 602 does not, so it is a
// genuine alternative and both should be offered for episode 2
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_AlternativeStreamsOffered)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(MakePlaylist(650u, 2400s, {1001u}, {2400s}, "jpn",
                                                               {MakeAudioStream("truehd", "jpn")},
                                                               {MakeSubtitleStream("eng")}))};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  // Most languages first
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 602u);
  EXPECT_EQ(GetPlaylistFromPath(items[1]->GetPath()), 650u);
}

// Playlist 650 offers English in a lossier codec than episode playlist 602, and no subtitles, so it
// carries nothing 602 does not and only 602 should be offered for episode 2
// Similar to Avatar the Last Airbender (2005) Bluray S3D3, where the individual episode playlists
// are AC-3 English only against the play-all group's DTS-HD MA English plus Spanish and French
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_LossierCodecIgnored)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(
      MakePlaylist(650u, 2400s, {1001u}, {2400s}, "eng", {MakeAudioStream("ac3", "eng")}))};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 602u);
}

// Playlist 650 offers English in a better codec than episode playlist 602, so it is a genuine
// alternative even though it offers fewer languages, and both should be offered for episode 2
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_BetterCodecOffered)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(MakePlaylist(
      650u, 2400s, {1001u}, {2400s}, "eng", {MakeAudioStream("truehd_atmos", "eng")}))};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const std::set<unsigned int> expected{602u, 650u};
  EXPECT_EQ(GetPlaylists(items), expected);
}

// A playlist offering a language the candidate carries in a better codec, alongside one the
// candidate does not carry at all, is a genuine alternative and both should be offered for
// episode 2
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_LossierCodecExtraLanguage)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(
      MakePlaylist(650u, 2400s, {1001u}, {2400s}, "eng,jpn",
                   {MakeAudioStream("ac3", "eng"), MakeAudioStream("ac3", "jpn")}))};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const std::set<unsigned int> expected{602u, 650u};
  EXPECT_EQ(GetPlaylists(items), expected);
}

// Playlist 650 offers English in the same codec as episode playlist 602 but in fewer channels, so
// it carries nothing 602 does not and only 602 should be offered for episode 2.
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_FewerChannelsIgnored)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(
      MakePlaylist(650u, 2400s, {1001u}, {2400s}, "eng", {MakeAudioStream("truehd", "eng", 2)}),
      {MakeAudioStream("truehd", "eng", 6), MakeAudioStream("ac3", "spa", 6),
       MakeAudioStream("ac3", "fra", 6)})};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 602u);
}

// Playlist 650 offers English in more channels than episode playlist 602, so it is a genuine
// alternative and both should be offered for episode 2
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_MoreChannelsOffered)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(
      MakePlaylist(650u, 2400s, {1001u}, {2400s}, "eng", {MakeAudioStream("truehd", "eng", 8)}),
      {MakeAudioStream("truehd", "eng", 6), MakeAudioStream("ac3", "spa", 6),
       MakeAudioStream("ac3", "fra", 6)})};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const std::set<unsigned int> expected{602u, 650u};
  EXPECT_EQ(GetPlaylists(items), expected);
}

// Playlist 650's channel count is unknown, as it is when a clip's stream map is unavailable, so
// nothing can be concluded from it and only the codec decides - leaving 650 a reduced presentation
// of episode playlist 602
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_UnknownChannelsIgnored)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(
      MakePlaylist(650u, 2400s, {1001u}, {2400s}, "eng", {MakeAudioStream("truehd", "eng")}),
      {MakeAudioStream("truehd", "eng", 6), MakeAudioStream("ac3", "spa", 6),
       MakeAudioStream("ac3", "fra", 6)})};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 602u);
}

// A playlist offering the same stream twice is not a reduced presentation of one offering it once,
// so both should be offered for episode 2
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_IdenticalPlaylist_RepeatedStreamOffered)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{MakeIdenticalPlaylistDisc(
      MakePlaylist(650u, 2400s, {1001u}, {2400s}, "eng,eng",
                   {MakeAudioStream("truehd", "eng"), MakeAudioStream("truehd", "eng")},
                   {MakeSubtitleStream("eng")}))};
  ClipMap clips{MakeIdenticalPlaylistDiscClips()};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const std::set<unsigned int> expected{650u, 602u};
  EXPECT_EQ(GetPlaylists(items), expected);
}

// Where two playlists of the same duration are candidates for one episode, the one with the most
// chapters is preferred. Both offer the same languages, so neither is offered as an alternative to
// the other.
// Playlist 600 = play-all; 601-603 = episodes 1-3; playlist 650 shares episode 2's clip
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_TwoCandidates_MostChaptersPreferred)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  const auto MakeDisc{
      [](std::vector<std::chrono::milliseconds> chapters602,
         std::vector<std::chrono::milliseconds> chapters650)
      {
        return PlaylistMap{
            {600u, MakePlaylist(600u, 7200s, {1000u, 1001u, 1002u}, {2400s, 2400s, 2400s})},
            {601u, MakePlaylist(601u, 2400s, {1000u}, {2400s})},
            {602u, MakePlaylist(602u, 2400s, {1001u}, std::move(chapters602), "eng")},
            {603u, MakePlaylist(603u, 2400s, {1002u}, {2400s})},
            {650u, MakePlaylist(650u, 2400s, {1001u}, std::move(chapters650), "eng")}};
      }};
  ClipMap clips{{1000u, MakeClip(2400s, {600u, 601u})},
                {1001u, MakeClip(2400s, {600u, 602u, 650u})},
                {1002u, MakeClip(2400s, {600u, 603u})}};

  // Playlist 602 has the most chapters
  PlaylistMap playlists{MakeDisc({1200s, 1200s}, {2400s})};
  ASSERT_TRUE(Validate(clips, playlists));
  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 602u);

  // Playlist 650 has the most chapters
  playlists = MakeDisc({2400s}, {1200s, 1200s});
  ASSERT_TRUE(Validate(clips, playlists));
  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 650u);
}

// The group method carries the clips and languages of each grouped playlist, so an identical
// playlist is offered on a disc with no play-all playlist too.
// Playlists 601-603 = episodes 1-3; playlist 650 has the same duration and clip as episode
// playlist 602 but offers a Japanese audio stream that 602 does not
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_GroupMethod_IdenticalPlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeThreeEpisodes()};

  PlaylistMap playlists{
      {601u, MakePlaylist(601u, 2400s, {1000u}, {2400s})},
      {602u, MakePlaylist(602u, 2400s, {1001u}, {2400s}, "eng", {MakeAudioStream("truehd", "eng")},
                          {MakeSubtitleStream("eng")})},
      {603u, MakePlaylist(603u, 2400s, {1002u}, {2400s})},
      {650u, MakePlaylist(650u, 2400s, {1001u}, {2400s}, "jpn", {MakeAudioStream("truehd", "jpn")},
                          {MakeSubtitleStream("eng")})}};
  ClipMap clips{{1000u, MakeClip(2400s, {601u})},
                {1001u, MakeClip(2400s, {602u, 650u})},
                {1002u, MakeClip(2400s, {603u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const std::set<unsigned int> expected{602u, 650u};
  EXPECT_EQ(GetPlaylists(items), expected);
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
      {250u, MakePlaylist(250u, 1475s, {1093u, 1062u}, {1s, 1474s})},
      {251u, MakePlaylist(251u, 1464s, {1096u}, {1464s})},
      {252u, MakePlaylist(252u, 1471s, {1097u}, {1471s})},
      {253u, MakePlaylist(253u, 1464s, {1098u}, {1464s})},
      {254u, MakePlaylist(254u, 1472s, {1099u}, {1472s})},
      {255u, MakePlaylist(255u, 1127s, {1100u}, {1127s})},
      {256u, MakePlaylist(256u, 678s, {1101u}, {678s})},
      {257u, MakePlaylist(257u, 2191s, {1102u}, {2191s})},
      {600u, MakePlaylist(600u, 6953s, {1093u, 1062u, 1086u, 1095u}, {1s, 1474s, 47s, 5431s})},
      {601u, MakePlaylist(601u, 1475s, {1093u, 1062u}, {1s, 1474s})},
      // Four episodes, each marked up with three chapters, then a two chapter credits tail
      {602u, MakePlaylist(602u, 5479s, {1093u, 1086u, 1095u},
                          {516s, 444s, 434s, // Episode 18
                           306s, 546s, 502s, // Episode 19
                           394s, 370s, 581s, // Episode 20
                           499s, 354s, 488s, // Episode 21
                           44s, 1s})}, // Credits
      {1601u, MakePlaylist(1601u, 1474s, {1062u}, {1474s})},
      {1602u, MakePlaylist(1602u, 47s, {1086u}, {47s})},
      {1617u, MakePlaylist(1617u, 1s, {1093u}, {1s})},
      {1619u, MakePlaylist(1619u, 5431s, {1095u}, {5431s})},
  };
  ClipMap clips{
      {1062u, MakeClip(1474s, {250u, 600u, 601u, 1601u})},
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

  // Episode 17 is a playlist of its own, so runs for all of it. Episodes 18-21 share playlist 602,
  // each running for its own three chapters of it, and each after the first carrying an episode
  // bookmark of where it starts.
  struct Expected
  {
    unsigned int playlist;
    int duration;
    double bookmark;
  };
  static constexpr std::array<Expected, 5> EXPECTED{{{601u, 1475, 0.0},
                                                     {602u, 1394, 0.0},
                                                     {602u, 1354, 1394.0},
                                                     {602u, 1345, 2748.0},
                                                     {602u, 1341, 4093.0}}};

  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED.size()); ++episodeIndex)
  {
    const auto& expectedEpisode{EXPECTED[episodeIndex]};

    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), expectedEpisode.playlist);

    const CVideoInfoTag* tag{items[0]->GetVideoInfoTag()};
    EXPECT_EQ(tag->GetDuration(), expectedEpisode.duration) << "episode index " << episodeIndex;
    EXPECT_DOUBLE_EQ(tag->m_EpBookmark.timeInSeconds, expectedEpisode.bookmark)
        << "episode index " << episodeIndex;
    if (expectedEpisode.bookmark > 0.0)
    {
      EXPECT_DOUBLE_EQ(tag->m_EpBookmark.totalTimeInSeconds, 5479.0);
    }
  }

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 5, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(),
            2); // 602 is only returned once as it's the same playlist for episodes 18-21
  const auto returned{GetPlaylists(items)};
  const std::set<unsigned int> expected{601u, 602u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  // Asking for all episodes gives the whole of 602, not one episode of it
  const auto allEpisodeItem{std::ranges::find_if(
      items, [](const auto& i) { return GetPlaylistFromPath(i->GetPath()) == 602u; })};
  ASSERT_NE(allEpisodeItem, items.cend());
  EXPECT_EQ((*allEpisodeItem)->GetVideoInfoTag()->GetDuration(), 5479);
  EXPECT_DOUBLE_EQ((*allEpisodeItem)->GetVideoInfoTag()->m_EpBookmark.timeInSeconds, 0.0);
}

// As GetEpisodePlaylists_RelaxedPlayAllPlaylist_WithFourEpisodePlaylist, but with the stream
// details a disc's directory supplies. Those describe the whole playlist, so for the episodes
// sharing 602 the duration must be replaced with the episode's own - otherwise every one of them is
// reported as running for the length of all four.
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_RelaxedPlayAllPlaylist_FourEpisodePlaylistStreamDetails)
{
  CURL url("bluray://test/");
  CFileItemList items;
  Episodes episodes{
      MakeEpisode(3, 17, 1500), // 25 minutes
      MakeEpisode(3, 18, 1380), MakeEpisode(3, 19, 1380),
      MakeEpisode(3, 20, 1380), MakeEpisode(3, 21, 1440),
  };

  PlaylistMap playlists{
      {600u, MakePlaylist(600u, 6953s, {1093u, 1062u, 1086u, 1095u}, {1s, 1474s, 47s, 5431s})},
      {601u, MakePlaylist(601u, 1475s, {1093u, 1062u}, {1s, 1474s})},
      {602u, MakePlaylist(602u, 5479s, {1093u, 1086u, 1095u},
                          {516s, 444s, 434s, 306s, 546s, 502s, 394s, 370s, 581s, 499s, 354s, 488s,
                           44s, 1s})},
  };
  ClipMap clips{
      {1062u, MakeClip(1474s, {600u, 601u})},
      {1086u, MakeClip(47s, {600u, 602u})},
      {1093u, MakeClip(1s, {600u, 601u, 602u})},
      {1095u, MakeClip(5431s, {600u, 602u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  // Every playlist on the disc, as CBlurayDirectory supplies them
  CFileItemList allTitles;
  for (const unsigned int playlist : std::views::keys(playlists))
  {
    allTitles.Add(std::make_shared<CFileItem>(
        StringUtils::Format("bluray://test/BDMV/PLAYLIST/{:05}.mpls", playlist), false));
  }

  // Describes the streams of a whole playlist, as CBlurayDirectory::SetStreamDetails does
  CDiscDirectoryHelper helper{
      [&playlists](unsigned int playlist, CFileItem& item)
      {
        VideoStreamInfo video;
        video.valid = true;
        const auto duration{static_cast<int>(playlists.at(playlist).duration.count() / 1000)};
        item.GetVideoInfoTag()->m_streamDetails.SetStreams(
            video, duration, AudioStreamInfo{}, SubtitleStreamInfo{}, CStreamDetail::MEDIA);
      }};

  // Episode 17 has playlist 601 to itself, so runs for all of it. Episodes 18-21 share 602 and each
  // runs for its own three chapters of it.
  static constexpr std::array<int, 5> EXPECTED_DURATIONS{1475, 1394, 1354, 1345, 1341};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED_DURATIONS.size());
       ++episodeIndex)
  {
    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;

    const CVideoInfoTag* tag{items[0]->GetVideoInfoTag()};
    EXPECT_EQ(tag->m_streamDetails.GetVideoDuration(), EXPECTED_DURATIONS[episodeIndex])
        << "episode index " << episodeIndex;
    EXPECT_EQ(tag->GetDuration(), static_cast<unsigned int>(EXPECTED_DURATIONS[episodeIndex]))
        << "episode index " << episodeIndex;
  }
}

// As GetEpisodePlaylists_RelaxedPlayAllPlaylist_WithFourEpisodePlaylist, but playlist 602's
// chapters do not divide into four runs of near-equal duration.
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_RelaxedPlayAllPlaylist_FourEpisodePlaylistChaptersDoNotDivide)
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
      {600u, MakePlaylist(600u, 6952s, {1093u, 1062u, 1086u, 1095u}, {1s, 1473s, 47s, 5431s})},
      {601u, MakePlaylist(601u, 1474s, {1093u, 1062u}, {1s, 1473s})},
      // One chapter per episode would divide evenly, but the runs are nothing like equal
      {602u, MakePlaylist(602u, 5479s, {1093u, 1086u, 1095u}, {2500s, 1200s, 1100s, 679s})},
  };
  ClipMap clips{
      {1062u, MakeClip(1473s, {600u, 601u})},
      {1086u, MakeClip(47s, {600u, 602u})},
      {1093u, MakeClip(1s, {600u, 601u, 602u})},
      {1095u, MakeClip(5431s, {600u, 602u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 5> EXPECTED_PLAYLISTS{601u, 602u, 602u, 602u, 602u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED_PLAYLISTS.size());
       ++episodeIndex)
  {
    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), EXPECTED_PLAYLISTS[episodeIndex]);

    // No episode bookmark, and the whole playlist as the duration
    const CVideoInfoTag* tag{items[0]->GetVideoInfoTag()};
    EXPECT_DOUBLE_EQ(tag->m_EpBookmark.timeInSeconds, 0.0) << "episode index " << episodeIndex;
    if (EXPECTED_PLAYLISTS[episodeIndex] == 602u)
    {
      EXPECT_EQ(tag->GetDuration(), 5479) << "episode index " << episodeIndex;
    }
  }
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 600 = play-all; 601 = episode 17; 602 = episode 18; 603 = episodes 19 and 20 as one
// continuous stream, marked up with three chapters each and a two chapter credits tail
// Similar to Avatar the Last Airbender (2005) Bluray S2D3
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_RelaxedPlayAllPlaylist_WithDoubleEpisodeChapters)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(2, 17, 1470), // 24.5 minutes
      MakeEpisode(2, 18, 1470),
      MakeEpisode(2, 19, 1470),
      MakeEpisode(2, 20, 1470),
  };

  PlaylistMap playlists{
      {600u, MakePlaylist(600u, 5894s, {1093u, 1062u, 1086u, 1101u, 1102u},
                          {1s, 1473s, 1473s, 46s, 2901s})},
      {601u, MakePlaylist(601u, 1474s, {1093u, 1062u}, {1s, 1473s})},
      {602u, MakePlaylist(602u, 1474s, {1093u, 1086u}, {1s, 1473s})},
      {603u, MakePlaylist(603u, 2948s, {1093u, 1101u, 1102u},
                          {520s, 540s, 414s, // Episode 19
                           480s, 500s, 448s, // Episode 20
                           45s, 1s})}, // Credits
  };
  ClipMap clips{
      {1062u, MakeClip(1473s, {600u, 601u})},          {1086u, MakeClip(1473s, {600u, 602u})},
      {1093u, MakeClip(1s, {600u, 601u, 602u, 603u})}, {1101u, MakeClip(46s, {600u, 603u})},
      {1102u, MakeClip(2901s, {600u, 603u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  struct Expected
  {
    unsigned int playlist;
    int duration;
    double bookmark;
  };
  static constexpr std::array<Expected, 4> EXPECTED{
      {{601u, 1474, 0.0}, {602u, 1474, 0.0}, {603u, 1474, 0.0}, {603u, 1428, 1474.0}}};

  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED.size()); ++episodeIndex)
  {
    const auto& expectedEpisode{EXPECTED[episodeIndex]};

    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), expectedEpisode.playlist);

    const CVideoInfoTag* tag{items[0]->GetVideoInfoTag()};
    EXPECT_EQ(tag->GetDuration(), expectedEpisode.duration) << "episode index " << episodeIndex;
    EXPECT_DOUBLE_EQ(tag->m_EpBookmark.timeInSeconds, expectedEpisode.bookmark)
        << "episode index " << episodeIndex;
    if (expectedEpisode.bookmark > 0.0)
    {
      EXPECT_DOUBLE_EQ(tag->m_EpBookmark.totalTimeInSeconds, 2948.0);
    }
  }
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 9);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 9);
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 9);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 9);
  returned = GetPlaylists(items);
  expected = {600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 9);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{600u, 601u, 602u, 603u, 604u, 605u, 606u, 607u, 608u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 11);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 142min, {1u}, {142min})},
      {801u, MakePlaylist(801u, 45min, {2u}, {45min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(142min, {800u})}, {2u, MakeClip(45min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(4min, {1u})},     {5u, MakeClip(4min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
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
      {1u, MakePlaylist(1u, 6min, {4u}, {6min})},
      {10u, MakePlaylist(10u, 6min, {5u}, {6min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(6min, {1u})},    {5u, MakeClip(6min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As GetEpisodePlaylists_ThreeEpisodes_GroupMethod_WithSpecial, but the episodes are given with the
// special last rather than first
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_WithSpecialLast)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700), MakeEpisode(1, 3, 2700), MakeEpisode(0, 1, 1800), // Special
  };

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 6min, {4u}, {6min})},
      {10u, MakePlaylist(10u, 6min, {5u}, {6min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(6min, {1u})},    {5u, MakeClip(6min, {10u})},
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

  // The special, at the end of the list rather than the start
  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 10u};
  EXPECT_TRUE(std::ranges::includes(
      returned, expected)); // Any of the 2 remaining playlists could be the special

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlists 20-22 are the episodes, and 30-32 offer the same clips again
// Episodes 11 and 13 are cut to the same length
// (Example Battlestar Galactica (2003) S1D3 Bluray)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_EqualEpisodeDurations)
{
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 11, 3600),
      MakeEpisode(1, 12, 3600),
      MakeEpisode(1, 13, 3600),
  };

  PlaylistMap playlists{
      {20u, MakePlaylist(20u, 2625s, {11u}, {2625s})},
      {21u, MakePlaylist(21u, 2623s, {12u}, {2623s})},
      {22u, MakePlaylist(22u, 2625s, {13u}, {2625s})}, // Same length as episode 11's playlist
      {30u, MakePlaylist(30u, 2625s, {11u}, {2625s})}, // Duplicates of 20-22
      {31u, MakePlaylist(31u, 2623s, {12u}, {2623s})},
      {32u, MakePlaylist(32u, 2625s, {13u}, {2625s})},
      {52u, MakePlaylist(52u, 752s, {14u}, {752s})}, // Extras
      {99u, MakePlaylist(99u, 2908s, {15u}, {2908s})},
  };
  ClipMap clips{
      {11u, MakeClip(2625s, {20u, 30u})}, {12u, MakeClip(2623s, {21u, 31u})},
      {13u, MakeClip(2625s, {22u, 32u})}, {14u, MakeClip(752s, {52u})},
      {15u, MakeClip(2908s, {99u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 3> EXPECTED_PLAYLISTS{20u, 21u, 22u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED_PLAYLISTS.size());
       ++episodeIndex)
  {
    CDiscDirectoryHelper helper;
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips, playlists))
        << "episode index " << episodeIndex;
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), EXPECTED_PLAYLISTS[episodeIndex]);
  }

  CDiscDirectoryHelper helper;

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected = {20u, 21u, 22u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {20u, 21u, 22u, 30u, 31u, 32u, 52u, 99u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {20u, 21u, 22u, 30u, 31u, 32u, 52u, 99u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As GetEpisodePlaylists_ThreeEpisodes_GroupMethod_EqualEpisodeDurations, at the point in a scan
// where the episodes already identified have taken their duration from the disc while the one being
// looked for still has the scraper's hour long slot. The disc then looks inconsistent with itself,
// and the last episode must still be found.
// (Example Battlestar Galactica (2003) S1D3 Bluray, scanning the last episode of the disc)
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_ThreeEpisodes_GroupMethod_PartlyMeasuredDurations)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 11, 2625), // Measured from the disc when these were identified
      MakeEpisode(1, 12, 2623), MakeEpisode(1, 13, 3600), // Still the scraper's hour long slot
  };

  PlaylistMap playlists{
      {20u, MakePlaylist(20u, 2625s, {11u}, {2625s})},
      {21u, MakePlaylist(21u, 2623s, {12u}, {2623s})},
      {22u, MakePlaylist(22u, 2625s, {13u}, {2625s})},
      {30u, MakePlaylist(30u, 2625s, {11u}, {2625s})}, // Duplicates of 20-22
      {31u, MakePlaylist(31u, 2623s, {12u}, {2623s})},
      {32u, MakePlaylist(32u, 2625s, {13u}, {2625s})},
      {52u, MakePlaylist(52u, 752s, {14u}, {752s})}, // Extras, one of them nearer the hour
      {99u, MakePlaylist(99u, 2908s, {15u}, {2908s})},
  };
  ClipMap clips{
      {11u, MakeClip(2625s, {20u, 30u})}, {12u, MakeClip(2623s, {21u, 31u})},
      {13u, MakeClip(2625s, {22u, 32u})}, {14u, MakeClip(752s, {52u})},
      {15u, MakeClip(2908s, {99u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 20u);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 21u);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 22u);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 3, episodes, clips,
                                          playlists)); // Invalid episode index
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected = {20u, 21u, 22u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {20u, 21u, 22u, 30u, 31u, 32u, 52u, 99u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {20u, 21u, 22u, 30u, 31u, 32u, 52u, 99u};
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
      {803u, MakePlaylist(803u, 38min, {7u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},   {7u, MakeClip(38min, {803u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {2u, MakePlaylist(2u, 45min, {6u}, {45min})},
      {3u, MakePlaylist(3u, 45min, {7u}, {45min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},   {6u, MakeClip(45min, {2u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {2u, 3u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 7);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {2u, MakePlaylist(2u, 30min, {5u}, {30min})},
      {3u, MakePlaylist(3u, 30min, {6u}, {30min})},
      {801u, MakePlaylist(801u, 45min, {1u}, {45min})},
      {802u, MakePlaylist(802u, 42min, {2u}, {42min})},
      {851u, MakePlaylist(851u, 45min, {1u}, {45min})},
      {852u, MakePlaylist(852u, 42min, {2u}, {42min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {801u, 851u})}, {2u, MakeClip(42min, {802u, 852u})},
      {4u, MakeClip(4min, {1u})},          {5u, MakeClip(30min, {2u})},
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {2u, 3u, 801u, 802u, 851u, 852u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 7);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 4min, {2u}, {4min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(4min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 142min, {2u}, {142min})},
      {802u, MakePlaylist(802u, 38min, {3u}, {38min})},
      {803u, MakePlaylist(803u, 40min, {7u}, {40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(142min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},    {7u, MakeClip(40min, {803u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 4);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u, 803u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 5); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As GetEpisodePlaylists_FiveEpisodes_GroupMethod_ExactNumberOfPlaylists
// Simulates the disc scanning process where the durations of the episodes are only known after they have been scanned
// Playlist 7 is shorter, but within tolerance
// (Example Twisted Metal S1D2 UK UHD)
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_FiveEpisodes_GroupMethod_ExactNumberOfPlaylists_PartialDurations)
{
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  static constexpr std::array<unsigned int, 5> DURATIONS{1740, 1920, 1740, 1440, 1980};

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 1702s, {0u, 2u}, {1702s})},
      {1u, MakePlaylist(1u, 1886s, {3u, 2u}, {1886s})},
      {2u, MakePlaylist(2u, 1697s, {7u, 2u}, {1697s})},
      {7u, MakePlaylist(7u, 1422s, {10u, 2u}, {1422s})},
      {8u, MakePlaylist(8u, 1952s, {11u, 2u}, {1952s})},
  };
  ClipMap clips{
      {0u, MakeClip(1702s, {0u})},  {2u, MakeClip(0min, {0u, 1u, 2u, 7u, 8u})},
      {3u, MakeClip(1886s, {1u})},  {7u, MakeClip(1697s, {2u})},
      {10u, MakeClip(1422s, {7u})}, {11u, MakeClip(1952s, {8u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 5> EXPECTED_PLAYLISTS{0u, 1u, 2u, 7u, 8u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(DURATIONS.size()); ++episodeIndex)
  {
    // Only the durations of the episodes scanned so far are known
    Episodes episodes;
    for (int i = 0; i < static_cast<int>(DURATIONS.size()); ++i)
      episodes.emplace_back(MakeEpisode(1, i + 1, i <= episodeIndex ? DURATIONS[i] : 0));

    CDiscDirectoryHelper helper;
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips, playlists))
        << "episode index " << episodeIndex;
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), EXPECTED_PLAYLISTS[episodeIndex]);
  }
}

// As GetEpisodePlaylists_FiveEpisodes_GroupMethod_ExactNumberOfPlaylists_PartialDurations, but the
// last episode is feature length
// (Example The Expanse S6D2 UK Bluray - episode 6 is the feature length finale)
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_ThreeEpisodes_GroupMethod_ExactNumberOfPlaylists_LongFinale)
{
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  static constexpr std::array<unsigned int, 3> DURATIONS{2820, 2880, 3780};

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 27s, {6u}, {27s})}, // Too short to be an episode
      {1u, MakePlaylist(1u, 2829s, {2u}, {2829s})}, {3u, MakePlaylist(3u, 2886s, {3u}, {2886s})},
      {4u, MakePlaylist(4u, 3815s, {4u}, {3815s})}, // The finale
      {7u, MakePlaylist(7u, 16s, {7u}, {16s})},
  };
  ClipMap clips{
      {2u, MakeClip(2829s, {1u})}, {3u, MakeClip(2886s, {3u})}, {4u, MakeClip(3815s, {4u})},
      {6u, MakeClip(27s, {0u})},   {7u, MakeClip(16s, {7u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 3> EXPECTED_PLAYLISTS{1u, 3u, 4u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(DURATIONS.size()); ++episodeIndex)
  {
    // Only the durations of the episodes scanned so far are known
    Episodes episodes;
    for (int i = 0; i < static_cast<int>(DURATIONS.size()); ++i)
      episodes.emplace_back(MakeEpisode(6, i + 4, i <= episodeIndex ? DURATIONS[i] : 0));

    CDiscDirectoryHelper helper;
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips, playlists))
        << "episode index " << episodeIndex;
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), EXPECTED_PLAYLISTS[episodeIndex]);
  }

  CDiscDirectoryHelper helper;
  Episodes episodes;
  for (int i = 0; i < static_cast<int>(DURATIONS.size()); ++i)
    episodes.emplace_back(MakeEpisode(1, i + 1, DURATIONS[i]));

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 3u, 4u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {1u, 3u, 4u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {0u, 1u, 3u, 4u, 7u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// There is no play-all playlist, nor any consecutive groups of playlists (of the correct number)
// There is one more playlist long enough to be an episode than there are episodes - playlist 810 is
// a feature length extra - so the one whose duration is nothing like an episode's is removed, and
// the rest map to episodes in ascending numerical order
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_FiveEpisodes_GroupMethod_ExactNumberOfPlaylists_LongExtraRemoved)
{
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{
      MakeEpisode(1, 1, 2700), // 45 min
      MakeEpisode(1, 2, 2700), MakeEpisode(1, 3, 2700),
      MakeEpisode(1, 4, 2700), MakeEpisode(1, 5, 2700),
  };

  // Non-consecutive, so there is no group of playlists to use
  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 2700s, {1u}, {2700s})},
      {802u, MakePlaylist(802u, 2640s, {2u}, {2640s})},
      {804u, MakePlaylist(804u, 2760s, {3u}, {2760s})},
      {806u, MakePlaylist(806u, 2700s, {4u}, {2700s})},
      {808u, MakePlaylist(808u, 2580s, {5u}, {2580s})},
      {810u, MakePlaylist(810u, 6000s, {6u}, {6000s})}, // Feature length extra
  };
  ClipMap clips{
      {1u, MakeClip(2700s, {800u})}, {2u, MakeClip(2640s, {802u})}, {3u, MakeClip(2760s, {804u})},
      {4u, MakeClip(2700s, {806u})}, {5u, MakeClip(2580s, {808u})}, {6u, MakeClip(6000s, {810u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  static constexpr std::array<unsigned int, 5> EXPECTED_PLAYLISTS{800u, 802u, 804u, 806u, 808u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED_PLAYLISTS.size());
       ++episodeIndex)
  {
    CDiscDirectoryHelper helper;
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips, playlists))
        << "episode index " << episodeIndex;
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), EXPECTED_PLAYLISTS[episodeIndex]);
  }

  CDiscDirectoryHelper helper;

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 5); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 802u, 804u, 806u, 808u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u, 806u, 808u, 810u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {800u, 802u, 804u, 806u, 808u, 810u};
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {0u, 1u, 2u, 7u, 8u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// The disc has a single long playlist holding one clip per episode and no individual episode
// playlists, so every episode resolves to that playlist. Each episode after the first carries an
// episode bookmark giving the start of its clip.
// (Example Tin Man S1D1 UK BD)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_SingleEpisodeClipsPlaylistMethod)
{
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 5400), // 90 min
                    MakeEpisode(1, 2, 5400), MakeEpisode(1, 3, 5400)};

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 16065s, {0u, 1u, 2u}, {5492s, 5262s, 5311s})},
      {1u, MakePlaylist(1u, 16s, {4u, 5u}, {10s, 6s})}, // Logo/trailer
  };
  ClipMap clips{
      {0u, MakeClip(5492s, {0u})}, {1u, MakeClip(5262s, {0u})}, {2u, MakeClip(5311s, {0u})},
      {4u, MakeClip(10s, {1u})},   {5u, MakeClip(6s, {1u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  // Each episode is the same playlist, offset by the durations of the clips before it
  static constexpr std::array<std::pair<unsigned int, double>, 3> EXPECTED{
      {{5492u, 0.0}, {5262u, 5492.0}, {5311u, 10754.0}}};

  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED.size()); ++episodeIndex)
  {
    const auto& [expectedDuration, expectedStart]{EXPECTED[episodeIndex]};

    CDiscDirectoryHelper helper;
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips, playlists))
        << "episode index " << episodeIndex;
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 0u);

    // The episode's duration is its own clip, not the whole playlist
    const CVideoInfoTag* tag{items[0]->GetVideoInfoTag()};
    EXPECT_EQ(tag->GetDuration(), static_cast<int>(expectedDuration));

    // The first episode starts at zero so needs no bookmark
    EXPECT_DOUBLE_EQ(tag->m_EpBookmark.timeInSeconds, expectedStart);
    if (expectedStart > 0.0)
    {
      EXPECT_DOUBLE_EQ(tag->m_EpBookmark.totalTimeInSeconds, 16065.0);
    }
  }

  CDiscDirectoryHelper helper;

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{0u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 1);
  returned = GetPlaylists(items);
  expected = {0u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 2);
  returned = GetPlaylists(items);
  expected = {0u, 1u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As above, but a clip does not match the duration of the episode in its position, so the disc
// does not fit the single-playlist assumption and no playlist is returned.
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_ThreeEpisodes_SingleEpisodeClipsPlaylistMethod_ClipDurationMismatch)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 5400), // 90 min
                    MakeEpisode(1, 2, 5400), MakeEpisode(1, 3, 5400)};

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 14265s, {0u, 1u, 2u}, {5492s, 5262s, 3511s})}, // Clip 2 far too short
      {1u, MakePlaylist(1u, 16s, {4u, 5u}, {10s, 6s})},
  };
  ClipMap clips{
      {0u, MakeClip(5492s, {0u})}, {1u, MakeClip(5262s, {0u})}, {2u, MakeClip(3511s, {0u})},
      {4u, MakeClip(10s, {1u})},   {5u, MakeClip(6s, {1u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 1);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected = {0u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 2);
  returned = GetPlaylists(items);
  expected = {0u, 1u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As above, but the playlist has more clips than there are episodes on the disc, so which clip is
// which episode cannot be determined and no playlist is returned.
TEST_F(TestDiscDirectoryHelper,
       GetEpisodePlaylists_ThreeEpisodes_SingleEpisodeClipsPlaylistMethod_TooManyClips)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 5400), // 90 min
                    MakeEpisode(1, 2, 5400), MakeEpisode(1, 3, 5400)};

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 21465s, {0u, 1u, 2u, 3u}, {5492s, 5262s, 5311s, 5400s})},
      {1u, MakePlaylist(1u, 16s, {4u, 5u}, {10s, 6s})},
  };
  ClipMap clips{
      {0u, MakeClip(5492s, {0u})}, {1u, MakeClip(5262s, {0u})}, {2u, MakeClip(5311s, {0u})},
      {3u, MakeClip(5400s, {0u})}, {4u, MakeClip(10s, {1u})},   {5u, MakeClip(6s, {1u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 1);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected = {0u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 2);
  returned = GetPlaylists(items);
  expected = {0u, 1u};
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 817u, 818u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
  returned = GetPlaylists(items);
  expected = {800u, 817u, 818u, 820u, 821u, 822u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 6);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected = {800u, 817u, 818u, 820u, 821u, 822u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {800u, 811u, 812u, 817u, 818u, 820u, 821u, 822u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// The disc offers each of its two episodes three times over (800/814/816 and 807/815/817), and has
// two extras that happen to be consecutively numbered (820 and 821, and again as 824 and 825).
// A run of two extras is as many playlists as there are episodes, so their multiples add up, but
// they are minutes long where an episode is an hour - the group is not the episodes.
// (Example Dune Prophecy S1D1 UK Bluray)
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_TwoEpisodes_GroupsWithMultiples_ExtrasRejected)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3960), MakeEpisode(1, 2, 3780)}; // 66 and 63 minutes

  static const std::string ALL_LANGS{"eng,fra,deu,ita,spa,ces"};
  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 3945s, {75u}, {3945s}, ALL_LANGS)}, // Episode 1
      {807u, MakePlaylist(807u, 3767s, {76u}, {3767s}, ALL_LANGS)}, // Episode 2
      {812u, MakePlaylist(812u, 356s, {81u}, {356s}, "eng")}, // Extras
      {813u, MakePlaylist(813u, 324s, {82u}, {324s}, "eng")},
      {814u, MakePlaylist(814u, 3945s, {75u}, {3945s}, ALL_LANGS)}, // The episodes again
      {815u, MakePlaylist(815u, 3767s, {76u}, {3767s}, ALL_LANGS)},
      {816u, MakePlaylist(816u, 3945s, {75u}, {3945s}, "eng")},
      {817u, MakePlaylist(817u, 3767s, {76u}, {3767s}, "eng")},
      {820u, MakePlaylist(820u, 356s, {81u}, {356s}, "eng")}, // And the extras again
      {821u, MakePlaylist(821u, 324s, {82u}, {324s}, "eng")},
      {824u, MakePlaylist(824u, 356s, {81u}, {356s}, "eng")},
      {825u, MakePlaylist(825u, 324s, {82u}, {324s}, "eng")},
  };
  ClipMap clips{
      {75u, MakeClip(3945s, {800u, 814u, 816u})},
      {76u, MakeClip(3767s, {807u, 815u, 817u})},
      {81u, MakeClip(356s, {812u, 820u, 824u})},
      {82u, MakeClip(324s, {813u, 821u, 825u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  // Each episode gets the playlist offering its own clip, and never an extra. Which of the copies
  // is offered is not important, so they are identified by the clip they hold.
  static constexpr std::array<unsigned int, 2> EXPECTED_CLIPS{75u, 76u};
  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED_CLIPS.size()); ++episodeIndex)
  {
    EXPECT_TRUE(
        helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips, playlists))
        << "episode index " << episodeIndex;
    ASSERT_GT(items.Size(), 0) << "episode index " << episodeIndex;

    for (const auto& item : items)
    {
      const unsigned int playlist{GetPlaylistFromPath(item->GetPath())};
      ASSERT_TRUE(playlists.contains(playlist));
      const std::vector<unsigned int> expectedClips{EXPECTED_CLIPS[episodeIndex]};
      EXPECT_EQ(playlists.at(playlist).clips, expectedClips)
          << "episode index " << episodeIndex << " given playlist " << playlist;
    }
  }
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 85min, {3u}, {85min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(85min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},
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
  ASSERT_EQ(items.Size(), 0);

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
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
  returned = GetPlaylists(items);
  expected = {1u, 10u, 800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// As GetEpisodePlaylists_FourEpisodesOneDouble_GroupMethod, but the double episode playlist 802 is
// marked up with three chapters per episode and a single credits chapter, so where episode 4 starts
// within it can be told
TEST_F(TestDiscDirectoryHelper, GetEpisodePlaylists_FourEpisodesOneDouble_GroupMethod_Chapters)
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 85min, {3u},
                          {900s, 850s, 800s, // Episode 3
                           880s, 870s, 780s, // Episode 4
                           20s})}, // Credits
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(85min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  struct Expected
  {
    unsigned int playlist;
    int duration;
    double bookmark;
  };
  static constexpr std::array<Expected, 4> EXPECTED{
      {{800u, 2700, 0.0}, {801u, 2520, 0.0}, {802u, 2550, 0.0}, {802u, 2530, 2550.0}}};

  for (int episodeIndex = 0; episodeIndex < static_cast<int>(EXPECTED.size()); ++episodeIndex)
  {
    const auto& expectedEpisode{EXPECTED[episodeIndex]};

    EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, episodeIndex, episodes, clips,
                                           playlists));
    ASSERT_EQ(items.Size(), 1) << "episode index " << episodeIndex;
    EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), expectedEpisode.playlist);

    const CVideoInfoTag* tag{items[0]->GetVideoInfoTag()};
    EXPECT_EQ(tag->GetDuration(), expectedEpisode.duration) << "episode index " << episodeIndex;
    EXPECT_DOUBLE_EQ(tag->m_EpBookmark.timeInSeconds, expectedEpisode.bookmark)
        << "episode index " << episodeIndex;
    if (expectedEpisode.bookmark > 0.0)
    {
      EXPECT_DOUBLE_EQ(tag->m_EpBookmark.totalTimeInSeconds, 5100.0);
    }
  }

  // Asking for all episodes gives the whole of 802, not one episode of it
  EXPECT_TRUE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  const auto allEpisodeItem{std::ranges::find_if(
      items, [](const auto& i) { return GetPlaylistFromPath(i->GetPath()) == 802u; })};
  ASSERT_NE(allEpisodeItem, items.cend());
  EXPECT_EQ((*allEpisodeItem)->GetVideoInfoTag()->GetDuration(), 5100);
  EXPECT_DOUBLE_EQ((*allEpisodeItem)->GetVideoInfoTag()->m_EpBookmark.timeInSeconds, 0.0);
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
      {1u, MakePlaylist(1u, 4min, {4u}, {4min})},
      {10u, MakePlaylist(10u, 4min, {5u}, {4min})},
      {800u, MakePlaylist(800u, 45min, {1u}, {45min})},
      {801u, MakePlaylist(801u, 42min, {2u}, {42min})},
      {802u, MakePlaylist(802u, 57min, {3u}, {57min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(57min, {802u})},
      {4u, MakeClip(4min, {1u})},    {5u, MakeClip(4min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetEpisodePlaylists(url, items, allTitles, ALL_PLAYLISTS, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::MAIN, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 3);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u, 802u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(helper.GetAllEpisodePlaylists(url, items, allTitles, GetTitle::ALL, episodes, clips,
                                            playlists));
  ASSERT_EQ(items.Size(), 5);
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
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, {}));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, {}, playlists));
  ASSERT_EQ(items.Size(), 0);
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
      {2u, MakePlaylist(2u, 4min, {2u}, {4min})},
  };
  ClipMap clips{
      {1u, MakeClip(20min, {1u})},
      {2u, MakeClip(4min, {2u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

  EXPECT_FALSE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 0);

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
  //   4min = too short      → filtered by MIN_MOVIE_DURATION for SINGLE/MAIN
  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u}, {120min})},
      {2u, MakePlaylist(2u, 90min, {2u}, {90min})},
      {3u, MakePlaylist(3u, 80min, {3u}, {80min})},
      {4u, MakePlaylist(4u, 4min, {4u}, {4min})},
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(90min, {2u})},
      {3u, MakeClip(80min, {3u})},
      {4u, MakeClip(4min, {4u})},
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
      {3u, MakePlaylist(3u, 4min, {3u}, {4min})}, // Too short
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(115min, {2u})},
      {3u, MakeClip(4min, {3u})},
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
      {3u, MakePlaylist(3u, 4min, {3u}, {4min})}, // Too short
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(80min, {2u})},
      {3u, MakeClip(4min, {3u})},
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

//
// ---- GetMoviePlaylists - duplicate presentations ------------------------------
//

// A disc may present the same content through several playlists, each exposing a different set of
// streams. The richest presentation is the one kept, wherever it appears in the list.
// (Example: 28 Days Later)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_RemovesDuplicatesKeepingTheRichest)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {120u, MakePlaylist(120u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(3),
                          MakeSubtitleStreams(6))},
      {121u, MakePlaylist(121u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(3),
                          MakeSubtitleStreams(4))},
      {122u, MakePlaylist(122u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(2),
                          MakeSubtitleStreams(2))},
  };
  ClipMap clips{{196u, MakeClip(120min, {120u, 121u, 122u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 120u);

  // More audio streams wins over more subtitle streams, whatever the playlist order
  playlists = {
      {800u, MakePlaylist(800u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(9),
                          MakeSubtitleStreams(0))},
      {805u, MakePlaylist(805u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(1),
                          MakeSubtitleStreams(8))},
  };
  clips = {{196u, MakeClip(120min, {800u, 805u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);
}

// The playlist the disc names as the main one is the copy kept, even where another copy of the
// same presentation exposes more streams, as the rest of the search identifies the movie by it.
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_RemovesDuplicatesKeepingTheMainPlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  // Playlist 121 (mainPlaylist) presents the same content as 120 and 122, but less fully than
  // either of them
  PlaylistMap playlists{
      {120u, MakePlaylist(120u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(3),
                          MakeSubtitleStreams(6))},
      {121u, MakePlaylist(121u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(1),
                          MakeSubtitleStreams(1))}, // mainPlaylist
      {122u, MakePlaylist(122u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(2),
                          MakeSubtitleStreams(2))},
  };
  ClipMap clips{{196u, MakeClip(120min, {120u, 121u, 122u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 121, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 121u);

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 121, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 121u);

  // The main playlist is kept wherever it appears in the list
  playlists = {
      {800u, MakePlaylist(800u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(1),
                          MakeSubtitleStreams(1))}, // mainPlaylist
      {805u, MakePlaylist(805u, 120min, {196u}, {120min}, "eng", MakeAudioStreams(9),
                          MakeSubtitleStreams(8))},
  };
  clips = {{196u, MakeClip(120min, {800u, 805u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 800, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);
}

// The same content is often presented in playlists both with and without chapter marks.
// The chaptered presentation is the one kept, unless another exposes more streams.
// (Example: Battlestar Galactica Razor)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_RemovesDuplicatesKeepingTheChaptered)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 120min, {1u}, {60min, 60min}, "eng")},
      {801u, MakePlaylist(801u, 120min, {1u}, {120min}, "eng")},
  };
  ClipMap clips{{1u, MakeClip(120min, {800u, 801u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  // Streams take precedence over chapters
  playlists = {
      {800u, MakePlaylist(800u, 120min, {1u}, {60min, 60min}, "eng", MakeAudioStreams(1))},
      {801u, MakePlaylist(801u, 120min, {1u}, {120min}, "eng", MakeAudioStreams(9))},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801u);
}

// Playlists playing the same clips at different in/out times are distinct editions of the movie,
// however different their stream tables
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_KeepsPlaylistsDifferingByDuration)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 120min, {1u}, {120min}, "eng", MakeAudioStreams(9))},
      {801u, MakePlaylist(801u, 140min, {2u}, {140min}, "eng", MakeAudioStreams(1))},
  };
  ClipMap clips{
      {1u, MakeClip(120min, {800u})},
      {2u, MakeClip(140min, {801u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const auto returned{GetPlaylists(items)};
  const std::set<unsigned int> expected{800u, 801u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// A disc can hold several copies of the clips joining the movie's longer ones and give each copy of
// the movie its own copies of them, so that the copies reference different clips while presenting
// identical content.
// (Example: Avatar (2009), playlists 800 and 801)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_RemovesDuplicatesBuiltFromCopiedClips)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 120min, {1u, 2u, 3u}, {90min, 14s, (29min + 46s)}, "eng")},
      {801u, MakePlaylist(801u, 120min, {1u, 4u, 5u}, {90min, 14s, (29min + 46s)}, "eng")},
  };
  ClipMap clips{
      {1u, MakeClip(90min, {800u, 801u})}, // Shared
      {2u, MakeClip(14s, {800u})},           {3u, MakeClip((29min + 46s), {800u})},
      {4u, MakeClip(14s, {801u})}, // A copy of clip 2
      {5u, MakeClip((29min + 46s), {801u})}, // A copy of clip 3
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  EXPECT_EQ(items.Size(), 2);
  const auto& returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Playlists that share no clip are not copies of one another, however alike their durations and
// chapters - two unrelated titles can run for the same length
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_KeepsPlaylistsSharingNoClip)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u, 2u}, {60min, 60min}, "eng")},
      {2u, MakePlaylist(2u, 120min, {3u, 4u}, {60min, 60min}, "eng")},
  };
  ClipMap clips{
      {1u, MakeClip(60min, {1u})},
      {2u, MakeClip(60min, {1u})},
      {3u, MakeClip(60min, {2u})},
      {4u, MakeClip(60min, {2u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const auto returned{GetPlaylists(items)};
  const std::set<unsigned int> expected{1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Discs hiding the movie among copies of itself can hold hundreds of them
// (Example: John Wick: Chapter 3 - Parabellum (2019), with 385)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_RemovesHundredsOfDuplicates)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  // Each copy of the movie shares its long clip and uses its own copies of the two short ones
  PlaylistMap playlists;
  ClipMap clips{{1u, MakeClip(90min, {})}};
  for (unsigned int i = 0; i < 200; ++i)
  {
    const unsigned int playlist{800u + i};
    const unsigned int first{2u + i * 2};
    playlists.emplace(playlist, MakePlaylist(playlist, 120min, {1u, first, first + 1u},
                                             {90min, 14s, (29min + 46s)}, "eng"));
    clips[1u].playlists.emplace_back(playlist);
    clips.emplace(first, MakeClip(14s, {playlist}));
    clips.emplace(first + 1u, MakeClip((29min + 46s), {playlist}));
  }
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  EXPECT_EQ(items.Size(), 200);
}

// Playlists of the same overall length cut into clips of the same durations are only the same
// presentation when they are cut into the same number of chapters
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_KeepsCopiedClipsHoldingDifferentChapters)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 120min, {1u, 2u}, {90min, 30min}, "eng")},
      {801u, MakePlaylist(801u, 120min, {3u, 4u}, {40min, 40min, 40min}, "eng")},
  };
  ClipMap clips{
      {1u, MakeClip(90min, {800u})},
      {2u, MakeClip(30min, {800u})},
      {3u, MakeClip(90min, {801u})},
      {4u, MakeClip(30min, {801u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  EXPECT_EQ(items.Size(), 2);
  returned = GetPlaylists(items);
  expected = {800u, 801u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Playlists playing clips of different durations are distinct editions of the movie
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_KeepsPlaylistsWhoseClipsAreCutDifferently)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 120min, {1u, 2u}, {30min, 90min}, "eng")},
      {801u, MakePlaylist(801u, 120min, {3u, 4u}, {40min, 80min}, "eng")},
  };
  ClipMap clips{
      {1u, MakeClip(30min, {800u})},
      {2u, MakeClip(90min, {800u})},
      {3u, MakeClip(40min, {801u})},
      {4u, MakeClip(80min, {801u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  EXPECT_EQ(items.Size(), 2);
  returned = GetPlaylists(items);
  expected = {800u, 801u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// The copies a disc holds are all titles, so all are listed when every title is asked for
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_KeepsDuplicatesWhenAllTitlesAreAskedFor)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {800u, MakePlaylist(800u, 120min, {1u}, {120min}, "eng", MakeAudioStreams(9))},
      {801u, MakePlaylist(801u, 120min, {1u}, {120min}, "eng", MakeAudioStreams(1))},
  };
  ClipMap clips{{1u, MakeClip(120min, {800u, 801u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
}

//
// ---- GetMoviePlaylists - editions with differing languages -------------------
//

// The editions of a movie don't necessarily offer the same languages - more dubs may have been
// made of one release than of another. An edition is not discarded for offering fewer.
// (Example: The Wicker Man, where the 1:41 cut has English audio only and the 1:28 cut is
// English and French)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_EditionsWithDifferingLanguages)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {0u, MakePlaylist(0u, 101min, {4u}, {50min, 51min}, "eng", {}, {}, 1080)},
      {10u, MakePlaylist(10u, 88min, {0u}, {44min, 44min}, "eng,fra", {}, {}, 1080)},
  };
  ClipMap clips{
      {0u, MakeClip(88min, {10u})},
      {4u, MakeClip(101min, {0u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const auto returned{GetPlaylists(items)};
  const std::set<unsigned int> expected{0u, 10u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  // The longest of the editions is the single best
  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 0u);
}

//
// ---- GetMoviePlaylists - the single best of playlists of the same length -----
//

// Playlists of (near) identical length are the same movie presented differently, so the single
// best is the fullest of them rather than the longest by a second or two.
// (Example: Snow White (2025), whose sing along runs 2 seconds longer than the movie but drops
// an audio track and half the subtitles)
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_SingleBestOfEqualLengthPlaylists)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      // The movie
      {800u, MakePlaylist(800u, 108min + 49s, {294u, 679u, 680u}, {54min, 54min + 49s},
                          "eng,eng,fra", MakeAudioStreams(3), MakeSubtitleStreams(17), 1080)},
      // The sing along - the movie wrapped in a bumper, with fewer streams
      {1666u,
       MakePlaylist(1666u, 108min + 51s, {710u, 294u, 679u, 680u, 710u}, {54min, 54min + 51s},
                    "eng,fra", MakeAudioStreams(2), MakeSubtitleStreams(9), 1080)},
  };
  ClipMap clips{
      {294u, MakeClip(50min, {800u, 1666u})},
      {679u, MakeClip(30min, {800u, 1666u})},
      {680u, MakeClip(28min + 49s, {800u, 1666u})},
      {710u, MakeClip(1s, {1666u})},
  };

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);

  // The best also leads the versions, as the first of them becomes the default
  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800u);
  EXPECT_EQ(GetPlaylistFromPath(items[1]->GetPath()), 1666u);

  // An edition that is genuinely longer is still the single best, however few streams it offers
  playlists[1666u] = MakePlaylist(1666u, 120min, {710u, 294u, 679u, 680u, 710u}, {60min, 60min},
                                  "eng", MakeAudioStreams(1), MakeSubtitleStreams(1), 1080);

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1666u);
}

//
// ---- GetMoviePlaylists - resolution ------------------------------------------
//

// A standard definition extra is not a version of the movie
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_Resolution)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  // The extra is longer than the movie (as a 'play all' playlist of the extras could be)
  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u}, {60min, 60min}, "eng", {}, {}, 1080)}, // The movie
      {2u, MakePlaylist(2u, 150min, {2u}, {75min, 75min}, "eng", {}, {}, 480)}, // Extra
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(150min, {2u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);

  // SINGLE returns the longest of the remaining candidates, not the longest on the disc
  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 1u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  const auto& returned{GetPlaylists(items)};
  const std::set<unsigned int> expected{1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Every version of the movie is presented at the same resolution, so all are kept - and an
// interlaced version is not discarded in favour of a lower resolution progressive one
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_Resolution_MultipleVersions)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {1u,
       MakePlaylist(1u, 120min, {1u}, {60min, 60min}, "eng", {}, {}, 1080)}, // Theatrical (1080i)
      {2u, MakePlaylist(2u, 140min, {2u}, {70min, 70min}, "eng", {}, {}, 1080)}, // Extended
      {3u, MakePlaylist(3u, 100min, {3u}, {50min, 50min}, "eng", {}, {}, 720)}, // Extra (720p)
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(140min, {2u})},
      {3u, MakeClip(100min, {3u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 2u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 2u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {1u, 2u, 3u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

// Playlists without video stream information are neither discarded nor used for comparison,
// and the known main playlist is never discarded for its resolution
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_Resolution_UnknownAndMainPlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  PlaylistMap playlists{
      {1u, MakePlaylist(1u, 120min, {1u}, {60min, 60min}, "eng", {}, {}, 1080)},
      {2u, MakePlaylist(2u, 110min, {2u}, {55min, 55min}, "eng")}, // No video information
      {3u, MakePlaylist(3u, 100min, {3u}, {50min, 50min}, "eng", {}, {}, 480)}, // mainPlaylist
  };
  ClipMap clips{
      {1u, MakeClip(120min, {1u})},
      {2u, MakeClip(110min, {2u})},
      {3u, MakeClip(100min, {3u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, 3, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{1u, 2u, 3u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, 3, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 3u);

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, 3, GetTitle::ALL, clips, playlists));
  ASSERT_EQ(items.Size(), 3);
  returned = GetPlaylists(items);
  expected = {1u, 2u, 3u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}

//
// ---- GetMoviePlaylists - seamlessly branched discs ---------------------------
//

// A seamlessly branched disc holding a theatrical (800) and a special edition (801) built from
// shared clips. Each of those clips is also playable on its own (for scene selection), and one of
// them (1629) is longer than MIN_MOVIE_DURATION. Playlists 805 and 806 present the same content
// as 800 and 801 but expose the original language only.
TEST_F(TestDiscDirectoryHelper, GetMoviePlaylists_SeamlesslyBranchedDisc)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;

  const std::string allLanguages{"eng,eng,eng,fra,spa"};
  PlaylistMap playlists{
      {800u,
       MakePlaylist(800u, 137min, {705u, 749u, 757u}, {68min, 69min}, allLanguages, {}, {}, 1080)},
      {801u, MakePlaylist(801u, 154min, {705u, 749u, 755u, 757u}, {77min, 77min}, allLanguages, {},
                          {}, 1080)},
      {805u, MakePlaylist(805u, 137min, {705u, 749u, 757u}, {68min, 69min}, "eng", {}, {}, 1080)},
      {806u,
       MakePlaylist(806u, 154min, {705u, 749u, 755u, 757u}, {77min, 77min}, "eng", {}, {}, 1080)},
      // The individually playable clips of the two editions
      {1491u, MakePlaylist(1491u, 9min, {705u}, {9min}, allLanguages, {}, {}, 1080)},
      {1629u, MakePlaylist(1629u, 33min, {749u}, {33min}, allLanguages, {}, {}, 1080)},
      {1635u, MakePlaylist(1635u, 17min, {755u}, {17min}, allLanguages, {}, {}, 1080)},
      {1637u, MakePlaylist(1637u, 95min, {757u}, {95min}, allLanguages, {}, {}, 1080)},
  };
  ClipMap clips{
      {705u, MakeClip(9min, {800u, 801u, 805u, 806u, 1491u})},
      {749u, MakeClip(33min, {800u, 801u, 805u, 806u, 1629u})},
      {755u, MakeClip(17min, {801u, 806u, 1635u})},
      {757u, MakeClip(95min, {800u, 801u, 805u, 806u, 1637u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::MAIN, clips, playlists));
  ASSERT_EQ(items.Size(), 2);
  auto returned{GetPlaylists(items)};
  std::set<unsigned int> expected{800u, 801u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));

  EXPECT_TRUE(
      helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::SINGLE, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 801u); // Longest edition

  EXPECT_TRUE(helper.GetMoviePlaylists(url, items, allTitles, -1, GetTitle::ALL, clips, playlists));
  EXPECT_EQ(items.Size(), 8);
  returned = GetPlaylists(items);
  expected = {800u, 801u, 805u, 806u, 1491u, 1629u, 1635u, 1637u};
  EXPECT_TRUE(std::ranges::includes(returned, expected));
}
