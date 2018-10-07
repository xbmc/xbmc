/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "utils/LabelFormatter.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "FileItem.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

/* Set default settings used by CLabelFormatter. */
class TestLabelFormatter : public testing::Test
{
protected:
  TestLabelFormatter()
  {
    //! @todo implement
    /* TODO
    CSettingsCategory* fl = CServiceBroker::GetSettingsComponent()->GetSettings()->AddCategory(7, "filelists", 14081);
    CServiceBroker::GetSettingsComponent()->GetSettings()->AddBool(fl, CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS, 13306, true);
    CServiceBroker::GetSettingsComponent()->GetSettings()->AddBool(fl, CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, 497, true);
    CServiceBroker::GetSettingsComponent()->GetSettings()->AddBool(fl, CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING, 13399, true);
    CServiceBroker::GetSettingsComponent()->GetSettings()->AddBool(fl, CSettings::SETTING_FILELISTS_ALLOWFILEDELETION, 14071, false);
    CServiceBroker::GetSettingsComponent()->GetSettings()->AddBool(fl, CSettings::SETTING_FILELISTS_SHOWADDSOURCEBUTTONS, 21382,  true);
    CServiceBroker::GetSettingsComponent()->GetSettings()->AddBool(fl, CSettings::SETTING_FILELISTS_SHOWHIDDEN, 21330, false);
    */
  }

  ~TestLabelFormatter() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->Unload();
  }
};

TEST_F(TestLabelFormatter, FormatLabel)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destpath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);

  formatter.FormatLabel(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}

TEST_F(TestLabelFormatter, FormatLabel2)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destpath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);

  formatter.FormatLabel2(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}
