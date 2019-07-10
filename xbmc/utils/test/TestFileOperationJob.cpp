/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "test/TestUtils.h"
#include "utils/FileOperationJob.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

TEST(TestFileOperationJob, ActionCopy)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  std::string destpath = URIUtils::GetDirectory(tmpfilepath);
  destpath = URIUtils::AddFileToFolder(destpath, "copy");
  destfile = URIUtils::AddFileToFolder(destpath, URIUtils::GetFileName(tmpfilepath));
  ASSERT_FALSE(XFILE::CFile::Exists(destfile));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destfile));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CFile::Delete(destfile));
  EXPECT_TRUE(XFILE::CDirectory::Remove(destpath));
}

TEST(TestFileOperationJob, ActionMove)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  std::string destpath = URIUtils::GetDirectory(tmpfilepath);
  destpath = URIUtils::AddFileToFolder(destpath, "move");
  destfile = URIUtils::AddFileToFolder(destpath, URIUtils::GetFileName(tmpfilepath));
  ASSERT_FALSE(XFILE::CFile::Exists(destfile));
  ASSERT_TRUE(XFILE::CDirectory::Create(destpath));

  job.SetFileOperation(CFileOperationJob::ActionMove, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionMove, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destfile));

  EXPECT_TRUE(XFILE::CFile::Delete(destfile));
  EXPECT_TRUE(XFILE::CDirectory::Remove(destpath));
}

TEST(TestFileOperationJob, ActionDelete)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  std::string destpath = URIUtils::GetDirectory(tmpfilepath);
  destpath = URIUtils::AddFileToFolder(destpath, "delete");
  destfile = URIUtils::AddFileToFolder(destpath, URIUtils::GetFileName(tmpfilepath));
  ASSERT_FALSE(XFILE::CFile::Exists(destfile));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destfile));

  job.SetFileOperation(CFileOperationJob::ActionDelete, items, "");
  EXPECT_EQ(CFileOperationJob::ActionDelete, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CFile::Exists(tmpfilepath));

  items.Clear();
  CFileItemPtr item2(new CFileItem(destfile));
  item2->SetPath(destfile);
  item2->m_bIsFolder = false;
  item2->Select(true);
  items.Add(item2);

  job.SetFileOperation(CFileOperationJob::ActionDelete, items, "");
  EXPECT_EQ(CFileOperationJob::ActionDelete, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CFile::Exists(destfile));
  EXPECT_TRUE(XFILE::CDirectory::Remove(destpath));
}

TEST(TestFileOperationJob, ActionReplace)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  std::string destpath = URIUtils::GetDirectory(tmpfilepath);
  destpath = URIUtils::AddFileToFolder(destpath, "replace");
  destfile = URIUtils::AddFileToFolder(destpath, URIUtils::GetFileName(tmpfilepath));
  ASSERT_FALSE(XFILE::CFile::Exists(destfile));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destfile));

  job.SetFileOperation(CFileOperationJob::ActionReplace, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionReplace, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(destfile));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CFile::Delete(destfile));
  EXPECT_TRUE(XFILE::CDirectory::Remove(destpath));
}

TEST(TestFileOperationJob, ActionCreateFolder)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  std::string tmpfiledirectory =
    CXBMCTestUtils::Instance().TempFileDirectory(tmpfile);

  tmpfile->Close();

  destpath = tmpfilepath;
  destpath += ".createfolder";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  CFileItemPtr item(new CFileItem(destpath));
  item->SetPath(destpath);
  item->m_bIsFolder = true;
  item->Select(true);
  items.Add(item);

  job.SetFileOperation(CFileOperationJob::ActionCreateFolder, items, tmpfiledirectory);
  EXPECT_EQ(CFileOperationJob::ActionCreateFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CDirectory::Exists(destpath));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CDirectory::Remove(destpath));
}

// This test will fail until ActionDeleteFolder has a proper implementation
TEST(TestFileOperationJob, ActionDeleteFolder)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  std::string tmpfiledirectory =
    CXBMCTestUtils::Instance().TempFileDirectory(tmpfile);

  tmpfile->Close();

  destpath = tmpfilepath;
  destpath += ".deletefolder";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  CFileItemPtr item(new CFileItem(destpath));
  item->SetPath(destpath);
  item->m_bIsFolder = true;
  item->Select(true);
  items.Add(item);

  job.SetFileOperation(CFileOperationJob::ActionCreateFolder, items, tmpfiledirectory);
  EXPECT_EQ(CFileOperationJob::ActionCreateFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CDirectory::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionDeleteFolder, items, tmpfiledirectory);
  EXPECT_EQ(CFileOperationJob::ActionDeleteFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CDirectory::Exists(destpath));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}

TEST(TestFileOperationJob, GetFunctions)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  std::string destpath = URIUtils::GetDirectory(tmpfilepath);
  destpath = URIUtils::AddFileToFolder(destpath, "getfunctions");
  destfile = URIUtils::AddFileToFolder(destpath, URIUtils::GetFileName(tmpfilepath));
  ASSERT_FALSE(XFILE::CFile::Exists(destfile));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destfile));

  std::cout << "GetAverageSpeed(): " << job.GetAverageSpeed() << std::endl;
  std::cout << "GetCurrentOperation(): " << job.GetCurrentOperation() << std::endl;
  std::cout << "GetCurrentFile(): " << job.GetCurrentFile() << std::endl;
  EXPECT_FALSE(job.GetItems().IsEmpty());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CFile::Delete(destfile));
  EXPECT_TRUE(XFILE::CDirectory::Remove(destpath));
}
