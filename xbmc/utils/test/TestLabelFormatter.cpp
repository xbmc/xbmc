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
  item->m_bIsFolder = false;
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
  item->m_bIsFolder = false;
  item->Select(true);

  formatter.FormatLabel2(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}
