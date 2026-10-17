/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "games/library/GameFileIdentity.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
// identity.zip holds a single 6 MB member, deflated. The size matters: the zip
// backend copies a deflated member over 4 MB into special://temp unless the read
// asks it not to, and nothing ever deletes that copy.
constexpr auto ARCHIVE = "xbmc/games/library/test/identity.zip";

// The CRC-32 that PKZIP stores and the ROM catalogues index by
constexpr auto MEMBER_CRC32 = "eeea5a12";
constexpr auto MEMBER_MD5 = "6835177061dbf19a6a4436a16dbb6cd7";
constexpr uint64_t MEMBER_SIZE = 6 * 1024 * 1024;

std::vector<std::string> TempFileNames()
{
  CFileItemList items;
  XFILE::CDirectory::GetDirectory("special://temp/", items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);

  std::vector<std::string> names;
  for (const auto& item : items)
  {
    if (!item->IsFolder())
      names.emplace_back(URIUtils::GetFileName(item->GetPath()));
  }
  return names;
}
} // namespace

TEST(TestGameFileIdentity, HashesTheGameInsideAnArchive)
{
  GameFile file;
  ASSERT_TRUE(
      CGameFileIdentity::Identify(XBMC_REF_FILE_PATH(ARCHIVE), file, MediaFormat::CARTRIDGE));

  // The hash is of the member, not of the archive around it
  EXPECT_EQ(file.crc32, MEMBER_CRC32);
  EXPECT_EQ(file.md5, MEMBER_MD5);
  EXPECT_EQ(file.size, MEMBER_SIZE);
}

TEST(TestGameFileIdentity, ReadsAnArchivedGameWithoutExtractingIt)
{
  const std::vector<std::string> before = TempFileNames();

  GameFile file;
  ASSERT_TRUE(
      CGameFileIdentity::Identify(XBMC_REF_FILE_PATH(ARCHIVE), file, MediaFormat::CARTRIDGE));

  EXPECT_EQ(TempFileNames(), before);
}

TEST(TestGameFileIdentity, ListsEveryFileOfAnArcadeSet)
{
  // arcade.zip holds three small files, one of them in a folder
  GameFile file;
  CGameFileIdentity::Identify(XBMC_REF_FILE_PATH("xbmc/games/library/test/arcade.zip"), file,
                              MediaFormat::CARTRIDGE);

  ASSERT_EQ(file.members.size(), 3U);
  EXPECT_EQ(file.members[0].name, "074-p1.p1");
  EXPECT_EQ(file.members[0].size, 120U);
  EXPECT_EQ(file.members[0].crc32, "2480cadb");
  EXPECT_EQ(file.members[2].name, "sub/074-s1.s1");
  EXPECT_EQ(file.members[2].crc32, "01281d6c");
}

TEST(TestGameFileIdentity, KnowsAZipOfOneFileInAFolderByItsDirectory)
{
  // nested.zip holds one file, under a folder as a merged arcade set files a
  // clone's chips. Only the zip's directory says what it is.
  GameFile file;
  ASSERT_TRUE(CGameFileIdentity::Identify(XBMC_REF_FILE_PATH("xbmc/games/library/test/nested.zip"),
                                          file, MediaFormat::CARTRIDGE));

  EXPECT_EQ(file.crc32, "f5765659");
  EXPECT_EQ(file.size, 45U);
  EXPECT_TRUE(file.members.empty());
}

TEST(TestGameFileIdentity, ListsNothingForAZipOfOneGame)
{
  GameFile file;
  CGameFileIdentity::Identify(XBMC_REF_FILE_PATH(ARCHIVE), file, MediaFormat::CARTRIDGE);

  EXPECT_TRUE(file.members.empty());
}

TEST(TestGameFileIdentity, KnowsASetFiledInAFolderByItsFiles)
{
  // folder.zip holds two files, both under one folder
  GameFile file;
  ASSERT_TRUE(CGameFileIdentity::Identify(XBMC_REF_FILE_PATH("xbmc/games/library/test/folder.zip"),
                                          file, MediaFormat::CARTRIDGE));

  ASSERT_EQ(file.members.size(), 2U);
  EXPECT_EQ(file.members[0].name, "set/074-p1.p1");
  EXPECT_EQ(file.members[1].crc32, "d22102e3");
}

TEST(TestGameFileIdentity, ReadsTheSerialOfAnArchivedDisc)
{
  // disc.zip holds a cue sheet, its data track and a larger audio track
  GameFile file;
  ASSERT_TRUE(CGameFileIdentity::Identify(XBMC_REF_FILE_PATH("xbmc/games/library/test/disc.zip"),
                                          file, MediaFormat::DISC));

  EXPECT_EQ(file.serial, "SLUS01234");
}

TEST(TestGameFileIdentity, ReadsAQuotedGdiTrack)
{
  const std::string track =
      CGameFileIdentity::FirstTrack(XBMC_REF_FILE_PATH("xbmc/games/library/test/quoted.gdi"));

  EXPECT_EQ(URIUtils::GetFileName(track), "Crazy Taxi (Track 3).bin");
}

TEST(TestGameFileIdentity, StartsAfreshForEachFile)
{
  GameFile file;
  CGameFileIdentity::Identify(XBMC_REF_FILE_PATH("xbmc/games/library/test/arcade.zip"), file,
                              MediaFormat::CARTRIDGE);
  ASSERT_FALSE(file.members.empty());

  CGameFileIdentity::Identify(XBMC_REF_FILE_PATH(ARCHIVE), file, MediaFormat::CARTRIDGE);

  EXPECT_TRUE(file.members.empty());
  EXPECT_EQ(file.crc32, MEMBER_CRC32);
}
