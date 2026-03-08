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
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto GAME_PATH = "/roms/my_game.m3u";
constexpr auto TEMP_PLAYLIST_DIRECTORY = "special://temp/test-disc-playlists";

void CleanupStateFile()
{
  const std::string stateM3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);

  XFILE::CFile::Delete(stateM3uPath);

  std::string stateSubdirectory = URIUtils::GetDirectory(stateM3uPath);
  URIUtils::RemoveSlashAtEnd(stateSubdirectory);
  if (!stateSubdirectory.empty() && XFILE::CDirectory::Exists(stateSubdirectory))
    XFILE::CDirectory::Remove(stateSubdirectory);
}

void EnsureStateSubdirectory()
{
  const std::string stateDirectory =
      URIUtils::GetDirectory(CGameClientDiscM3U::GetM3UPath(GAME_PATH));
  ASSERT_TRUE(XFILE::CDirectory::Create(stateDirectory));
}

std::string ReadStateM3U()
{
  const std::string stateM3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);

  XFILE::CFile file;
  if (!file.Open(stateM3uPath))
    return "";

  std::string m3u;
  m3u.resize(static_cast<size_t>(file.GetLength()));
  if (!m3u.empty())
    file.Read(m3u.data(), m3u.size());

  file.Close();
  return m3u;
}

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

TEST(TestGameClientDiscM3U, SaveWritesM3UWithTwoDiscs)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.AddDisc("/roms/disc2.chd");

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, "/roms/disc1.chd\n/roms/disc2.chd\n");

  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, SaveOmitsRemovedSlotsFromM3U)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.AddRemovedSlot();
  savedModel.AddDisc("/roms/disc3.chd");

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, "/roms/disc1.chd\n/roms/disc3.chd\n");

  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, SaveNormalizesBinToCueInM3UWhenCueExists)
{
  CleanupStateFile();

  const std::string tempDiscDirectory = "special://temp/test-disc-inputs";
  ASSERT_TRUE(XFILE::CDirectory::Create(tempDiscDirectory));
  const std::string binPath = URIUtils::AddFileToFolder(tempDiscDirectory, "disc1.bin");
  const std::string cuePath = URIUtils::ReplaceExtension(binPath, ".cue");

  XFILE::CFile cueFile;
  ASSERT_TRUE(cueFile.OpenForWrite(cuePath, true));
  cueFile.Close();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc(binPath);

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, cuePath + "\n");

  XFILE::CFile::Delete(cuePath);
  XFILE::CDirectory::Remove(tempDiscDirectory);
  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, LoadReadsDiscsFromSuppliedPlaylistPath)
{
  CleanupStateFile();

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
  CleanupStateFile();

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
  CleanupStateFile();

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
  CleanupStateFile();

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

  ASSERT_TRUE(discM3U.Save(GAME_PATH, loadedModel));
  const std::string persistedM3U = ReadStateM3U();

  EXPECT_EQ(persistedM3U, expectedDisc1 + "\n" + expectedDisc2 + "\n");

  DeletePlaylistFile(playlistPath);
  CleanupPlaylistDirectory();
  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, LoadIgnoresEmptyAndCommentLinesInSuppliedPlaylist)
{
  CleanupStateFile();

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

TEST(TestGameClientDiscM3U, LoadStartupSeedingUsesRealPlaylistPathNotPersistedStatePath)
{
  CleanupStateFile();
  EnsureStateSubdirectory();

  const std::string persistedM3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);
  XFILE::CFile persistedFile;
  ASSERT_TRUE(persistedFile.OpenForWrite(persistedM3uPath, true));
  static constexpr char persistedM3u[] = "/persisted/disc-do-not-read.chd\n";
  ASSERT_EQ(persistedFile.Write(persistedM3u, sizeof(persistedM3u) - 1), sizeof(persistedM3u) - 1);
  persistedFile.Close();

  const std::string playlistPath =
      CreatePlaylistFile("startup-seeding-playlist.m3u", "/roms/disc-from-launch-playlist.chd\n");

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(playlistPath, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 1U);
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc-from-launch-playlist.chd");

  DeletePlaylistFile(playlistPath);
  CleanupPlaylistDirectory();
  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, LoadMissingPlaylistIsNonErrorAndLeavesEmptyModel)
{
  CleanupStateFile();

  const std::string missingPlaylistPath =
      URIUtils::AddFileToFolder(TEMP_PLAYLIST_DIRECTORY, "missing-playlist.m3u");

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;

  ASSERT_TRUE(discM3U.Load(missingPlaylistPath, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());
}

TEST(TestGameClientDiscM3U, GetM3UPathUsesPerGameDirectoryAndExtensionlessBaseName)
{
  const std::string m3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);

  EXPECT_EQ(URIUtils::GetFileName(m3uPath), "my_game.m3u");
  EXPECT_EQ(URIUtils::GetExtension(m3uPath), ".m3u");
  EXPECT_EQ(m3uPath.find("my_game.m3u.m3u"), std::string::npos);

  std::string m3uDirectoryName = URIUtils::GetDirectory(m3uPath);
  URIUtils::RemoveSlashAtEnd(m3uDirectoryName);
  m3uDirectoryName = URIUtils::GetFileName(m3uDirectoryName);

  EXPECT_TRUE(StringUtils::StartsWith(m3uDirectoryName, "my_game.m3u_"));
}

TEST(TestGameClientDiscM3U, SaveCreatesPerGameStateFile)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  EXPECT_TRUE(CFileUtils::Exists(CGameClientDiscM3U::GetM3UPath(GAME_PATH)));

  CleanupStateFile();
}
