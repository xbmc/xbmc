/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "system.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"

#include <errno.h>

#include "gtest/gtest.h"

#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif

TEST(TestRarFile, Read)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));
  std::string reffile, strpathinrar;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  CURL rarUrl = URIUtils::CreateArchivePath("rar", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(rarUrl, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(file.Open(strpathinrar));
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
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
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

TEST(TestRarFile, Exists)
{
  std::string reffile, strpathinrar;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  CURL rarUrl = URIUtils::CreateArchivePath("rar", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(rarUrl, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinrar = itemlist[0]->GetPath();

  EXPECT_TRUE(XFILE::CFile::Exists(strpathinrar));
}

TEST(TestRarFile, Stat)
{
  struct __stat64 buffer;
  std::string reffile, strpathinrar;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  CURL rarUrl = URIUtils::CreateArchivePath("rar", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(rarUrl, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinrar = itemlist[0]->GetPath();

  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &buffer));
  EXPECT_TRUE(buffer.st_mode | _S_IFREG);
}

/* Test case to test for graceful handling of corrupted input.
 * NOTE: The test case is considered a "success" as long as the corrupted
 * file was successfully generated and the test case runs without a segfault.
 */
TEST(TestRarFile, CorruptedFile)
{
  XFILE::CFile *file;
  char buf[16];
  memset(&buf, 0, sizeof(buf));
  std::string reffilepath, strpathinrar, str;
  CFileItemList itemlist;
  unsigned int size, i;
  int64_t count = 0;

  reffilepath = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  ASSERT_TRUE((file = XBMC_CREATECORRUPTEDFILE(reffilepath, ".rar")) != NULL);
  std::cout << "Reference file generated at '" << XBMC_TEMPFILEPATH(file) << "'" << std::endl;

  CURL rarUrl = URIUtils::CreateArchivePath("rar", CURL(XBMC_TEMPFILEPATH(file)), "");
  if (!XFILE::CDirectory::GetDirectory(rarUrl, itemlist, "",
                                       XFILE::DIR_FLAG_NO_FILE_DIRS))
  {
    XBMC_DELETETEMPFILE(file);
    SUCCEED();
    return;
  }
  if (itemlist.IsEmpty())
  {
    XBMC_DELETETEMPFILE(file);
    SUCCEED();
    return;
  }
  strpathinrar = itemlist[0]->GetPath();

  if (!file->Open(strpathinrar))
  {
    XBMC_DELETETEMPFILE(file);
    SUCCEED();
    return;
  }
  std::cout << "file->GetLength(): " <<
    testing::PrintToString(file->GetLength()) << std::endl;
  std::cout << "file->Seek(file->GetLength() / 2, SEEK_CUR) return value: " <<
    testing::PrintToString(file->Seek(file->GetLength() / 2, SEEK_CUR)) << std::endl;
  std::cout << "file->Seek(0, SEEK_END) return value: " <<
    testing::PrintToString(file->Seek(0, SEEK_END)) << std::endl;
  std::cout << "file->Seek(0, SEEK_SET) return value: " <<
    testing::PrintToString(file->Seek(0, SEEK_SET)) << std::endl;
  std::cout << "File contents:" << std::endl;
  while ((size = file->Read(buf, sizeof(buf))) > 0)
  {
    str = StringUtils::Format("  %08X", count);
    std::cout << str << "  ";
    count += size;
    for (i = 0; i < size; i++)
    {
      str = StringUtils::Format("%02X ", buf[i]);
      std::cout << str;
    }
    while (i++ < sizeof(buf))
      std::cout << "   ";
    std::cout << " [";
    for (i = 0; i < size; i++)
    {
      if (buf[i] >= ' ' && buf[i] <= '~')
        std::cout << buf[i];
      else
        std::cout << ".";
    }
    std::cout << "]" << std::endl;
  }
  file->Close();
  XBMC_DELETETEMPFILE(file);
}

TEST(TestRarFile, StoredRAR)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));
  std::string reffile, strpathinrar;
  CFileItemList itemlist, itemlistemptydir;
  struct __stat64 stat_buffer;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/refRARstored.rar");
  CURL rarUrl = URIUtils::CreateArchivePath("rar", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(rarUrl, itemlist));
  itemlist.Sort(SortByPath, SortOrderAscending);

  /* /reffile.txt */
  /*
   * NOTE: Use of Seek gives inconsistent behavior from when seeking through
   * an uncompressed RAR archive. See TestRarFile.Read test case.
   */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/reffile.txt"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFREG);

  ASSERT_TRUE(file.Open(strpathinrar));
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
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
  EXPECT_EQ(1596, file.GetPosition());
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_TRUE(!memcmp("multimedia jukebox.\n", buf, sizeof(buf) - 1));
  EXPECT_EQ(-1, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(20, file.GetPosition());
  EXPECT_TRUE(!memcmp("About\n-----\nXBMC is ", buf, sizeof(buf) - 1));
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(-100, file.Seek(-100, SEEK_SET));
  file.Close();

  /* /testsymlink -> testdir/reffile.txt */
  strpathinrar = itemlist[2]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlink"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  /*
   * FIXME: Reading symlinks in RARs is currently broken. It takes a long time
   * to read them and they produce erroneous results. The expected result is
   * the target paths of the symlinks.
   */
  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(19, file.GetLength());
  file.Close();

  /* /testsymlinksubdir -> testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlinksubdir"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(30, file.GetLength());
  file.Close();

  /* /testdir/ */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SortByPath, SortOrderAscending);

  /* /testdir/reffile.txt */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/reffile.txt"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFREG);

  ASSERT_TRUE(file.Open(strpathinrar));
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
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
  EXPECT_EQ(1596, file.GetPosition());
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_TRUE(!memcmp("multimedia jukebox.\n", buf, sizeof(buf) - 1));
  EXPECT_EQ(-1, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(20, file.GetPosition());
  EXPECT_TRUE(!memcmp("About\n-----\nXBMC is ", buf, sizeof(buf) - 1));
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(-100, file.Seek(-100, SEEK_SET));
  file.Close();

  /* /testdir/testemptysubdir */
  strpathinrar = itemlist[2]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testemptysubdir"));
  /* TODO: Should this set the itemlist to an empty list instead? */
  EXPECT_FALSE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlistemptydir));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  /* FIXME: This directory appears a second time as a file */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsubdir"));

  /* /testdir/testsymlink -> testsubdir/reffile.txt */
  strpathinrar = itemlist[4]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsymlink"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(22, file.GetLength());
  file.Close();

  /* /testdir/testsubdir/ */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsubdir/"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SortByPath, SortOrderAscending);

  /* /testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar,
                                    "/testdir/testsubdir/reffile.txt"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFREG);

  ASSERT_TRUE(file.Open(strpathinrar));
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
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
  EXPECT_EQ(1596, file.GetPosition());
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_TRUE(!memcmp("multimedia jukebox.\n", buf, sizeof(buf) - 1));
  EXPECT_EQ(-1, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(sizeof(buf), file.Read(buf, sizeof(buf)));
  file.Flush();
  EXPECT_EQ(20, file.GetPosition());
  EXPECT_TRUE(!memcmp("About\n-----\nXBMC is ", buf, sizeof(buf) - 1));
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(-100, file.Seek(-100, SEEK_SET));
  file.Close();
}

TEST(TestRarFile, NormalRAR)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));
  std::string reffile, strpathinrar;
  CFileItemList itemlist, itemlistemptydir;
  struct __stat64 stat_buffer;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/refRARnormal.rar");
  CURL rarUrl = URIUtils::CreateArchivePath("rar", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(rarUrl, itemlist));
  itemlist.Sort(SortByPath, SortOrderAscending);

  /* /reffile.txt */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/reffile.txt"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFREG);

  ASSERT_TRUE(file.Open(strpathinrar));
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
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
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

  /* /testsymlink -> testdir/reffile.txt */
  strpathinrar = itemlist[2]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlink"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  /*
   * FIXME: Reading symlinks in RARs is currently broken. It takes a long time
   * to read them and they produce erroneous results. The expected result is
   * the target paths of the symlinks.
   */
  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(19, file.GetLength());
  file.Close();

  /* /testsymlinksubdir -> testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlinksubdir"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(30, file.GetLength());
  file.Close();

  /* /testdir/ */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SortByPath, SortOrderAscending);

  /* /testdir/reffile.txt */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/reffile.txt"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFREG);

  ASSERT_TRUE(file.Open(strpathinrar));
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
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
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

  /* /testdir/testemptysubdir */
  strpathinrar = itemlist[2]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testemptysubdir"));
  /* TODO: Should this set the itemlist to an empty list instead? */
  EXPECT_FALSE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlistemptydir));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  /* FIXME: This directory appears a second time as a file */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsubdir"));

  /* /testdir/testsymlink -> testsubdir/reffile.txt */
  strpathinrar = itemlist[4]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsymlink"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(22, file.GetLength());
  file.Close();

  /* /testdir/testsubdir/ */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsubdir/"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SortByPath, SortOrderAscending);

  /* /testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar,
                                    "/testdir/testsubdir/reffile.txt"));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFREG);

  ASSERT_TRUE(file.Open(strpathinrar));
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
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
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
#endif /*HAS_FILESYSTEM_RAR*/
