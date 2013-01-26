/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"
#ifdef HAVE_LIBARCHIVE
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"

#include <errno.h>

#include "gtest/gtest.h"

class TestRarFile : public testing::Test
{
protected:
  TestRarFile()
  {
    /* Add default settings for locale.
     * Settings here are taken from CGUISettings::Initialize()
     */
    CSettingsCategory *loc = g_guiSettings.AddCategory(7, "locale", 14090);
    g_guiSettings.AddString(loc, "locale.language",248,"english",
                            SPIN_CONTROL_TEXT);
    g_guiSettings.AddString(loc, "locale.country", 20026, "USA",
                            SPIN_CONTROL_TEXT);
    g_guiSettings.AddString(loc, "locale.charset", 14091, "DEFAULT",
                            SPIN_CONTROL_TEXT); // charset is set by the
                                                // language file

    /* Add default settings for subtitles */
    CSettingsCategory *sub = g_guiSettings.AddCategory(5, "subtitles", 287);
    g_guiSettings.AddString(sub, "subtitles.charset", 735, "DEFAULT",
                            SPIN_CONTROL_TEXT);
  }
  ~TestRarFile()
  {
    g_guiSettings.Clear();
  }
};


TEST_F(TestRarFile, Read)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));
  CStdString reffile, strrarpath, strpathinrar;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  URIUtils::CreateArchivePath(strrarpath, "rar", reffile, "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strrarpath, itemlist, "",
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

TEST_F(TestRarFile, Exists)
{
  CStdString reffile, strrarpath, strpathinrar;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  URIUtils::CreateArchivePath(strrarpath, "rar", reffile, "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strrarpath, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinrar = itemlist[0]->GetPath();

  EXPECT_TRUE(XFILE::CFile::Exists(strpathinrar));
}

TEST_F(TestRarFile, Stat)
{
  struct __stat64 buffer;
  CStdString reffile, strrarpath, strpathinrar;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  URIUtils::CreateArchivePath(strrarpath, "rar", reffile, "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strrarpath, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinrar = itemlist[0]->GetPath();

  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &buffer));
  EXPECT_TRUE(buffer.st_mode | _S_IFREG);
}

/* Test case to test for graceful handling of corrupted input.
 * NOTE: The test case is considered a "success" as long as the corrupted
 * file was successfully generated and the test case runs without a segfault.
 */
TEST_F(TestRarFile, CorruptedFile)
{
  XFILE::CFile *file;
  char buf[16];
  memset(&buf, 0, sizeof(buf));
  CStdString reffilepath, strrarpath, strpathinrar, str;
  CFileItemList itemlist;
  unsigned int size, i;
  int64_t count = 0;

  reffilepath = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.rar");
  ASSERT_TRUE((file = XBMC_CREATECORRUPTEDFILE(reffilepath, ".rar")) != NULL);
  std::cout << "Reference file generated at '" << XBMC_TEMPFILEPATH(file) << "'" << std::endl;

  URIUtils::CreateArchivePath(strrarpath, "rar", XBMC_TEMPFILEPATH(file), "");
  if (!XFILE::CDirectory::GetDirectory(strrarpath, itemlist, "",
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
    str.Format("  %08X", count);
    std::cout << str << "  ";
    count += size;
    for (i = 0; i < size; i++)
    {
      str.Format("%02X ", buf[i]);
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

TEST_F(TestRarFile, StoredRAR)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));
  CStdString reffile, strrarpath, strpathinrar;
  CFileItemList itemlist, itemlistemptydir;
  struct __stat64 stat_buffer;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/refRARstored.rar");
  URIUtils::CreateArchivePath(strrarpath, "rar", reffile, "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strrarpath, itemlist));
  itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);

  /* /reffile.txt */
  /*
   * NOTE: Use of Seek gives inconsistent behavior from when seeking through
   * an uncompressed RAR archive. See TestRarFile.Read test case.
   */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/reffile.txt", true));
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
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlink", true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  /*
   * FIXME: Reading symlinks in RARs is currently broken. It takes a long time
   * to read them and they produce erroneous results. The expected result is
   * the target paths of the symlinks.
   */
  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(0, file.GetLength());
  file.Close();

  /* /testsymlinksubdir -> testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlinksubdir", true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(0, file.GetLength());
  file.Close();

  /* /testdir/ */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/", true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);

  /* /testdir/reffile.txt */
  strpathinrar = itemlist[2]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/reffile.txt",
                                    true));
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
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testemptysubdir/",
                                    true));
  /* TODO: Should this set the itemlist to an empty list instead? */
  EXPECT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlistemptydir));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  /* /testdir/testsymlink -> testsubdir/reffile.txt */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsymlink",
                                    true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(0, file.GetLength());
  file.Close();

  /* /testdir/testsubdir/ */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsubdir/",
                                    true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);

  /* /testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar,
                                    "/testdir/testsubdir/reffile.txt", true));
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

TEST_F(TestRarFile, NormalRAR)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));
  CStdString reffile, strrarpath, strpathinrar;
  CFileItemList itemlist, itemlistemptydir;
  struct __stat64 stat_buffer;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/refRARnormal.rar");
  URIUtils::CreateArchivePath(strrarpath, "rar", reffile, "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strrarpath, itemlist));
  itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);

  /* /reffile.txt */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/reffile.txt", true));
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
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlink", true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  /*
   * FIXME: Reading symlinks in RARs is currently broken. It takes a long time
   * to read them and they produce erroneous results. The expected result is
   * the target paths of the symlinks.
   */
  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(0, file.GetLength());
  file.Close();

  /* /testsymlinksubdir -> testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testsymlinksubdir", true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(0, file.GetLength());
  file.Close();

  /* /testdir/ */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/", true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);

  /* /testdir/reffile.txt */
  strpathinrar = itemlist[2]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/reffile.txt",
                                    true));
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
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testemptysubdir/",
                                    true));
  /* TODO: Should this set the itemlist to an empty list instead? */
  EXPECT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlistemptydir));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  /* /testdir/testsymlink -> testsubdir/reffile.txt */
  strpathinrar = itemlist[3]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsymlink",
                                    true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFLNK);

  ASSERT_TRUE(file.Open(strpathinrar));
  EXPECT_EQ(0, file.GetLength());
  file.Close();

  /* /testdir/testsubdir/ */
  strpathinrar = itemlist[1]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar, "/testdir/testsubdir/",
                                    true));
  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinrar, &stat_buffer));
  EXPECT_TRUE((stat_buffer.st_mode & S_IFMT) | S_IFDIR);

  itemlist.Clear();
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strpathinrar, itemlist));
  itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);

  /* /testdir/testsubdir/reffile.txt */
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(StringUtils::EndsWith(strpathinrar,
                                    "/testdir/testsubdir/reffile.txt", true));
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

