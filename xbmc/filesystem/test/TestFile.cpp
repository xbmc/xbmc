/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "test/TestUtils.h"

#include <errno.h>
#include <string>

#include <gtest/gtest.h>

TEST(TestFile, Read)
{
  const std::string newLine = CXBMCTestUtils::Instance().getNewLineCharacters();
  const size_t size = 1616;
  const size_t lines = 25;
  size_t addPerLine = newLine.length() - 1;
  size_t realSize = size + lines * addPerLine;

  const std::string firstBuf  = "About" + newLine + "-----" + newLine + "XBMC is ";
  const std::string secondBuf = "an award-winning fre";
  const std::string thirdBuf  = "ent hub for digital ";
  const std::string fourthBuf  = "rs, XBMC is a non-pr";
  const std::string fifthBuf = "multimedia jukebox." + newLine;

  XFILE::CFile file;
  char buf[23] = {};

  size_t currentPos;
  ASSERT_TRUE(file.Open(
    XBMC_REF_FILE_PATH("/xbmc/filesystem/test/reffile.txt")));
  EXPECT_EQ(0, file.GetPosition());
  EXPECT_EQ(realSize, file.GetLength());
  EXPECT_EQ(firstBuf.length(), static_cast<size_t>(file.Read(buf, firstBuf.length())));
  file.Flush();
  currentPos = firstBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(0, memcmp(firstBuf.c_str(), buf, firstBuf.length()));
  EXPECT_EQ(secondBuf.length(), static_cast<size_t>(file.Read(buf, secondBuf.length())));
  currentPos += secondBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(0, memcmp(secondBuf.c_str(), buf, secondBuf.length()));
  currentPos = 100 + addPerLine * 3;
  EXPECT_EQ(currentPos, file.Seek(currentPos));
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(thirdBuf.length(), static_cast<size_t>(file.Read(buf, thirdBuf.length())));
  file.Flush();
  currentPos += thirdBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(0, memcmp(thirdBuf.c_str(), buf, thirdBuf.length()));
  currentPos += 100 + addPerLine * 1;
  EXPECT_EQ(currentPos, file.Seek(100 + addPerLine * 1, SEEK_CUR));
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(fourthBuf.length(), static_cast<size_t>(file.Read(buf, fourthBuf.length())));
  file.Flush();
  currentPos += fourthBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(0, memcmp(fourthBuf.c_str(), buf, fourthBuf.length()));
  currentPos = realSize - fifthBuf.length();
  EXPECT_EQ(currentPos, file.Seek(-(int64_t)fifthBuf.length(), SEEK_END));
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(fifthBuf.length(), static_cast<size_t>(file.Read(buf, fifthBuf.length())));
  file.Flush();
  currentPos += fifthBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(0, memcmp(fifthBuf.c_str(), buf, fifthBuf.length()));
  currentPos += 100;
  EXPECT_EQ(currentPos, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(currentPos, file.GetPosition());
  currentPos = 0;
  EXPECT_EQ(currentPos, file.Seek(currentPos, SEEK_SET));
  EXPECT_EQ(firstBuf.length(), static_cast<size_t>(file.Read(buf, firstBuf.length())));
  file.Flush();
  currentPos += firstBuf.length();
  EXPECT_EQ(currentPos, file.GetPosition());
  EXPECT_EQ(0, memcmp(firstBuf.c_str(), buf, firstBuf.length()));
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(-1, file.Seek(-100, SEEK_SET));
  file.Close();
}

TEST(TestFile, Write)
{
  XFILE::CFile *file;
  const char str[] = "TestFile.Write test string\n";
  char buf[30] = {};

  ASSERT_NE(nullptr, file = XBMC_CREATETEMPFILE(""));
  file->Close();
  ASSERT_TRUE(file->OpenForWrite(XBMC_TEMPFILEPATH(file), true));
  EXPECT_EQ((int)sizeof(str), file->Write(str, sizeof(str)));
  file->Flush();
  EXPECT_EQ((int64_t)sizeof(str), file->GetPosition());
  file->Close();
  ASSERT_TRUE(file->Open(XBMC_TEMPFILEPATH(file)));
  EXPECT_EQ(0, file->GetPosition());
  EXPECT_EQ((int64_t)sizeof(str), file->Seek(0, SEEK_END));
  EXPECT_EQ(0, file->Seek(0, SEEK_SET));
  EXPECT_EQ((int64_t)sizeof(str), file->GetLength());
  EXPECT_EQ(sizeof(str), static_cast<size_t>(file->Read(buf, sizeof(buf))));
  file->Flush();
  EXPECT_EQ((int64_t)sizeof(str), file->GetPosition());
  EXPECT_EQ(0, memcmp(str, buf, sizeof(str)));
  file->Close();
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestFile, Exists)
{
  XFILE::CFile *file;

  ASSERT_NE(nullptr, file = XBMC_CREATETEMPFILE(""));
  file->Close();
  EXPECT_TRUE(XFILE::CFile::Exists(XBMC_TEMPFILEPATH(file)));
  EXPECT_FALSE(XFILE::CFile::Exists(""));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestFile, Stat)
{
  XFILE::CFile *file;
  struct __stat64 buffer;

  ASSERT_NE(nullptr, file = XBMC_CREATETEMPFILE(""));
  EXPECT_EQ(0, file->Stat(&buffer));
  file->Close();
  EXPECT_NE(0U, buffer.st_mode | _S_IFREG);
  EXPECT_EQ(-1, XFILE::CFile::Stat("", &buffer));
  EXPECT_EQ(ENOENT, errno);
  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestFile, Delete)
{
  XFILE::CFile *file;
  std::string path;

  ASSERT_NE(nullptr, file = XBMC_CREATETEMPFILE(""));
  file->Close();
  path = XBMC_TEMPFILEPATH(file);
  EXPECT_TRUE(XFILE::CFile::Exists(path));
  EXPECT_TRUE(XFILE::CFile::Delete(path));
  EXPECT_FALSE(XFILE::CFile::Exists(path));
  EXPECT_FALSE(XBMC_DELETETEMPFILE(file));
}

TEST(TestFile, Rename)
{
  XFILE::CFile *file1, *file2;
  std::string path1, path2;

  ASSERT_NE(nullptr, file1 = XBMC_CREATETEMPFILE(""));
  file1->Close();
  path1 = XBMC_TEMPFILEPATH(file1);
  ASSERT_NE(nullptr, file2 = XBMC_CREATETEMPFILE(""));
  file2->Close();
  path2 = XBMC_TEMPFILEPATH(file2);
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
  EXPECT_FALSE(XFILE::CFile::Exists(path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Rename(path2, path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path1));
  EXPECT_FALSE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
  EXPECT_FALSE(XBMC_DELETETEMPFILE(file1));
  EXPECT_FALSE(XBMC_DELETETEMPFILE(file2));
}

TEST(TestFile, Copy)
{
  XFILE::CFile *file1, *file2;
  std::string path1, path2;

  ASSERT_NE(nullptr, file1 = XBMC_CREATETEMPFILE(""));
  file1->Close();
  path1 = XBMC_TEMPFILEPATH(file1);
  ASSERT_NE(nullptr, file2 = XBMC_CREATETEMPFILE(""));
  file2->Close();
  path2 = XBMC_TEMPFILEPATH(file2);
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
  EXPECT_FALSE(XFILE::CFile::Exists(path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Copy(path2, path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path1));
  EXPECT_TRUE(XFILE::CFile::Exists(path2));
  EXPECT_TRUE(XFILE::CFile::Delete(path1));
  EXPECT_TRUE(XFILE::CFile::Delete(path2));
  EXPECT_FALSE(XBMC_DELETETEMPFILE(file1));
  EXPECT_FALSE(XBMC_DELETETEMPFILE(file2));
}

TEST(TestFile, SetHidden)
{
  XFILE::CFile *file;

  ASSERT_NE(nullptr, file = XBMC_CREATETEMPFILE(""));
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
