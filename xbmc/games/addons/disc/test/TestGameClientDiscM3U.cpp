/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto TEMP_PLAYLIST_DIRECTORY = "special://temp/test-disc-playlists";

std::string CreatePlaylistFile(const std::string& fileName, const std::string& m3uBody)
{
  EXPECT_TRUE(XFILE::CDirectory::Create(TEMP_PLAYLIST_DIRECTORY));

  const std::string playlistPath = URIUtils::AddFileToFolder(TEMP_PLAYLIST_DIRECTORY, fileName);
  XFILE::CFile file;
  if (!file.OpenForWrite(playlistPath, true))
    return playlistPath;

  EXPECT_EQ(file.Write(m3uBody.data(), m3uBody.size()), static_cast<ssize_t>(m3uBody.size()));
  file.Close();

  return playlistPath;
}

void DeletePlaylistFile(const std::string& playlistPath)
{
  XFILE::CFile::Delete(playlistPath);
}

void CleanupPlaylistDirectory()
{
  if (XFILE::CDirectory::Exists(TEMP_PLAYLIST_DIRECTORY))
    XFILE::CDirectory::Remove(TEMP_PLAYLIST_DIRECTORY);
}

} // namespace

TEST(TestGameClientDiscM3U, LoadReadsDiscsFromSuppliedPlaylistPath)
{
  const std::string playlistPath =
      CreatePlaylistFile("load-read-supplied-path.m3u", "/roms/disc1.chd\n/roms/disc2.chd\n");

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(playlistPath, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 2U);
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetPathByIndex(1), "/roms/disc2.chd");

  DeletePlaylistFile(playlistPath);
  CleanupPlaylistDirectory();
}

TEST(TestGameClientDiscM3U, LoadResolvesRelativeEntriesAgainstPlaylistDirectory)
{
  const std::string playlistPath = CreatePlaylistFile(
      "load-relative-entries.m3u",
      "Disc 1/Metal Gear Solid (Disc 1).cue\nDisc 2/Metal Gear Solid (Disc 2).cue\n");
  const std::string playlistDirectory = URIUtils::GetDirectory(playlistPath);

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(playlistPath, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 2U);
  EXPECT_EQ(loadedModel.GetPathByIndex(0),
            URIUtils::AddFileToFolder(playlistDirectory, "Disc 1/Metal Gear Solid (Disc 1).cue"));
  EXPECT_EQ(loadedModel.GetPathByIndex(1),
            URIUtils::AddFileToFolder(playlistDirectory, "Disc 2/Metal Gear Solid (Disc 2).cue"));

  DeletePlaylistFile(playlistPath);
  CleanupPlaylistDirectory();
}

TEST(TestGameClientDiscM3U, LoadPreservesAbsoluteEntries)
{
  const std::string playlistPath = CreatePlaylistFile(
      "load-absolute-entries.m3u", "/roms/disc1.chd\nsmb://server/psx/disc2.chd\n");

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(playlistPath, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 2U);
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetPathByIndex(1), "smb://server/psx/disc2.chd");

  DeletePlaylistFile(playlistPath);
  CleanupPlaylistDirectory();
}

TEST(TestGameClientDiscM3U, LoadProducesStableAbsolutePathsForRestore)
{
  const std::string playlistPath = CreatePlaylistFile(
      "load-stable-restore-paths.m3u",
      "Disc 1/Metal Gear Solid (Disc 1).cue\nDisc 2/Metal Gear Solid (Disc 2).cue\n");
  const std::string playlistDirectory = URIUtils::GetDirectory(playlistPath);

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(playlistPath, loadedModel));

  const std::string expectedDisc1 =
      URIUtils::AddFileToFolder(playlistDirectory, "Disc 1/Metal Gear Solid (Disc 1).cue");
  const std::string expectedDisc2 =
      URIUtils::AddFileToFolder(playlistDirectory, "Disc 2/Metal Gear Solid (Disc 2).cue");

  ASSERT_EQ(loadedModel.Size(), 2U);
  // CI may keep special://temp paths as URL-style absolute paths, while local runs can
  // materialize plain POSIX paths. Validate the actual contract: entries are stably
  // resolved against the playlist directory (i.e. not left as raw relative paths).
  EXPECT_EQ(loadedModel.GetPathByIndex(0), expectedDisc1);
  EXPECT_EQ(loadedModel.GetPathByIndex(1), expectedDisc2);

  DeletePlaylistFile(playlistPath);
  CleanupPlaylistDirectory();
}

TEST(TestGameClientDiscM3U, LoadIgnoresEmptyAndCommentLinesInSuppliedPlaylist)
{
  const std::string playlistPath = CreatePlaylistFile(
      "load-ignores-comments.m3u", "\n#EXTM3U\n/roms/disc1.chd\n   \n# comment\n/roms/disc2.chd\n");

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(playlistPath, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 2U);
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetPathByIndex(1), "/roms/disc2.chd");

  DeletePlaylistFile(playlistPath);
  CleanupPlaylistDirectory();
}

TEST(TestGameClientDiscM3U, LoadMissingPlaylistIsNonErrorAndLeavesEmptyModel)
{
  const std::string missingPlaylistPath =
      URIUtils::AddFileToFolder(TEMP_PLAYLIST_DIRECTORY, "missing-playlist.m3u");

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;

  ASSERT_TRUE(discM3U.Load(missingPlaylistPath, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());
}
