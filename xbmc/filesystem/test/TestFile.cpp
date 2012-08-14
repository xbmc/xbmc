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

#include "filesystem/File.h"
#include "test/TestUtils.h"

#include <errno.h>

#include "gtest/gtest.h"

TEST(TestFile, Read)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));

  ASSERT_TRUE(file.Open(
    XBMC_REF_FILE_PATH("/xbmc/filesystem/test/reffile.txt")));
  EXPECT_EQ(0, file.GetPosition());
  EXPECT_EQ(1616, file.GetLength());
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(20, file.GetPosition());
  EXPECT_TRUE(!memcmp("About\n-----\nXBMC is ", buf, sizeof(buf) - 1));
  EXPECT_TRUE(file.ReadString(buf, sizeof(buf)));
  EXPECT_EQ(39, file.GetPosition());
  EXPECT_STREQ("an award-winning fr", buf);
  EXPECT_EQ(100, file.Seek(100));
  EXPECT_EQ(100, file.GetPosition());
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(120, file.GetPosition());
  EXPECT_TRUE(!memcmp("ent hub for digital ", buf, sizeof(buf) - 1));
  EXPECT_EQ(220, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(220, file.GetPosition());
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(240, file.GetPosition());
  EXPECT_TRUE(!memcmp("rs, XBMC is a non-pr", buf, sizeof(buf) - 1));
  EXPECT_EQ(1596, file.Seek(-sizeof(buf), SEEK_END));
  EXPECT_EQ(1596, file.GetPosition());
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_TRUE(!memcmp("multimedia jukebox.\n", buf, sizeof(buf) - 1));
  EXPECT_EQ(1716, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(1716, file.GetPosition());
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(20, file.GetPosition());
  EXPECT_TRUE(!memcmp("About\n-----\nXBMC is ", buf, sizeof(buf) - 1));
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(-1, file.Seek(-100, SEEK_SET));
  file.Close();
}

TEST(TestFile, Write)
{
  XFILE::CFile *file;
  const char str[] = "TestFile.Write test string\n";
  char buf[30];
  memset(&buf, 0, sizeof(buf));

  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  ASSERT_TRUE(file->OpenForWrite(XBMC_TEMPFILEPATH(file), true));
  EXPECT_EQ((int)sizeof(str), file->Write(str, sizeof(str)));
  file->Flush();
  EXPECT_EQ(0, file->GetPosition());
  file->Close();
  ASSERT_TRUE(file->Open(XBMC_TEMPFILEPATH(file)));
  EXPECT_EQ(0, file->GetPosition());
  EXPECT_EQ((int64_t)sizeof(str), file->Seek(0, SEEK_END));
  EXPECT_EQ(0, file->Seek(0, SEEK_SET));
  EXPECT_EQ((int64_t)sizeof(str), file->GetLength());
  EXPECT_EQ(sizeof(str), file->Read(buf, sizeof(buf)));
  file->Flush();
  EXPECT_EQ((int64_t)sizeof(str), file->GetPosition());
  EXPECT_TRUE(!memcmp(str, buf, sizeof(str)));
  file->Close();
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestFile, Exists)
{
  XFILE::CFile *file;

  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  EXPECT_TRUE(XFILE::CFile::Exists(XBMC_TEMPFILEPATH(file)));
  EXPECT_FALSE(XFILE::CFile::Exists(""));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestFile, Stat)
{
  XFILE::CFile *file;
  struct __stat64 buffer;

  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  EXPECT_EQ(0, file->Stat(&buffer));
  file->Close();
  EXPECT_TRUE(buffer.st_mode | _S_IFREG);
  EXPECT_EQ(-1, XFILE::CFile::Stat("", &buffer));
  EXPECT_EQ(ENOENT, errno);
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestFile, Delete)
{
  XFILE::CFile *file;
  CStdString path;

  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  path = XBMC_TEMPFILEPATH(file);
  EXPECT_TRUE(XFILE::CFile::Exists(path));
  EXPECT_TRUE(XFILE::CFile::Delete(path));
  EXPECT_FALSE(XFILE::CFile::Exists(path));
}

TEST(TestFile, Rename)
{
  XFILE::CFile *file;
  CStdString path1, path2;

  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  path1 = XBMC_TEMPFILEPATH(file);
  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  path2 = XBMC_TEMPFILEPATH(file);
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
  EXPECT_FALSE(XFILE::CFile::Exists(path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Rename(path2, path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path1));
  EXPECT_FALSE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
}

TEST(TestFile, Cache)
{
  XFILE::CFile *file;
  CStdString path1, path2;

  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  path1 = XBMC_TEMPFILEPATH(file);
  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  path2 = XBMC_TEMPFILEPATH(file);
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
  EXPECT_FALSE(XFILE::CFile::Exists(path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Cache(path2, path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
  EXPECT_TRUE(XFILE::CFile::Delete(path2));
}

TEST(TestFile, SetHidden)
{
  XFILE::CFile *file;

  ASSERT_TRUE((file = XBMC_CREATETEMPFILE("")) != NULL);
  file->Close();
  EXPECT_TRUE(XFILE::CFile::Exists(XBMC_TEMPFILEPATH(file)));
  EXPECT_FALSE(XFILE::CFile::SetHidden(XBMC_TEMPFILEPATH(file), true));
  EXPECT_TRUE(XFILE::CFile::Exists(XBMC_TEMPFILEPATH(file)));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}
