/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/BlurayDirectory.h"
#include "filesystem/DiscDirectoryHelper.h"

#include <chrono>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;
using namespace std::chrono_literals;

class TestBlurayDirectory : public ::testing::Test
{
protected:
  static bool FilterPlaylists(std::vector<PlaylistInformation>& playlists)
  {
    return CBlurayDirectory::FilterPlaylists(playlists);
  }
};

namespace
{
PlaylistInformation MakePlaylist(unsigned int playlist,
                                 std::chrono::milliseconds duration,
                                 std::vector<unsigned int> clips,
                                 std::vector<std::chrono::milliseconds> chapters = {1min})
{
  PlaylistInformation info;
  info.playlist = playlist;
  info.duration = duration;
  info.clips = std::move(clips);
  info.chapters = std::move(chapters);
  return info;
}

std::vector<unsigned int> PlaylistNumbers(const std::vector<PlaylistInformation>& playlists)
{
  std::vector<unsigned int> numbers;
  numbers.reserve(playlists.size());
  for (const PlaylistInformation& playlist : playlists)
    numbers.emplace_back(playlist.playlist);
  return numbers;
}
} // namespace

//
// ---- FilterPlaylists -------------------------------------------------------
//

// No playlists at all
TEST_F(TestBlurayDirectory, FilterPlaylists_NoPlaylists)
{
  std::vector<PlaylistInformation> playlists;

  EXPECT_FALSE(FilterPlaylists(playlists));
  EXPECT_TRUE(playlists.empty());
}

// No valid playlists
TEST_F(TestBlurayDirectory, FilterPlaylists_EveryPlaylistDiscarded)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 0ms, {}), // No clips
      MakePlaylist(801u, 500ms, {1u}), // Under a second
      MakePlaylist(802u, 2h, {1u, 1u}), // Repeated clip
  };

  EXPECT_FALSE(FilterPlaylists(playlists));
  EXPECT_TRUE(playlists.empty());
}

// A single playlist means the duplicate comparison has no pair to consider
TEST_F(TestBlurayDirectory, FilterPlaylists_SinglePlaylistIsKept)
{
  std::vector<PlaylistInformation> playlists{MakePlaylist(800u, 2h, {1u})};

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{800u});
}

TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesPlaylistsWithoutClips)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {}),
      MakePlaylist(801u, 2h, {1u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{801u});
}

TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesPlaylistsShorterThanASecond)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 999ms, {1u}),
      MakePlaylist(801u, 1s, {2u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{801u});
}

TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesPlaylistsWithARepeatedClip)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u, 2u, 1u}),
      MakePlaylist(801u, 2h, {3u, 4u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{801u});
}

// Duplicates are matched on clips, chapters, audio and subtitle streams.
// The earlier playlist is the one kept.
TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesDuplicatesKeepingTheFirst)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u, 2u}),
      MakePlaylist(801u, 2h, {1u, 2u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{800u});
}

// Differing in any compared field is enough to be a distinct title
TEST_F(TestBlurayDirectory, FilterPlaylists_KeepsPlaylistsDifferingByClips)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u, 2u}),
      MakePlaylist(801u, 2h, {3u, 4u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), (std::vector<unsigned int>{800u, 801u}));
}

TEST_F(TestBlurayDirectory, FilterPlaylists_KeepsPlaylistsDifferingByChapters)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u}, {1min}),
      MakePlaylist(801u, 2h, {1u}, {1min, 2min}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), (std::vector<unsigned int>{800u, 801u}));
}

// Three copies must all collapse to the first, not just the adjacent pair
TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesAllCopiesOfADuplicate)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u}),
      MakePlaylist(801u, 2h, {1u}),
      MakePlaylist(802u, 2h, {1u}),
      MakePlaylist(803u, 2h, {2u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), (std::vector<unsigned int>{800u, 803u}));
}
