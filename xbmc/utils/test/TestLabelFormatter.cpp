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

#include "utils/LabelFormatter.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "FileItem.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

/* Set default settings used by CLabelFormatter. */
class TestLabelFormatter : public testing::Test
{
protected:
  TestLabelFormatter()
  {
    CSettingsCategory* fl = g_guiSettings.AddCategory(7, "filelists", 14081);
    g_guiSettings.AddBool(fl, "filelists.showparentdiritems", 13306, true);
    g_guiSettings.AddBool(fl, "filelists.showextensions", 497, true);
    g_guiSettings.AddBool(fl, "filelists.ignorethewhensorting", 13399, true);
    g_guiSettings.AddBool(fl, "filelists.allowfiledeletion", 14071, false);
    g_guiSettings.AddBool(fl, "filelists.showaddsourcebuttons", 21382,  true);
    g_guiSettings.AddBool(fl, "filelists.showhidden", 21330, false);
  }

  ~TestLabelFormatter()
  {
    g_guiSettings.Clear();
  }
};

TEST_F(TestLabelFormatter, FormatLabel)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath, destpath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
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
  CStdString tmpfilepath, destpath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);

  formatter.FormatLabel2(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}
