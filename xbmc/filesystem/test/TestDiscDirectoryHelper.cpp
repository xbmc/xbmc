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
#include "video/Episode.h"

#include <chrono>
#include <numeric>
#include <ranges>
#include <set>
#include <string>

#include <gtest/gtest.h>

using namespace XFILE;
using namespace std::chrono_literals;
using Episodes = std::vector<KODI::VIDEO::EPISODE>;

namespace
{
PlaylistInformation MakePlaylist(std::chrono::milliseconds duration,
                                 std::vector<unsigned int> clips,
                                 std::vector<std::chrono::milliseconds> chapterDurations,
                                 std::string languages = "")
{
  PlaylistInformation info;
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

bool Validate(ClipMap& clips, PlaylistMap& playlists)
{
  // Check relationship between clips and playlists
  for (const auto& [playlistNumber, playlistInformation] : playlists)
  {
    std::chrono::milliseconds duration{0ms};

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

TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_EmptyPlaylistMap)
{
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, {}, {}));
  EXPECT_EQ(items.Size(), 0);
}

TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_AllPlaylistsBelowMinEpisodeDuration)
{
  // A single playlist shorter than MIN_EPISODE_DURATION must not be chosen.
  CDiscDirectoryHelper helper;
  CURL url;
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(5min, {1u}, {5min})}};
  ClipMap clips{{1u, MakeClip(5min, {800u})}};

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}

//
// ---- GetEpisodePlaylists – single episode disc ------------------------------
//

// Single episode on disc with no specials
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_OnePlaylist)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(60min, {1u}, {60min})}};
  ClipMap clips{{1u, MakeClip(60min, {800u})}};
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 1, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)
}

// One playlist of > MIN_EPISODE_DURATION and multiple shorter playlists
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_MultiplePlaylists)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(60min, {1u}, {60min})},
                        {1u, MakePlaylist(5min, {2u}, {5min})},
                        {10u, MakePlaylist(5min, {3u}, {5min})},
                        {100u, MakePlaylist(5min, {4u}, {5min})}};
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)
}

// Two playlists of > MIN_EPISODE_DURATION, one with a common playlist number, and multiple shorter playlists
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_MultiplePlaylists2)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(60min, {1u}, {60min})},
                        {1u, MakePlaylist(5min, {2u}, {5min})},
                        {10u, MakePlaylist(5min, {3u}, {5min})},
                        {100u, MakePlaylist(40min, {4u}, {40min})}};
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)
}

// Two playlists of > MIN_EPISODE_DURATION, one with a common playlist number, and multiple shorter playlists
// One of the other playlists is a special feature
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_SingleEpisode_MultiplePlaylists_WithSpecial)
{
  CDiscDirectoryHelper helper;
  CURL url("bluray://test/");
  CFileItemList items;
  CFileItemList allTitles;
  Episodes episodes{MakeEpisode(0, 1, 1800), // Special
                    MakeEpisode(1, 1, 3600)};

  PlaylistMap playlists{{800u, MakePlaylist(60min, {1u}, {60min})},
                        {1u, MakePlaylist(5min, {2u}, {5min})},
                        {10u, MakePlaylist(5min, {3u}, {5min})},
                        {100u, MakePlaylist(30min, {4u}, {30min})}};
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
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(1u)); // Any of the 3 remaining playlists could be the special
  EXPECT_TRUE(returned.contains(10u));
  EXPECT_TRUE(returned.contains(100u));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 2, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(GetPlaylistFromPath(items[0]->GetPath()), 800); // All Episodes (single episode)
}

