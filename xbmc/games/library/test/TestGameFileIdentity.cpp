/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/library/GameFileIdentity.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <string>
#include <string_view>
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

TEST(TestGameFileIdentity, HashesAGameFiledInAFolderInsideAnArchive)
{
  // nested.zip holds one file, under a folder as a merged arcade set files a
  // clone's chips. An archive without a zip's directory, such as a 7z, only
  // knows it by hashing it.
  GameFile file;
  ASSERT_TRUE(CGameFileIdentity::Identify(XBMC_REF_FILE_PATH("xbmc/games/library/test/nested.zip"),
                                          file, MediaFormat::CARTRIDGE));

  EXPECT_EQ(file.crc32, "f5765659");
  EXPECT_EQ(file.md5, "c8c67df7a396d5cd4b341a7c96692f7b");
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

TEST(TestGameFileIdentity, ReadsTheSerialOfADiscArchivedInAFolder)
{
  // disc-folder.zip holds disc.zip's files under a folder
  GameFile file;
  ASSERT_TRUE(CGameFileIdentity::Identify(
      XBMC_REF_FILE_PATH("xbmc/games/library/test/disc-folder.zip"), file, MediaFormat::DISC));

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

TEST(TestGameFileIdentity, HashesACartridgeTooLargeToTellFromADisc)
{
  // 65 MB of zeros, over the size a file of unknown media is hashed at
  XFILE::CFile* rom = XBMC_CREATETEMPFILE(".nds");
  ASSERT_NE(rom, nullptr);
  const std::vector<char> chunk(1024 * 1024);
  for (int i = 0; i < 65; ++i)
    ASSERT_EQ(rom->Write(chunk.data(), chunk.size()), static_cast<ssize_t>(chunk.size()));
  rom->Flush();
  const std::string path = XBMC_TEMPFILEPATH(rom);

  GameFile file;
  EXPECT_TRUE(CGameFileIdentity::Identify(path, file, MediaFormat::CARTRIDGE));
  EXPECT_EQ(file.crc32, "97d38551");
  EXPECT_EQ(file.md5, "0a9739d8f171bb3a216168a2b7376e2e");

  GameFile unknown;
  EXPECT_FALSE(CGameFileIdentity::Identify(path, unknown, MediaFormat::UNKNOWN));
  EXPECT_TRUE(unknown.crc32.empty());

  XBMC_DELETETEMPFILE(rom);
}

TEST(TestGameFileIdentity, DoesNotHashAnInstalledGamesLargestFile)
{
  // program.zip holds a program, a data file larger than it and a readme
  GameFile file;
  CGameFileIdentity::Identify(XBMC_REF_FILE_PATH("xbmc/games/library/test/program.zip"), file,
                              MediaFormat::DISK);

  EXPECT_TRUE(file.crc32.empty());
  EXPECT_TRUE(file.md5.empty());
  EXPECT_EQ(file.members.size(), 3U);
}

TEST(TestGameFileIdentity, KnowsAnArchivedDiscOnlyByItsSerial)
{
  // identity.zip holds a ROM, not a disc with a header
  GameFile file;
  EXPECT_FALSE(CGameFileIdentity::Identify(XBMC_REF_FILE_PATH(ARCHIVE), file, MediaFormat::DISC));
  EXPECT_TRUE(file.crc32.empty());
  EXPECT_TRUE(file.md5.empty());
}

TEST(TestGameFileIdentity, HashesALargeCartridgeInsideAnArchive)
{
  // large.zip holds 65 MB of zeros, over the size a file of unknown media is hashed at
  const std::string path = XBMC_REF_FILE_PATH("xbmc/games/library/test/large.zip");

  GameFile file;
  EXPECT_TRUE(CGameFileIdentity::Identify(path, file, MediaFormat::CARTRIDGE));
  EXPECT_EQ(file.crc32, "97d38551");
  EXPECT_EQ(file.md5, "0a9739d8f171bb3a216168a2b7376e2e");

  // Unhashed, it is still known by the CRC in the zip's directory
  GameFile unknown;
  EXPECT_TRUE(CGameFileIdentity::Identify(path, unknown, MediaFormat::UNKNOWN));
  EXPECT_EQ(unknown.crc32, "97d38551");
  EXPECT_TRUE(unknown.md5.empty());
}

namespace
{
// A disc image of zeros with a header written at an offset
std::string WriteDisc(XFILE::CFile* disc,
                      size_t size,
                      size_t at,
                      std::string_view magic,
                      size_t serialOffset,
                      std::string_view serial)
{
  std::string data(size, '\0');
  data.replace(at, magic.size(), magic);
  data.replace(at + serialOffset, serial.size(), serial);
  EXPECT_EQ(disc->Write(data.data(), data.size()), static_cast<ssize_t>(data.size()));
  disc->Flush();
  return XBMC_TEMPFILEPATH(disc);
}
} // namespace

TEST(TestGameFileIdentity, ReadsTheSerialOfEachDiscHeader)
{
  struct Disc
  {
    std::string_view magic;
    size_t serialOffset;
    std::string_view serial;
    std::string_view expected;
  };
  const std::vector<Disc> discs{
      {"SEGA SEGAKATANA", 0x40, "MK-51171        ", "MK51171"},
      {"SEGA SEGASATURN", 0x20, "GS-9076   V1.000", "GS9076"},
      {"SEGADISCSYSTEM", 0x180, "GM MK-4407 -00  ", "MK4407"},
  };

  for (const Disc& d : discs)
  {
    XFILE::CFile* disc = XBMC_CREATETEMPFILE(".bin");
    ASSERT_NE(disc, nullptr);
    const std::string path = WriteDisc(disc, 64 * 1024, 0x10, d.magic, d.serialOffset, d.serial);
    EXPECT_EQ(CGameFileIdentity::ReadDiscSerial(path), d.expected) << d.magic;
    XBMC_DELETETEMPFILE(disc);
  }
}

TEST(TestGameFileIdentity, ReadsAHeaderThatStraddlesTwoReads)
{
  // Discs are read a megabyte at a time. Only "MK-5" of the serial is in the first.
  constexpr size_t READ = 1024 * 1024;
  XFILE::CFile* disc = XBMC_CREATETEMPFILE(".bin");
  ASSERT_NE(disc, nullptr);
  const std::string path =
      WriteDisc(disc, READ + 64 * 1024, READ - 0x44, "SEGA SEGAKATANA", 0x40, "MK-51171        ");

  EXPECT_EQ(CGameFileIdentity::ReadDiscSerial(path), "MK51171");
  XBMC_DELETETEMPFILE(disc);
}
