/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"

#include "gtest/gtest.h"

class TestFileFactory : public testing::Test
{
protected:
  TestFileFactory()
  {
    if (CServiceBroker::GetSettings().Initialize())
    {
      std::vector<std::string> advancedsettings =
        CXBMCTestUtils::Instance().getAdvancedSettingsFiles();
      std::vector<std::string> guisettings =
        CXBMCTestUtils::Instance().getGUISettingsFiles();

      for (const auto& it : guisettings)
        CServiceBroker::GetSettings().Load(it);

      for (const auto& it : advancedsettings)
        g_advancedSettings.ParseSettingsFile(it);

      CServiceBroker::GetSettings().SetLoaded();
    }
  }

  ~TestFileFactory() override
  {
    g_advancedSettings.Clear();
    CServiceBroker::GetSettings().Unload();
    CServiceBroker::GetSettings().Uninitialize();
  }
};

/* The tests for XFILE::CFileFactory are tested indirectly through
 * XFILE::CFile. Since most parts of the VFS require some form of
 * network connection, the settings and VFS URLs must be given as
 * arguments in the main testsuite program.
 */
TEST_F(TestFileFactory, Read)
{
  XFILE::CFile file;
  std::string str;
  unsigned int size, i;
  unsigned char buf[16];
  int64_t count = 0;

  std::vector<std::string> urls =
    CXBMCTestUtils::Instance().getTestFileFactoryReadUrls();

  for (const auto& url : urls)
  {
    std::cout << "Testing URL: " << url << std::endl;
    ASSERT_TRUE(file.Open(url));
    std::cout << "file.GetLength(): " <<
      testing::PrintToString(file.GetLength()) << std::endl;
    std::cout << "file.Seek(file.GetLength() / 2, SEEK_CUR) return value: " <<
      testing::PrintToString(file.Seek(file.GetLength() / 2, SEEK_CUR)) << std::endl;
    std::cout << "file.Seek(0, SEEK_END) return value: " <<
      testing::PrintToString(file.Seek(0, SEEK_END)) << std::endl;
    std::cout << "file.Seek(0, SEEK_SET) return value: " <<
      testing::PrintToString(file.Seek(0, SEEK_SET)) << std::endl;
    std::cout << "File contents:" << std::endl;
    while ((size = file.Read(buf, sizeof(buf))) > 0)
    {
      str = StringUtils::Format("  %08llX", count);
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
    file.Close();
  }
}

TEST_F(TestFileFactory, Write)
{
  XFILE::CFile file, inputfile;
  std::string str;
  unsigned int size, i;
  unsigned char buf[16];
  int64_t count = 0;

  str = CXBMCTestUtils::Instance().getTestFileFactoryWriteInputFile();
  ASSERT_TRUE(inputfile.Open(str));

  std::vector<std::string> urls =
    CXBMCTestUtils::Instance().getTestFileFactoryWriteUrls();

  for (const auto& url : urls)
  {
    std::cout << "Testing URL: " << url << std::endl;
    std::cout << "Writing...";
    ASSERT_TRUE(file.OpenForWrite(url, true));
    while ((size = inputfile.Read(buf, sizeof(buf))) > 0)
    {
      EXPECT_GE(file.Write(buf, size), 0);
    }
    file.Close();
    std::cout << "done." << std::endl;
    std::cout << "Reading..." << std::endl;
    ASSERT_TRUE(file.Open(url));
    EXPECT_EQ(inputfile.GetLength(), file.GetLength());
    std::cout << "file.Seek(file.GetLength() / 2, SEEK_CUR) return value: " <<
      testing::PrintToString(file.Seek(file.GetLength() / 2, SEEK_CUR)) << std::endl;
    std::cout << "file.Seek(0, SEEK_END) return value: " <<
      testing::PrintToString(file.Seek(0, SEEK_END)) << std::endl;
    std::cout << "file.Seek(0, SEEK_SET) return value: " <<
      testing::PrintToString(file.Seek(0, SEEK_SET)) << std::endl;
    std::cout << "File contents:\n";
    while ((size = file.Read(buf, sizeof(buf))) > 0)
    {
      str = StringUtils::Format("  %08llX", count);
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
    file.Close();
  }
  inputfile.Close();
}
