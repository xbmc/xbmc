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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/FileOperationJob.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

TEST(TestFileOperationJob, ActionCopy)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  destpath = tmpfilepath;
  destpath += ".copy";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destpath));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CFile::Delete(destpath));
}

TEST(TestFileOperationJob, ActionMove)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  destpath = tmpfilepath;
  destpath += ".move";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionMove, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionMove, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destpath));

  EXPECT_TRUE(XFILE::CFile::Delete(destpath));
}

TEST(TestFileOperationJob, ActionDelete)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  destpath = tmpfilepath;
  destpath += ".delete";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionDelete, items, "");
  EXPECT_EQ(CFileOperationJob::ActionDelete, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CFile::Exists(tmpfilepath));

  items.Clear();
  CFileItemPtr item2(new CFileItem(destpath));
  item->SetPath(destpath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item2);

  job.SetFileOperation(CFileOperationJob::ActionDelete, items, "");
  EXPECT_EQ(CFileOperationJob::ActionDelete, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CFile::Exists(destpath));
}

TEST(TestFileOperationJob, ActionReplace)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  destpath = tmpfilepath;
  destpath += ".replace";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionReplace, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionReplace, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(destpath));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CFile::Delete(destpath));
}

TEST(TestFileOperationJob, ActionCreateFolder)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  destpath = tmpfilepath;
  destpath += ".createfolder";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionCreateFolder, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCreateFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CDirectory::Exists(destpath));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CDirectory::Remove(destpath));
}

TEST(TestFileOperationJob, ActionDeleteFolder)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  destpath = tmpfilepath;
  destpath += ".deletefolder";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionCreateFolder, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCreateFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CDirectory::Exists(destpath));

  items.Clear();
  CFileItemPtr item2(new CFileItem(destpath));
  item->SetPath(destpath);
  item->m_bIsFolder = true;
  item->Select(true);
  items.Add(item2);

  job.SetFileOperation(CFileOperationJob::ActionDeleteFolder, items, "");
  EXPECT_EQ(CFileOperationJob::ActionDeleteFolder, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_FALSE(XFILE::CDirectory::Exists(destpath));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}

TEST(TestFileOperationJob, GetFunctions)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  CFileItemList items;
  CFileOperationJob job;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);
  items.Add(item);

  destpath = tmpfilepath;
  destpath += ".getfunctions";
  ASSERT_FALSE(XFILE::CFile::Exists(destpath));

  job.SetFileOperation(CFileOperationJob::ActionCopy, items, destpath);
  EXPECT_EQ(CFileOperationJob::ActionCopy, job.GetAction());

  EXPECT_TRUE(job.DoWork());
  EXPECT_TRUE(XFILE::CFile::Exists(tmpfilepath));
  EXPECT_TRUE(XFILE::CFile::Exists(destpath));

  fprintf(stdout, "GetAverageSpeed(): %s\n", job.GetAverageSpeed().c_str());
  fprintf(stdout, "GetCurrentOperation(): %s\n",
          job.GetCurrentOperation().c_str());
  fprintf(stdout, "GetCurrentFile(): %s\n", job.GetCurrentFile().c_str());
  EXPECT_FALSE(job.GetItems().IsEmpty());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
  EXPECT_TRUE(XFILE::CFile::Delete(destpath));
}
