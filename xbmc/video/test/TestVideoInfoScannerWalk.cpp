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
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/IDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoScannerWalk.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

using namespace KODI::VIDEO;

TEST(TestVideoInfoScannerWalk, ARecordIsReadAsItWasStored)
{
  const FolderRecord stored{
      .cfg = "abc", .mtime = 1793970048, .digest = "0123", .hasSubfolders = true, .noMedia = true};

  const std::optional<FolderRecord> read{FolderRecord::Parse(stored.Format())};

  ASSERT_TRUE(read);
  EXPECT_EQ("abc", read->cfg);
  EXPECT_EQ(1793970048, read->mtime);
  EXPECT_EQ("0123", read->digest);
  EXPECT_TRUE(read->hasSubfolders);
  EXPECT_TRUE(read->noMedia);
  EXPECT_EQ(stored.Format(), read->Format());
}

TEST(TestVideoInfoScannerWalk, ARecordWithoutItsTimeKeepsNone)
{
  const std::optional<FolderRecord> read{FolderRecord::Parse("2|abc|?|0123|")};

  ASSERT_TRUE(read);
  EXPECT_FALSE(read->mtime);
  EXPECT_FALSE(read->hasSubfolders);
  EXPECT_EQ("2|abc|?|0123|", read->Format());
}

TEST(TestVideoInfoScannerWalk, AnythingButARecordIsNone)
{
  EXPECT_FALSE(FolderRecord::Parse(""));
  EXPECT_FALSE(FolderRecord::Parse("0123456789abcdef0123456789abcdef"));
  EXPECT_FALSE(FolderRecord::Parse("1|abc|1793970048|0123|"));
  EXPECT_FALSE(FolderRecord::Parse("2|abc|1793970048|0123"));
  EXPECT_FALSE(FolderRecord::Parse("2|abc|x179|0123|"));
}

TEST(TestVideoInfoScannerWalk, AFolderIsUnchangedUnlistedOnlyByItsKnownTimeAndSettings)
{
  const FolderRecord record{.cfg = "abc", .mtime = 100, .digest = "0123"};

  EXPECT_TRUE(record.UnchangedUnlisted("abc", 100, true));
  EXPECT_FALSE(record.UnchangedUnlisted("abc", 101, true));
  EXPECT_FALSE(record.UnchangedUnlisted("abd", 100, true));
  EXPECT_FALSE(record.UnchangedUnlisted("abc", std::nullopt, true));
  // Without the fast hash, the time doesn't stand for what the folder holds
  EXPECT_FALSE(record.UnchangedUnlisted("abc", 100, false));

  FolderRecord withSubfolders{record};
  withSubfolders.hasSubfolders = true;
  EXPECT_FALSE(withSubfolders.UnchangedUnlisted("abc", 100, true));
}

TEST(TestVideoInfoScannerWalk, AListingIsUnchangedByItsDigestAndSettings)
{
  const FolderRecord record{.cfg = "abc", .mtime = 100, .digest = "0123"};

  EXPECT_TRUE(record.UnchangedListing("abc", "0123"));
  EXPECT_FALSE(record.UnchangedListing("abc", "0124"));
  EXPECT_FALSE(record.UnchangedListing("abd", "0123"));
  EXPECT_FALSE(FolderRecord{.cfg = "abc"}.UnchangedListing("abc", ""));
}

namespace
{
class TestDirectorySnapshot : public ::testing::Test
{
protected:
  void SetUp() override
  {
    XFILE::CDirectory::RemoveRecursive(m_folder);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_folder));
  }

  void TearDown() override { XFILE::CDirectory::RemoveRecursive(m_folder); }

  static std::string File(const std::string& folder, const std::string& name)
  {
    const std::string path{URIUtils::AddFileToFolder(folder, name)};
    XFILE::CFile file;
    EXPECT_TRUE(file.OpenForWrite(path, true));
    file.Close();
    return path;
  }

  static std::string Folder(const std::string& parent, const std::string& name)
  {
    std::string folder{URIUtils::AddFileToFolder(parent, name)};
    URIUtils::AddSlashAtEnd(folder);
    EXPECT_TRUE(XFILE::CDirectory::Create(folder));
    return folder;
  }

  const std::string m_folder{
      CSpecialProtocol::TranslatePath("special://temp/TestDirectorySnapshot/")};
};
} // namespace

// Hidden files are entries too, as .nomedia is
TEST_F(TestDirectorySnapshot, EveryEntryIsInTheListing)
{
  File(m_folder, "Film.mkv");
  File(m_folder, ".nomedia");
  Folder(m_folder, "Extras");

  const auto snapshot{CDirectorySnapshot::Take(m_folder)};

  ASSERT_TRUE(snapshot);
  EXPECT_EQ(3U, snapshot->Entries().size());
  EXPECT_TRUE(snapshot->HasNoMedia());
  ASSERT_TRUE(snapshot->Find("extras"));
  EXPECT_TRUE(snapshot->Find("extras")->isFolder);
  EXPECT_FALSE(snapshot->Find("Other.mkv"));
}

