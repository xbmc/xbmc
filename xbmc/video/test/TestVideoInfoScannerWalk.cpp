/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/IDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoScannerWalk.h"

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

// A hash stored before records, or anything else, is no record
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

// The bar runs once over a source, and never back as folders are found
TEST(TestVideoInfoScannerWalk, ProgressRunsOnceOverASource)
{
  CScanProgress progress;
  progress.Start({"smb://host/Movies/"}, "smb://host/Movies/");
  progress.Found(3);
  EXPECT_FLOAT_EQ(0.f, progress.Percentage());

  progress.Finished();
  EXPECT_FLOAT_EQ(25.f, progress.Percentage());

  // Folders found make the whole larger, which holds the bar where it is
  progress.Found(4);
  EXPECT_FLOAT_EQ(25.f, progress.Percentage());

  progress.Finished(3);
  EXPECT_FLOAT_EQ(50.f, progress.Percentage());
  EXPECT_FLOAT_EQ(56.25f, progress.Percentage(0.5f));

  progress.Finished(4);
  EXPECT_FLOAT_EQ(100.f, progress.Percentage());
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

TEST(TestVideoInfoScannerWalk, EachSourceHasABarOfItsOwn)
{
  CScanProgress progress;
  progress.Start({"smb://host/Films/", "smb://host/Movies/"}, "smb://host/Films/");
  progress.Found(99);
  progress.Finished(100);
  EXPECT_FLOAT_EQ(100.f, progress.Percentage());

  // The next source starts again, its folders found as it is listed
  progress.Start({"smb://host/Movies/"}, "smb://host/Movies/");
  EXPECT_FLOAT_EQ(0.f, progress.Percentage());
  progress.Found(9);
  progress.Finished(5);
  EXPECT_FLOAT_EQ(50.f, progress.Percentage());
  progress.Finished(5);
  EXPECT_FLOAT_EQ(100.f, progress.Percentage());
}

TEST(TestVideoInfoScannerWalk, TheFoldersBelowASourceTheScanWasAskedToScanAreKnown)
{
  CScanProgress progress;
  // A source of the library, rescanned with the folders of it the library holds
  progress.Start({"smb://host/Movies/", "smb://host/Movies/A/", "smb://host/Movies/B/",
                  "smb://host/Movies/B/Disc 1/", "smb://host/TV/"},
                 "smb://host/Movies/");
  progress.Finished(2);
  EXPECT_FLOAT_EQ(50.f, progress.Percentage());
  progress.Finished(2);
  EXPECT_FLOAT_EQ(100.f, progress.Percentage());
}

// A source holding one folder of movies runs from start to end over that folder
TEST(TestVideoInfoScannerWalk, AFolderLeftOutTakesNoShareOfTheBar)
{
  CScanProgress progress;
  progress.Start({"smb://host/Collections/"}, "smb://host/Collections/");
  progress.Found();
  progress.LeaveOut();
  EXPECT_FLOAT_EQ(50.f, progress.Percentage(0.5f));

  progress.Finished();
  EXPECT_FLOAT_EQ(100.f, progress.Percentage());
}