TEST_F(TestRarFile, Multivolume)
{
  XFILE::CFile file;
  char buf[64];
  memset(&buf, 0, sizeof(buf));
  CStdString reffile, strrarpath, strpathinrar;
  CFileItemList itemlist;

  reffile =
    XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.large.txt.part1.rar");
  URIUtils::CreateArchivePath(strrarpath, "rar", reffile, "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(strrarpath, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinrar = itemlist[0]->GetPath();
  ASSERT_TRUE(file.Open(strpathinrar));

  /* Do some basic reads and seeks */
  EXPECT_EQ(0, file.GetPosition());
  EXPECT_EQ(1053632, file.GetLength());
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(63, file.GetPosition());
  EXPECT_STREQ("About\n-----\nXBMC is an award-winning "
               "free and open source (GPL)",
               buf);
  memset(&buf, 0, sizeof(buf));

  EXPECT_TRUE(file.ReadString(buf, sizeof(buf)));
  EXPECT_EQ(91, file.GetPosition());
  EXPECT_STREQ(" software media player and\n\n", buf);
  memset(&buf, 0, sizeof(buf));

  EXPECT_EQ(100, file.Seek(100));
  EXPECT_EQ(100, file.GetPosition());
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(163, file.GetPosition());
  EXPECT_STREQ("ent hub for digital media. XBMC is "
               "available for multiple platf",
               buf);
  memset(&buf, 0, sizeof(buf));

  EXPECT_EQ(263, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(263, file.GetPosition());
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(326, file.GetPosition());
  EXPECT_STREQ("veloped by volunteers located around the world. "
               "More than 50\nso",
               buf);
  memset(&buf, 0, sizeof(buf));

  EXPECT_EQ(1053569, file.Seek(-(int64_t)(sizeof(buf)-1), SEEK_END));
  EXPECT_EQ(1053569, file.GetPosition());
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(1053632, file.GetPosition());
  EXPECT_STREQ("ur\ncomputer will become a fully functional "
               "multimedia jukebox.\n",
               buf);
  memset(&buf, 0, sizeof(buf));

  EXPECT_EQ(1053732, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(1053732, file.GetPosition());
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(63, file.GetPosition());
  EXPECT_STREQ("About\n-----\nXBMC is an award-winning free "
               "and open source (GPL)",
               buf);
  memset(&buf, 0, sizeof(buf));

  /*
   * Now check for expected behavior of seeking. This should be how
   * lseek() behaves.
   */
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(0, file.Seek(0, SEEK_CUR));
  EXPECT_EQ(-1, file.Seek(-100, SEEK_SET));
  EXPECT_EQ(0, file.Seek(0, SEEK_CUR));
  EXPECT_EQ(64, file.Seek(64, SEEK_SET));
  EXPECT_EQ(64, file.Seek(0, SEEK_CUR));
  EXPECT_EQ(-1, file.Seek(-128, SEEK_SET));
  EXPECT_EQ(64, file.Seek(0, SEEK_CUR));
  EXPECT_EQ(1053632, file.Seek(0, SEEK_END));
  EXPECT_EQ(1053632, file.Seek(0, SEEK_CUR));
  EXPECT_EQ(1053600, file.Seek(-32, SEEK_END));
  EXPECT_EQ(1053600, file.Seek(0, SEEK_CUR));
  EXPECT_EQ(1053632, file.Seek(32, SEEK_CUR));
  EXPECT_EQ(1053664, file.Seek(32, SEEK_CUR));
  EXPECT_EQ(1053696, file.Seek(32, SEEK_CUR));
  EXPECT_EQ(1053728, file.Seek(32, SEEK_CUR));
  EXPECT_EQ(1053664, file.Seek(32, SEEK_END));

  /*
   * Now check that reading between multivolume files works correctly.
   * Seek to 32 bytes before the end of the third multivolume RAR
   */
  EXPECT_EQ(921271, file.Seek(921271 - 1053664, SEEK_CUR));
  EXPECT_EQ(921271, file.GetPosition());
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(921334, file.GetPosition());
  EXPECT_STREQ("ltiple platforms.\nCreated in 2003 by a "
               "group of like minded pro",
               buf);
  memset(&buf, 0, sizeof(buf));

  /* Seek to 32 bytes before the end of the first multivolume RAR */
  EXPECT_EQ(307069, file.Seek(-746563, SEEK_END));
  EXPECT_EQ(307069, file.GetPosition());
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(307132, file.GetPosition());
  EXPECT_STREQ("winning free and open source (GPL) "
               "software media player and\nen",
               buf);
  memset(&buf, 0, sizeof(buf));

  /* Seek to 32 bytes before the end of the second multivolume RAR */
  EXPECT_EQ(614170, file.Seek(614170, SEEK_SET));
  EXPECT_EQ(614170, file.GetPosition());
  EXPECT_EQ(sizeof(buf)-1, file.Read(buf, sizeof(buf)-1));
  file.Flush();
  EXPECT_EQ(614233, file.GetPosition());
  EXPECT_STREQ("entertainment hub for digital media. "
               "XBMC is available for mult",
               buf);

  file.Close();
}
#endif /*HAVE_LIBARCHIVE*/
