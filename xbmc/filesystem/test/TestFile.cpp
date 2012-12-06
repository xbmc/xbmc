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

#include "filesystem/File.h"
#include "test/TestUtils.h"

#include <errno.h>

#include "gtest/gtest.h"

TEST(TestFile, Read)
{
  const std::string newLine = CXBMCTestUtils::Instance().getNewLineCharacters();
  const int size = 1616;
  const int lines = 25;
  int addPerLine = newLine.length() - 1;
  int realSize = size + lines * addPerLine;

  const std::string firstBuf  = "About" + newLine + "-----" + newLine + "XBMC is ";
  const std::string secondBuf = "an award-winning fre";
  const std::string thirdBuf  = "ent hub for digital ";
  const std::string fourthBuf  = "rs, XBMC is a non-pr";
  const std::string fifthBuf = "multimedia jukebox." + newLine;

  XFILE::CFile file;
  char buf[23];
  memset(&buf, 0, sizeof(buf));

  int currentPos;
  ASSERT_TRUE(file.Open(
    XBMC_REF_FILE_PATH("/xbmc/filesystem/test/reffile.txt")));
  EXPECT_EQ(0, file.GetPosition());
  EXPECT_EQ(realSize, file.GetLength());
  EXPECT_EQ(firstBuf.length(), file.Read(buf, firstBuf.length()));
  file.Flush();
  currentPos = firstBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_TRUE(memcmp(firstBuf.c_str(), buf, firstBuf.length()) == 0);
  EXPECT_TRUE(file.Read(buf, secondBuf.length()));
  currentPos += secondBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_TRUE(memcmp(secondBuf.c_str(), buf, secondBuf.length()) == 0);
  currentPos = 100 + addPerLine * 3;
  EXPECT_EQ(currentPos, file.Seek(currentPos));
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(thirdBuf.length(), file.Read(buf, thirdBuf.length()));
  file.Flush();
  currentPos += thirdBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_TRUE(memcmp(thirdBuf.c_str(), buf, thirdBuf.length()) == 0);
  currentPos += 100 + addPerLine * 1;
  EXPECT_EQ(currentPos, file.Seek(100 + addPerLine * 1, SEEK_CUR));
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(fourthBuf.length(), file.Read(buf, fourthBuf.length()));
  file.Flush();
  currentPos += fourthBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_TRUE(memcmp(fourthBuf.c_str(), buf, fourthBuf.length()) == 0);
  currentPos = realSize - fifthBuf.length();
  EXPECT_EQ(currentPos, file.Seek(-(int64_t)fifthBuf.length(), SEEK_END));
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(fifthBuf.length(), file.Read(buf, fifthBuf.length()));
  file.Flush();
  currentPos += fifthBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_TRUE(memcmp(fifthBuf.c_str(), buf, fifthBuf.length()) == 0);
  currentPos += 100;
  EXPECT_EQ(currentPos, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(currentPos, file.GetPosition());
  currentPos = 0;
  EXPECT_EQ(currentPos, file.Seek(currentPos, SEEK_SET));
  EXPECT_EQ(firstBuf.length(), file.Read(buf, firstBuf.length()));
  file.Flush();
  currentPos += firstBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_TRUE(memcmp(firstBuf.c_str(), buf, firstBuf.length()) == 0);
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
  bool result = XFILE::CFile::SetHidden(XBMC_TEMPFILEPATH(file), true);
#ifdef TARGET_WINDOWS
  EXPECT_TRUE(result);
#else
  EXPECT_FALSE(result);
#endif
  EXPECT_TRUE(XFILE::CFile::Exists(XBMC_TEMPFILEPATH(file)));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}
