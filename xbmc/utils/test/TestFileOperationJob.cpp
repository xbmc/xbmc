/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/FileOperationJob.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "utils/URIUtils.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

TEST(TestFileOperationJob, ActionCopy)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  URIUtils::GetDirectory(tmpfilepath, destpath);
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
  CStdString tmpfilepath, destpath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  URIUtils::GetDirectory(tmpfilepath, destpath);
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
  CStdString tmpfilepath, destpath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  URIUtils::GetDirectory(tmpfilepath, destpath);
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
  CStdString tmpfilepath, destpath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  URIUtils::GetDirectory(tmpfilepath, destpath);
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

// This test will fail until ActionCreateFolder has a proper implementation
TEST(TestFileOperationJob, ActionCreateFolder)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  destpath = tmpfilepath;
  destpath += ".createfolder";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  CFileItemPtr item(new CFileItem(destpath));
  item->SetPath(destpath);
  item->m_bIsFolder = true;
  item->Select(true);
  items.Add(item);

  job.SetFileOperation(CFileOperationJob::ActionCreateFolder, items, destpath);
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
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  destpath = tmpfilepath;
  destpath += ".deletefolder";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  CFileItemPtr item(new CFileItem(destpath));
  item->SetPath(destpath);
  item->m_bIsFolder = true;
  item->Select(true);
  items.Add(item);

  job.SetFileOperation(CFileOperationJob::ActionCreateFolder, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCreateFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CDirectory::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionDeleteFolder, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionDeleteFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CDirectory::Exists(destpath));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}

TEST(TestFileOperationJob, GetFunctions)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath, destfile;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);
  tmpfile->Close();

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  URIUtils::GetDirectory(tmpfilepath, destpath);
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
