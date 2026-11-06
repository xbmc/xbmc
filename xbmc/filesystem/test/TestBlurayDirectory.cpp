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
#include "filesystem/BlurayDirectory.h"
#include "filesystem/BlurayDiscCache.h"
#include "filesystem/Directory.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "language/LanguageTag.h"
#include "test/TestUtils.h"
#include "utils/ItemProperties.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;
using namespace std::chrono_literals;
using KODI::LANGUAGE::CLanguageTag;

class TestBlurayDirectory : public ::testing::Test
{
protected:
  static bool FilterPlaylists(std::vector<PlaylistInformation>& playlists)
  {
    return CBlurayDirectory::FilterPlaylists(playlists);
  }

  static void ProcessPlaylist(PlaylistMap& playlists,
                              PlaylistInformation& titleInfo,
                              ClipMap& clips)
  {
    CBlurayDirectory::ProcessPlaylist(playlists, titleInfo, clips);
  }

  static bool GetPlaylists(const CURL& url, CFileItemList& items)
  {
    std::map<unsigned int, ClipInformation> clipCache;
    return CBlurayDirectory::GetPlaylists(url, "", 0, ALL_PLAYLISTS, items, clipCache);
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

// A playlist playing nothing but the same clip over and over is a loop (eg. a menu background)
TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesLoopingPlaylists)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u, 1u, 1u}),
      MakePlaylist(801u, 2h, {3u, 4u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{801u});

  // A reel assembled from a few clips played over and over is a loop too
  // (Example: Fast X, where playlist 149 plays 3 clips 251 times)
  std::vector<unsigned int> reel;
  for (int i = 0; i < 30; i++)
    reel.insert(reel.end(), {1u, 2u, 3u});

  playlists = {
      MakePlaylist(800u, 2h, reel),
      MakePlaylist(801u, 2h, {3u, 4u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{801u});
}

// A movie can revisit a clip, so a playlist that isn't looping over its clips is kept.
// (Example: Blackhat, where the feature is clips 0,4,4 and the disc holds nothing else)
TEST_F(TestBlurayDirectory, FilterPlaylists_KeepsPlaylistsRevisitingAClip)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {0u, 4u, 4u}),
      MakePlaylist(801u, 2h, {3u, 4u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), (std::vector<unsigned int>{800u, 801u}));
}

// Duplicates are matched on clips, chapters, audio and subtitle streams.
// The lowest numbered playlist is the one kept, whatever order the disc lists them in.
TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesDuplicatesKeepingTheLowestNumbered)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u, 2u}),
      MakePlaylist(801u, 2h, {1u, 2u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{800u});

  playlists = {
      MakePlaylist(801u, 2h, {1u, 2u}),
      MakePlaylist(799u, 2h, {1u, 2u}),
      MakePlaylist(800u, 2h, {1u, 2u}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{799u});
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

// Playlists can play the same clips from the same chapter starts but to different out times,
// making them distinct cuts rather than copies
TEST_F(TestBlurayDirectory, FilterPlaylists_KeepsPlaylistsDifferingByDuration)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u}, {1min, 2min}),
      MakePlaylist(801u, 2h + 3min, {1u}, {1min, 2min}),
  };

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), (std::vector<unsigned int>{800u, 801u}));
}

// A picture-in-picture copy is the one removed, even when numbered first
TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesAPictureInPictureCopyKeepingThePlainOne)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u}),
      MakePlaylist(801u, 2h, {1u}),
  };
  playlists[0].hasSecondaryVideo = true;

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{801u});
}

// A copy with a Dolby Vision enhancement layer is the one kept, even when numbered second
TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesDuplicatesKeepingTheDolbyVisionOne)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u}),
      MakePlaylist(801u, 2h, {1u}),
  };
  playlists[1].hasDolbyVision = true;

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{801u});
}

// Being picture-in-picture outweighs carrying Dolby Vision, as such a copy never leads
TEST_F(TestBlurayDirectory, FilterPlaylists_RemovesAPictureInPictureCopyEvenWithDolbyVision)
{
  std::vector<PlaylistInformation> playlists{
      MakePlaylist(800u, 2h, {1u}),
      MakePlaylist(801u, 2h, {1u}),
  };
  playlists[1].hasSecondaryVideo = true;
  playlists[1].hasDolbyVision = true;

  EXPECT_TRUE(FilterPlaylists(playlists));
  EXPECT_EQ(PlaylistNumbers(playlists), std::vector<unsigned int>{800u});
}

