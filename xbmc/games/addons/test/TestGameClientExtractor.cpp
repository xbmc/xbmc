/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameUtils.h"
#include "games/addons/GameClientExtractor.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
std::string Archive()
{
  return XBMC_REF_FILE_PATH("xbmc/games/addons/test/testdata/archived-games.zip");
}

std::string Extract(const std::string& pathInArchive, bool supportsFolders = false)
{
  return CGameClientExtractor(supportsFolders, true)
      .Extract(URIUtils::CreateArchivePath("zip", CURL(Archive()), pathInArchive).Get());
}

std::string Read(const std::string& path)
{
  std::vector<uint8_t> data;
  if (XFILE::CFile().LoadFile(path, data) < 0)
    return "";
  return std::string(data.begin(), data.end());
}
} // namespace

TEST(TestGameClientExtractor, KeepsFilesOfTheSameNameApart)
{
  const std::string first = Extract("A/disk.hdm");
  const std::string second = Extract("B/disk.hdm");

  EXPECT_NE(first, second);
  EXPECT_EQ(Read(first), "disk A");
  EXPECT_EQ(Read(second), "disk B");
}

TEST(TestGameClientExtractor, CopiesBesideTheGameFolders)
{
  EXPECT_EQ(Extract("A/disk.hdm"),
            CSpecialProtocol::TranslatePath(CGameUtils::GetGameFolder(Archive()) +
                                            "_extracted/A/disk.hdm"));
}

TEST(TestGameClientExtractor, CopiesTheTracksOfACueSheet)
{
  const std::string game = Extract("cd/game.cue");

  ASSERT_TRUE(URIUtils::HasExtension(game, ".cue"));
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(URIUtils::GetDirectory(game), "game.bin")), "track 1");
}

TEST(TestGameClientExtractor, CopiesTheDisksOfAPlaylist)
{
  const std::string game = Extract("set/game.m3u");

  EXPECT_EQ(Read(game), "disc1.chd\ndisc2.chd\n");
  const std::string folder = URIUtils::GetDirectory(game);
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(folder, "disc1.chd")), "disc 1");
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(folder, "disc2.chd")), "disc 2");
}

TEST(TestGameClientExtractor, LeavesTheArchivesOwnPlaylistAlone)
{
  const std::string game = Extract("discs/game.cue");

  EXPECT_EQ(Read(game), "game.cue\ngame2.cue\n");
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(URIUtils::GetDirectory(game), "game.m3u")),
            "game2.cue\ngame.cue\n");
}

TEST(TestGameClientExtractor, KeepsAFileNamedLikeACopyInProgress)
{
  // Copies are kept, so start from nothing for the clash to show
  const std::string folder = CGameUtils::GetGameFolder(Archive()) + "_extracted/";
  XFILE::CDirectory::RemoveRecursive(folder);

  const std::string game = Extract("dos/GAME.EXE", true);
  ASSERT_FALSE(game.empty());
  const std::string dos = URIUtils::GetDirectory(game);
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(dos, "GAME.DAT.tmp")), "saved data");
  EXPECT_EQ(Read(URIUtils::AddFileToFolder(dos, "GAME.DAT")), "game data");
  EXPECT_EQ(Read(game), "program");
}

TEST(TestGameClientExtractor, CopiesAFileWithALongName)
{
  // A name with little room left under the 255-byte limit most filesystems set
  const std::string game = Extract("long/" + std::string(220, 'L') + ".bin");
  ASSERT_FALSE(game.empty());
  EXPECT_EQ(Read(game), "long name");

  // The copy's path is longer than the test environment's own clean-up can
  // remove on Windows, so the VFS removes it
  XFILE::CDirectory::RemoveRecursive(CGameUtils::GetGameFolder(Archive()) + "_extracted/");
}
