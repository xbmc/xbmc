/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/LabelFormatter.h"

#include <gtest/gtest.h>

/* Set default settings used by CLabelFormatter. */
class TestLabelFormatter : public testing::Test
{
protected:
  TestLabelFormatter() = default;

  ~TestLabelFormatter() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->Unload();
  }
};

TEST_F(TestLabelFormatter, FormatLabel)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->SetFolder(false);
  item->Select(true);

  formatter.FormatLabel(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}

TEST_F(TestLabelFormatter, FormatLabel2)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->SetFolder(false);
  item->Select(true);

  formatter.FormatLabel2(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}

class TestLabelFormatterHiddenExtensions : public testing::Test
{
protected:
  void SetUp() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_showExtensions = settings->GetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS);
    settings->SetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, false);
  }

  void TearDown() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, m_showExtensions);
  }

  static std::string LabelFor(const std::string& path, const std::string& label)
  {
    CFileItem item;
    item.SetPath(path);
    item.SetLabel(label);
    item.SetFolder(false);

    CLabelFormatter formatter("%L", "");
    formatter.FormatLabel(&item);
    return item.GetLabel();
  }

private:
  bool m_showExtensions{true};
};

TEST_F(TestLabelFormatterHiddenExtensions, HidesTheExtensionOfAnEscapedName)
{
  EXPECT_EQ("file_name", LabelFor("davs://server/files/file_name.mkv", "file_name.mkv"));
  EXPECT_EQ("file name", LabelFor("davs://server/files/file%20name.mkv", "file name.mkv"));
}

TEST_F(TestLabelFormatterHiddenExtensions, KeepsAPlusInTheName)
{
  EXPECT_EQ("C++ Collection",
            LabelFor("smb://server/share/C++ Collection.mkv", "C++ Collection.mkv"));
}
