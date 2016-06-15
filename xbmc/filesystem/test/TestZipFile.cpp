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

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "test/TestUtils.h"
#include "URL.h"

#include <errno.h>

#include "gtest/gtest.h"

class TestZipFile : public testing::Test
{
protected:
  TestZipFile()
  {
    /* Add default settings for locale.
     * Settings here are taken from CGUISettings::Initialize()
     */
    //! @todo implement
    /*
    CSettingsCategory *loc = CSettings::GetInstance().AddCategory(7, "locale", 14090);
    CSettings::GetInstance().AddString(loc, CSettings::SETTING_LOCALE_LANGUAGE,248,"english",
                            SPIN_CONTROL_TEXT);
    CSettings::GetInstance().AddString(loc, CSettings::SETTING_LOCALE_COUNTRY, 20026, "USA",
                            SPIN_CONTROL_TEXT);
    CSettings::GetInstance().AddString(loc, CSettings::SETTING_LOCALE_CHARSET, 14091, "DEFAULT",
                            SPIN_CONTROL_TEXT); // charset is set by the
                                                // language file
    */
  }

  ~TestZipFile()
  {
    CSettings::GetInstance().Unload();
  }
};

TEST_F(TestZipFile, Read)
{
  XFILE::CFile file;
  char buf[20];
  memset(&buf, 0, sizeof(buf));
  std::string reffile, strpathinzip;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.zip");
  CURL zipUrl = URIUtils::CreateArchivePath("zip", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(zipUrl, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  EXPECT_GT(itemlist.Size(), 0);
  EXPECT_FALSE(itemlist[0]->GetPath().empty());
  strpathinzip = itemlist[0]->GetPath();
  ASSERT_TRUE(file.Open(strpathinzip));
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
  EXPECT_EQ(-1, file.Seek(-100, SEEK_SET));
  file.Close();
}

TEST_F(TestZipFile, Exists)
{
  std::string reffile, strpathinzip;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.zip");
  CURL zipUrl = URIUtils::CreateArchivePath("zip", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(zipUrl, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinzip = itemlist[0]->GetPath();

  EXPECT_TRUE(XFILE::CFile::Exists(strpathinzip));
}

TEST_F(TestZipFile, Stat)
{
  struct __stat64 buffer;
  std::string reffile, strpathinzip;
  CFileItemList itemlist;

  reffile = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.zip");
  CURL zipUrl = URIUtils::CreateArchivePath("zip", CURL(reffile), "");
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(zipUrl, itemlist, "",
    XFILE::DIR_FLAG_NO_FILE_DIRS));
  strpathinzip = itemlist[0]->GetPath();

  EXPECT_EQ(0, XFILE::CFile::Stat(strpathinzip, &buffer));
  EXPECT_TRUE(buffer.st_mode | _S_IFREG);
}

/* Test case to test for graceful handling of corrupted input.
 * NOTE: The test case is considered a "success" as long as the corrupted
 * file was successfully generated and the test case runs without a segfault.
 */
TEST_F(TestZipFile, CorruptedFile)
{
  XFILE::CFile *file;
  char buf[16];
  memset(&buf, 0, sizeof(buf));
  std::string reffilepath, strpathinzip, str;
  CFileItemList itemlist;
  unsigned int size, i;
  int64_t count = 0;

  reffilepath = XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.zip");
  ASSERT_TRUE((file = XBMC_CREATECORRUPTEDFILE(reffilepath, ".zip")) != NULL);
  std::cout << "Reference file generated at '" << XBMC_TEMPFILEPATH(file) << "'" << std::endl;

  CURL zipUrl = URIUtils::CreateArchivePath("zip", CURL(reffilepath), "");
  if (!XFILE::CDirectory::GetDirectory(zipUrl, itemlist, "",
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
  strpathinzip = itemlist[0]->GetPath();

  if (!file->Open(strpathinzip))
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
