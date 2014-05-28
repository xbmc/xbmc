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

#ifndef TEST_UTILS_FILEUTILS_H_INCLUDED
#define TEST_UTILS_FILEUTILS_H_INCLUDED
#include "utils/FileUtils.h"
#endif

#ifndef TEST_FILESYSTEM_FILE_H_INCLUDED
#define TEST_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif


#ifndef TEST_TEST_TESTUTILS_H_INCLUDED
#define TEST_TEST_TESTUTILS_H_INCLUDED
#include "test/TestUtils.h"
#endif


#ifndef TEST_GTEST_GTEST_H_INCLUDED
#define TEST_GTEST_GTEST_H_INCLUDED
#include "gtest/gtest.h"
#endif


TEST(TestFileUtils, DeleteItem_CFileItemPtr)
{
  XFILE::CFile *tmpfile;
  CStdString tmpfilepath;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);

  EXPECT_TRUE(CFileUtils::DeleteItem(item));
}

TEST(TestFileUtils, DeleteItem_CStdString)
{
  XFILE::CFile *tmpfile;

  ASSERT_TRUE((tmpfile = XBMC_CREATETEMPFILE("")));
  EXPECT_TRUE(CFileUtils::DeleteItem(XBMC_TEMPFILEPATH(tmpfile)));
}

/* Executing RenameFile() requires input from the user */
// static bool RenameFile(const CStdString &strFile);