//
// ---- GetEpisodePlaylists – play-all playlist method -------------------------
//

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})},
      {804u, MakePlaylist(38min, {3u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(802u));
  EXPECT_TRUE(returned.contains(804u));
}

// Disc has a play - all playlist(clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// One other the other playlists is a special
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_WithSpecial)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})},
      {804u, MakePlaylist(38min, {3u}, {38min})},
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
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(1u)); // Any of the 2 remaining playlists could be the special
  EXPECT_TRUE(returned.contains(10u));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  returned.clear();
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(802u));
  EXPECT_TRUE(returned.contains(804u));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an intro clip
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(128min, {6u, 1u, 2u, 3u}, {3min, 45min, 42min, 38min})},
      {800u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})},
      {804u, MakePlaylist(38min, {3u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(802u));
  EXPECT_TRUE(returned.contains(804u));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an ending clip
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips2)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(128min, {1u, 2u, 3u, 6u}, {45min, 42min, 38min, 3min})},
      {800u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})},
      {804u, MakePlaylist(38min, {3u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(802u));
  EXPECT_TRUE(returned.contains(804u));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an intro clip and clip 7 is an ending clip
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraClips3)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(131min, {6u, 1u, 2u, 3u, 7u}, {3min, 45min, 42min, 38min, 3min})},
      {800u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})},
      {804u, MakePlaylist(38min, {3u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(802u));
  EXPECT_TRUE(returned.contains(804u));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Individual episode playlists are allowed to have short beginning/ending clips (for recap/credits etc.)
// First episode has credits, second has intro and credits, last has intro
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_ExtraIndividualClips)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(48min, {1u, 6u}, {45min, 3min})},
      {802u, MakePlaylist(48min, {7u, 2u, 8u}, {3min, 42min, 3min})},
      {804u, MakePlaylist(41min, {9u, 3u}, {3min, 38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(802u));
  EXPECT_TRUE(returned.contains(804u));
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Note that clips in playlist 100 don't match the individual episodes' clips (800,802,804)
// So failure expected
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_Fail)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})},
      {804u, MakePlaylist(38min, {6u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u})},       {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(38min, {804u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Play-all playlist is allowed to have short clips at the beginning and/or end (eg. intro/ending credits)
// Clip 6 is an intro clip and clip 7 is an ending clip - but both are too long
// So failure expected
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_Fail2)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(155min, {6u, 1u, 2u, 3u, 7u}, {15min, 45min, 42min, 38min, 15min})},
      {800u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})},
      {804u, MakePlaylist(38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {100u, 800u})}, {2u, MakeClip(42min, {100u, 802u})},
      {3u, MakeClip(38min, {100u, 804u})}, {4u, MakeClip(5min, {1u})},
      {5u, MakeClip(5min, {10u})},         {6u, MakeClip(15min, {100u})},
      {7u, MakeClip(15min, {100u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}

// Disc has a play-all playlist (clips shared with individual episode playlists)
// Playlist 100 = play-all; 800 = episode 1; 802 = episode 2; 804 = episode 3
// Playlists not sequential to prevent group matching
// Individual episode playlists are allowed to have short beginning/ending clips (for recap/credits etc.)
// First episode has credits, second has intro and credits, last has intro but the credits
// Note the credits on the middle episode are too long
// So failure is expected
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_PlayAllPlaylist_Fail3)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},
      {10u, MakePlaylist(5min, {5u}, {5min})},
      {100u, MakePlaylist(125min, {1u, 2u, 3u}, {45min, 42min, 38min})},
      {800u, MakePlaylist(48min, {1u, 6u}, {45min, 3min})},
      {802u, MakePlaylist(60min, {7u, 2u, 8u}, {3min, 42min, 15min})},
      {804u, MakePlaylist(41min, {9u, 3u}, {3min, 38min})},
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

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}

