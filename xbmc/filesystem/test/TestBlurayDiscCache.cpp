/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "filesystem/BlurayDirectory.h"
#include "filesystem/BlurayDiscCache.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/bluray/M2TSParser.h"
#include "filesystem/bluray/PlaylistStructure.h"
#include "utils/URIUtils.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
const std::string DISC{"D:\\Movies\\Movie\\"};

void WriteIndex(const std::string& disc, const std::string& content)
{
  CFile file;
  ASSERT_TRUE(file.OpenForWrite(URIUtils::AddFileToFolder(disc, "BDMV", "index.bdmv"), true));
  ASSERT_EQ(file.Write(content.data(), content.size()), static_cast<ssize_t>(content.size()));
}

//! A playlist playing each clip for as long as given, in turn, and a 3D one's stereo clip
BlurayPlaylistInformation Playlist(
    const std::vector<std::pair<unsigned int, std::chrono::milliseconds>>& clips,
    std::optional<unsigned int> stereoClip = {})
{
  BlurayPlaylistInformation playlist;
  std::chrono::milliseconds time{0};
  for (const auto& [clip, duration] : clips)
  {
    PlayItemInformation playItem;
    playItem.inTime = time;
    playItem.outTime = time + duration;
    time += duration;
    playItem.angleClips.emplace_back(clip, "M2TS");
    playlist.playItems.push_back(std::move(playItem));
  }
  if (stereoClip)
  {
    SubPlayItemInformation subPlayItem;
    subPlayItem.clips.emplace_back(*stereoClip, "M2TS");
    playlist.extensionSubPlayItems.push_back(std::move(subPlayItem));
  }
  return playlist;
}
} // namespace

using namespace std::chrono_literals;

// A playlist's streams are named by the clip files they are read from: its longest clip, and a 3D
// playlist's stereo clip too
TEST(TestBlurayDiscCache, GetStreamClipFiles_NamesTheLongestClipAndAStereoClip)
{
  const std::string part{GetStreamClipFiles(Playlist({{848, 10min}}))};
  EXPECT_EQ(part, GetStreamClipFiles(Playlist({{847, 1min}, {848, 30min}})));
  EXPECT_NE(part, GetStreamClipFiles(Playlist({{848, 10min}}, 849)));
  EXPECT_NE(part, GetStreamClipFiles(Playlist({{855, 10min}})));
  EXPECT_EQ("", GetStreamClipFiles(BlurayPlaylistInformation{}));
}

// The streams read for one playlist serve each playlist playing the same clip files, as the
// playlists of a disc's extras often play parts of one clip, and go with the disc
TEST(TestBlurayDiscCache, ClipStreamInfo_ServesEachPlaylistPlayingTheSameFiles)
{
  CBlurayDiscCache cache;
  StreamMap streams;
  streams.emplace(0x1011, std::make_shared<TSVideoStreamInfo>());
  cache.SetClipStreamInfo(DISC, GetStreamClipFiles(Playlist({{848, 10min}})), streams);

  StreamMap found;
  EXPECT_TRUE(cache.GetClipStreamInfo(
      DISC, GetStreamClipFiles(Playlist({{847, 1min}, {848, 30min}})), found));
  EXPECT_EQ(1U, found.size());
  EXPECT_FALSE(
      cache.GetClipStreamInfo(DISC, GetStreamClipFiles(Playlist({{848, 10min}}, 849)), found));

  cache.ClearDisc(DISC);
  EXPECT_FALSE(cache.GetClipStreamInfo(DISC, GetStreamClipFiles(Playlist({{848, 10min}})), found));
}

// A disc in a drive is probed for its name before it is ever listed, so the probe records which
// disc its name came from too. Eg. a removable drive holding a disc folder, replaced by another
// under the same drive letter.
TEST(TestBlurayDiscCache, ProbeDisc_DropsWhatAReplacedDiscCached)
{
  const auto previousCache{CServiceBroker::GetBlurayDiscCache()};
  const auto cache{std::make_shared<CBlurayDiscCache>()};
  CServiceBroker::RegisterBlurayDiscCache(cache);

  const std::string disc{CSpecialProtocol::TranslatePath("special://temp/TestBlurayDiscCache/")};
  ASSERT_TRUE(CDirectory::Create(disc));
  ASSERT_TRUE(CDirectory::Create(URIUtils::AddFileToFolder(disc, "BDMV")));
  WriteIndex(disc, "first disc");

  // Not a real disc, so the probe finds no name - but the disc it probed is recorded
  CBlurayDirectory::ProbeDisc(disc);

  // As a probe of a real disc would have cached it
  cache->SetDiscTitle(disc, "First Disc");
  EXPECT_EQ(CBlurayDirectory::ProbeDisc(disc).name, "First Disc");

  WriteIndex(disc, "a different disc");
  EXPECT_TRUE(CBlurayDirectory::ProbeDisc(disc).name.empty());

  CDirectory::RemoveRecursive(disc);
  if (previousCache)
    CServiceBroker::RegisterBlurayDiscCache(previousCache);
  else
    CServiceBroker::UnregisterBlurayDiscCache();
}

TEST(TestBlurayDiscCache, CheckDisc_KeepsTheSameDisc)
{
  CBlurayDiscCache cache;
  cache.CheckDisc(DISC, "1:100");
  cache.SetDiscTitle(DISC, "Movie");

  cache.CheckDisc(DISC, "1:100");

  std::string title;
  EXPECT_TRUE(cache.GetDiscTitle(DISC, title));
  EXPECT_EQ(title, "Movie");
}

TEST(TestBlurayDiscCache, CheckDisc_DropsAReplacedDisc)
{
  CBlurayDiscCache cache;
  cache.CheckDisc(DISC, "1:100");
  cache.SetDiscTitle(DISC, "Movie");

  cache.CheckDisc(DISC, "2:100");

  std::string title;
  EXPECT_FALSE(cache.GetDiscTitle(DISC, title));

  // The replacement is now the disc held
  cache.SetDiscTitle(DISC, "Other");
  cache.CheckDisc(DISC, "2:100");
  EXPECT_TRUE(cache.GetDiscTitle(DISC, title));
  EXPECT_EQ(title, "Other");
}