// As CInfoScanner::HasNoMedia() finds it, which a folder of the name isn't, nor another case of the
// name where the file system tells names of different cases apart
TEST_F(TestDirectorySnapshot, OnlyAFileNamedNomediaHoldsNoMedia)
{
  Folder(m_folder, ".nomedia");
  const std::string other{Folder(m_folder, "Other")};
  File(other, ".NOMEDIA");

  EXPECT_FALSE(CDirectorySnapshot::Take(m_folder)->HasNoMedia());
  EXPECT_EQ(XFILE::CFile::Exists(URIUtils::AddFileToFolder(other, ".nomedia"), false),
            CDirectorySnapshot::Take(other)->HasNoMedia());
}

TEST_F(TestDirectorySnapshot, TheSameListingHasTheSameDigestAndAnotherAnother)
{
  File(m_folder, "Film.mkv");
  const auto first{CDirectorySnapshot::Take(m_folder)};
  const auto again{CDirectorySnapshot::Take(m_folder)};
  File(m_folder, "Film.nfo");
  const auto changed{CDirectorySnapshot::Take(m_folder)};

  ASSERT_TRUE(first && again && changed);
  EXPECT_EQ(first->Digest(), again->Digest());
  EXPECT_NE(first->Digest(), changed->Digest());
}

// A file that stops being hidden, its size and time unchanged, changes the digest
TEST_F(TestDirectorySnapshot, WhetherAnEntryIsHiddenIsPartOfTheDigest)
{
  // Listed from the directory cache, as the file system's attribute can't be set everywhere
  const auto take{[this](bool hidden)
                  {
                    const auto film{std::make_shared<CFileItem>(
                        URIUtils::AddFileToFolder(m_folder, "Film.mkv"), false)};
                    film->SetProperty("file:hidden", hidden);
                    CFileItemList items;
                    items.Add(film);
                    g_directoryCache.SetDirectory(CURL{m_folder}, items, XFILE::CacheType::ALWAYS);
                    auto snapshot{CDirectorySnapshot::Take(m_folder)};
                    g_directoryCache.ClearDirectory(CURL{m_folder});
                    return snapshot;
                  }};

  const auto hidden{take(true)};
  const auto shown{take(false)};

  ASSERT_TRUE(hidden && shown);
  ASSERT_TRUE(hidden->Find("Film.mkv"));
  EXPECT_TRUE(hidden->Find("Film.mkv")->hidden);
  EXPECT_NE(hidden->Digest(), shown->Digest());
}

TEST_F(TestDirectorySnapshot, ItsItemsAreThoseOfTheMask)
{
  File(m_folder, "Film.mkv");
  File(m_folder, "Film.nfo");
  File(m_folder, ".nomedia");
  Folder(m_folder, "Extras");

  CFileItemList items;
  CDirectorySnapshot::Take(m_folder)->GetItems(items, ".mkv", false);

  // The video and the folder; neither the nfo nor the hidden file
  EXPECT_EQ(2, items.Size());
  EXPECT_EQ(m_folder, items.GetPath());
}

TEST_F(TestDirectorySnapshot, ADvdFolderRipIsFoundAsItsIfo)
{
  File(Folder(m_folder, "VIDEO_TS"), "VIDEO_TS.IFO");

  EXPECT_EQ(URIUtils::AddFileToFolder(m_folder, "VIDEO_TS", "VIDEO_TS.IFO"),
            CDirectorySnapshot::Take(m_folder)->GetDiscPath());
}

// As the file system matches names, which may take another case for the same name
TEST_F(TestDirectorySnapshot, ADiscFileOfAnotherCaseIsFoundWhereTheFileSystemFindsIt)
{
  File(m_folder, "video_ts.ifo");

  EXPECT_EQ(XFILE::CFile::Exists(URIUtils::AddFileToFolder(m_folder, "VIDEO_TS.IFO"), false),
            !CDirectorySnapshot::Take(m_folder)->GetDiscPath().empty());
}

#ifdef HAVE_LIBBLURAY
TEST_F(TestDirectorySnapshot, ABlurayFolderRipIsFoundAsItsIndex)
{
  File(Folder(m_folder, "BDMV"), "index.bdmv");

  EXPECT_EQ(URIUtils::AddFileToFolder(m_folder, "BDMV", "index.bdmv"),
            CDirectorySnapshot::Take(m_folder)->GetDiscPath());
}
#endif

TEST_F(TestDirectorySnapshot, AFolderOfFilesIsNoDisc)
{
  File(m_folder, "Film.mkv");
  Folder(m_folder, "Extras");

  EXPECT_TRUE(CDirectorySnapshot::Take(m_folder)->GetDiscPath().empty());
}

TEST(TestVideoInfoScannerWalk, AFileListedWithoutItsStatHasTheTimeTheListingReports)
{
  CFileItem file{"ftp://host/Film.mkv", false};
  EXPECT_FALSE(ListedFileTime(file));

  file.SetDateTime(time_t{1793970048});
  EXPECT_EQ(file.GetDateTime().GetAsSecondsSinceEpoch(), ListedFileTime(file));

  file.SetProperty(XFILE::DIR_PROPERTY_STAT_MTIME, int64_t{100});
  EXPECT_EQ(100, ListedFileTime(file));
}