//
// ---- GetEpisodePlaylists – multi-episode disc, group method -----------------
//

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(42min, {2u}, {42min})},
      {802u, MakePlaylist(38min, {3u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(801u));
  EXPECT_TRUE(returned.contains(802u));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// Note episode 1 is significantly longer (but this is allowed as the group playlists = number of episodes)
// (Similar to Firefly S1D1 US Bluray where episode 1 (DVD Order) Serenity is significantly longer)
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_LongEpisode)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},       {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(142min, {1u}, {142min})}, {801u, MakePlaylist(45min, {2u}, {45min})},
      {802u, MakePlaylist(38min, {3u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(801u));
  EXPECT_TRUE(returned.contains(802u));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// One of the playlists is a special
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_WithSpecial)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(42min, {2u}, {42min})},
      {802u, MakePlaylist(38min, {3u}, {38min})},
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
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(1u)); // Any of the 2 remaining playlists could be the special
  EXPECT_TRUE(returned.contains(10u));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 4, episodes, clips,
                                          playlists)); // Invalid episode index
  EXPECT_EQ(items.Size(), 0);

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  returned.clear();
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(801u));
  EXPECT_TRUE(returned.contains(802u));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// The group is 800-803. The episodes are mapped to the start of the group
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_LongerGroup)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(42min, {2u}, {42min})},
      {802u, MakePlaylist(38min, {3u}, {38min})}, {803u, MakePlaylist(38min, {7u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(801u));
  EXPECT_TRUE(returned.contains(802u));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// There is an additional 3 playlist group but the playlist 1 is too short so the group is ignored
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_TwoGroupsOneInvalid)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(42min, {2u}, {42min})},
      {802u, MakePlaylist(38min, {3u}, {38min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 3); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(801u));
  EXPECT_TRUE(returned.contains(802u));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 801 = episode 1; 802 = episode 2
// There is an additional group at 851-852 using the same clips
// (Example - The Last of Us S1D1 UK UHD)
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_TwoEpisodes_GroupMethod_TwoGroupsBothValid)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {2u, MakePlaylist(30min, {5u}, {30min})},
      {3u, MakePlaylist(30min, {6u}, {30min})},   {801u, MakePlaylist(45min, {1u}, {45min})},
      {802u, MakePlaylist(42min, {2u}, {42min})}, {851u, MakePlaylist(45min, {1u}, {45min})},
      {852u, MakePlaylist(42min, {2u}, {42min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 2); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(801u));
  EXPECT_TRUE(returned.contains(802u));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// Playlist 801 is too small, so failure expected
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_Fail)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(5min, {2u}, {5min})},
      {802u, MakePlaylist(38min, {3u}, {38min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(5min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episode 3
// The group is 800-803. The episodes should be mapped to the start of the group but
// playlist 801 is long. This is allowed when group playlists = number of episodes but
// as there are 4 playlists in the group they must be within 20% of the desired episode.
// So failure expected
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_ThreeEpisodes_GroupMethod_Fail2)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(142min, {2u}, {142min})},
      {802u, MakePlaylist(38min, {3u}, {38min})}, {803u, MakePlaylist(40min, {7u}, {40min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(142min, {801u})}, {3u, MakeClip(38min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},    {7u, MakeClip(40min, {803u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}

// There is no play-all playlist, nor any consecutive groups of playlists (of the correct number)
// There are only n playlists of the appropriate l length, so the assumption is these map to episodes
// in ascending numerical order.
// (Example Twisted Metal S1D1 UK UHD)
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_FiveEpisodes_GroupMethod_ExactNumberOfPlaylists)
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
      {0u, MakePlaylist(40min, {1u, 2u}, {40min})},  {1u, MakePlaylist(42min, {3u, 2u}, {42min})},
      {2u, MakePlaylist(45min, {7u, 2u}, {45min})},  {7u, MakePlaylist(47min, {10u, 2u}, {47min})},
      {8u, MakePlaylist(48min, {11u, 2u}, {48min})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(items.Size(), 5); // All episodes
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_TRUE(returned.contains(0u));
  EXPECT_TRUE(returned.contains(1u));
  EXPECT_TRUE(returned.contains(2u));
  EXPECT_TRUE(returned.contains(7u));
  EXPECT_TRUE(returned.contains(8u));
}

// There is no play-all playlist, nor any consecutive groups of playlists (of the correct number)
// There are only n playlists of the appropriate l length, so the assumption is these map to episodes
// in ascending numerical order.
// Playlist 1 is long, so the playlist collection is rejected
// So failure expected
TEST(TestDiscDirectoryHelper,
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
      {0u, MakePlaylist(40min, {1u, 2u}, {40min})},  {1u, MakePlaylist(142min, {3u, 2u}, {142min})},
      {2u, MakePlaylist(45min, {7u, 2u}, {45min})},  {7u, MakePlaylist(47min, {10u, 2u}, {47min})},
      {8u, MakePlaylist(48min, {11u, 2u}, {48min})},
  };
  ClipMap clips{
      {1u, MakeClip(40min, {0u})},  {2u, MakeClip(0min, {0u, 1u, 2u, 7u, 8u})},
      {3u, MakeClip(142min, {1u})}, {7u, MakeClip(45min, {2u})},
      {10u, MakeClip(47min, {7u})}, {11u, MakeClip(48min, {8u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episodes 3 and 4
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_FourEpisodesOneDouble_GroupMethod)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(42min, {2u}, {42min})},
      {802u, MakePlaylist(92min, {3u}, {92min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(92min, {802u})},
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

  EXPECT_TRUE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  ASSERT_EQ(
      items.Size(),
      3); // All episode playlists (802 is only returned once as it's the same playlist for episodes 3 and 4)
  std::set<unsigned int> returned;
  for (int i = 0; i < items.Size(); ++i)
    returned.insert(GetPlaylistFromPath(items[i]->GetPath()));
  EXPECT_EQ(returned.size(), 3);
  EXPECT_TRUE(returned.contains(800u));
  EXPECT_TRUE(returned.contains(801u));
  EXPECT_TRUE(returned.contains(802u));
}

// Consecutive playlists → group method assigns the nth playlist to episode n
// Playlist 800 = episode 1; 801 = episode 2; 802 = episodes 3 and 4
// Playlist 802 is only 1.5x the average episode length so fail expected
TEST(TestDiscDirectoryHelper, GetEpisodePlaylists_FourEpisodesOneDouble_GroupMethod_Fail)
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
      {1u, MakePlaylist(5min, {4u}, {5min})},     {10u, MakePlaylist(5min, {5u}, {5min})},
      {800u, MakePlaylist(45min, {1u}, {45min})}, {801u, MakePlaylist(42min, {2u}, {42min})},
      {802u, MakePlaylist(57min, {3u}, {57min})},
  };
  ClipMap clips{
      {1u, MakeClip(45min, {800u})}, {2u, MakeClip(42min, {801u})}, {3u, MakeClip(57min, {802u})},
      {4u, MakeClip(5min, {1u})},    {5u, MakeClip(5min, {10u})},
  };
  ASSERT_TRUE(Validate(clips, playlists));

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, 0, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);

  EXPECT_FALSE(helper.GetEpisodePlaylists(url, items, allTitles, -1, episodes, clips, playlists));
  EXPECT_EQ(items.Size(), 0);
}