// Three copies must all collapse to one, not just the adjacent pair
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

// What a playlist offers has to survive being recorded, or the movie and episode searches decide
// on a playlist described differently to the one the disc holds
TEST_F(TestBlurayDirectory, ProcessPlaylist_CarriesThePlaylistThrough)
{
  PlaylistInformation titleInfo{MakePlaylist(801u, 2h, {1u, 2u}, {1min, 2min})};
  titleInfo.clipDuration = {{1u, 1h}, {2u, 1h}};
  titleInfo.hasSecondaryVideo = true;

  AudioStreamInfo audio;
  audio.language = CLanguageTag::Parse("eng");
  titleInfo.audioStreams.emplace_back(audio);

  SubtitleStreamInfo subtitle;
  subtitle.language = CLanguageTag::Parse("fra");
  titleInfo.pgStreams.emplace_back(subtitle);

  PlaylistMap playlists;
  ClipMap clips;
  ProcessPlaylist(playlists, titleInfo, clips);

  ASSERT_TRUE(playlists.contains(801u));
  const PlaylistInformation& recorded{playlists[801u]};
  EXPECT_EQ(recorded.playlist, 801u);
  EXPECT_EQ(recorded.duration, 2h);
  EXPECT_EQ(recorded.clips, (std::vector<unsigned int>{1u, 2u}));
  EXPECT_EQ(recorded.chapters.size(), titleInfo.chapters.size());
  EXPECT_EQ(recorded.audioStreams.size(), 1U);
  EXPECT_EQ(recorded.pgStreams.size(), 1U);
  EXPECT_EQ(recorded.languages, "en");
  EXPECT_TRUE(recorded.hasSecondaryVideo);

  // The clips are recorded once, in the clip map, along with which playlists reference them
  EXPECT_TRUE(recorded.clipDuration.empty());
  ASSERT_TRUE(clips.contains(1u));
  EXPECT_EQ(clips[1u].duration, 1h);
  EXPECT_EQ(clips[1u].playlists, (std::vector<unsigned int>{801u}));
}

// A disc folder can hold other files named after a playlist, eg. a checksum of each file
TEST_F(TestBlurayDirectory, GetPlaylists_IgnoresOtherFilesNamedAfterAPlaylist)
{
  const auto previousCache{CServiceBroker::GetBlurayDiscCache()};
  CServiceBroker::RegisterBlurayDiscCache(std::make_shared<CBlurayDiscCache>());

  const std::string disc{CSpecialProtocol::TranslatePath("special://temp/TestBlurayDirectory/")};
  const std::string playlistFolder{URIUtils::AddFileToFolder(disc, "BDMV", "PLAYLIST")};
  ASSERT_TRUE(CDirectory::Create(disc));
  ASSERT_TRUE(CDirectory::Create(URIUtils::AddFileToFolder(disc, "BDMV")));
  ASSERT_TRUE(CDirectory::Create(playlistFolder));
  ASSERT_TRUE(
      CFile::Copy(XBMC_REF_FILE_PATH("xbmc/utils/test/data/bluray/BDMV/PLAYLIST/00888.mpls"),
                  URIUtils::AddFileToFolder(playlistFolder, "00888.mpls")));
  {
    CFile hash;
    ASSERT_TRUE(
        hash.OpenForWrite(URIUtils::AddFileToFolder(playlistFolder, "00888.mpls.hash"), true));
    const std::string content{"d41d8cd98f00b204e9800998ecf8427e"};
    ASSERT_EQ(hash.Write(content.data(), content.size()), static_cast<ssize_t>(content.size()));
  }

  CURL url;
  url.SetProtocol("bluray");
  url.SetHostName(disc);
  CFileItemList items;
  const bool found{GetPlaylists(url, items)};

  CDirectory::RemoveRecursive(disc);
  if (previousCache)
    CServiceBroker::RegisterBlurayDiscCache(previousCache);
  else
    CServiceBroker::UnregisterBlurayDiscCache();

  EXPECT_TRUE(found);
  ASSERT_EQ(items.Size(), 1);
  EXPECT_EQ(items[0]->GetProperty(KODI::ITEM::PROPERTY::BLURAY_PLAYLIST).asInteger(), 888);
}
