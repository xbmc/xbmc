/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/ZipFile.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <errno.h>

#include <gtest/gtest.h>

class TestZipFile : public testing::Test
{
protected:
  TestZipFile() = default;

  ~TestZipFile() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->Unload();
  }
};

TEST_F(TestZipFile, Read)
{
  XFILE::CFile file;
  char buf[20] = {};
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
  EXPECT_EQ(sizeof(buf), static_cast<size_t>(file.Read(buf, sizeof(buf))));
  file.Flush();
  EXPECT_EQ(20, file.GetPosition());
  EXPECT_TRUE(!memcmp("About\n-----\nXBMC is ", buf, sizeof(buf) - 1));
  EXPECT_TRUE(file.ReadString(buf, sizeof(buf)));
  EXPECT_EQ(39, file.GetPosition());
  EXPECT_STREQ("an award-winning fr", buf);
  EXPECT_EQ(100, file.Seek(100));
  EXPECT_EQ(100, file.GetPosition());
  EXPECT_EQ(sizeof(buf), static_cast<size_t>(file.Read(buf, sizeof(buf))));
  file.Flush();
  EXPECT_EQ(120, file.GetPosition());
  EXPECT_TRUE(!memcmp("ent hub for digital ", buf, sizeof(buf) - 1));
  EXPECT_EQ(220, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(220, file.GetPosition());
  EXPECT_EQ(sizeof(buf), static_cast<size_t>(file.Read(buf, sizeof(buf))));
  file.Flush();
  EXPECT_EQ(240, file.GetPosition());
  EXPECT_TRUE(!memcmp("rs, XBMC is a non-pr", buf, sizeof(buf) - 1));
  EXPECT_EQ(1596, file.Seek(-(int64_t)sizeof(buf), SEEK_END));
  EXPECT_EQ(1596, file.GetPosition());
  EXPECT_EQ(sizeof(buf), static_cast<size_t>(file.Read(buf, sizeof(buf))));
  file.Flush();
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_TRUE(!memcmp("multimedia jukebox.\n", buf, sizeof(buf) - 1));
  EXPECT_EQ(-1, file.Seek(100, SEEK_CUR));
  EXPECT_EQ(1616, file.GetPosition());
  EXPECT_EQ(0, file.Seek(0, SEEK_SET));
  EXPECT_EQ(sizeof(buf), static_cast<size_t>(file.Read(buf, sizeof(buf))));
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
  char buf[16] = {};
  std::string reffilepath, strpathinzip, str;
  CFileItemList itemlist;
  ssize_t size, i;
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
    str = StringUtils::Format("  {:08X}", count);
    std::cout << str << "  ";
    count += size;
    for (i = 0; i < size; i++)
    {
      str = StringUtils::Format("{:02X} ", buf[i]);
      std::cout << str;
    }
    while (i++ < static_cast<ssize_t> (sizeof(buf)))
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

TEST_F(TestZipFile, ExtendedLocalHeader)
{
  XFILE::CFile file;
  ssize_t readlen;
  char zipdata[20000]; // size of zip file is 15352 Bytes

  ASSERT_TRUE(file.Open(XBMC_REF_FILE_PATH("xbmc/filesystem/test/extendedlocalheader.zip")));
  readlen = file.Read(zipdata, sizeof(zipdata));
  EXPECT_TRUE(readlen);

  XFILE::CZipFile zipfile;
  std::string strBuffer;

  int iSize = zipfile.UnpackFromMemory(strBuffer, std::string(zipdata, readlen), false);
  EXPECT_EQ(152774, iSize); // sum of uncompressed size of all files in zip
  EXPECT_TRUE(strBuffer.substr(0, 6) == "<Data>");
  file.Close();
}
