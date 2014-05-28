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

#ifndef TEST_FILESYSTEM_DIRECTORY_H_INCLUDED
#define TEST_FILESYSTEM_DIRECTORY_H_INCLUDED
#include "filesystem/Directory.h"
#endif

#ifndef TEST_FILESYSTEM_SPECIALPROTOCOL_H_INCLUDED
#define TEST_FILESYSTEM_SPECIALPROTOCOL_H_INCLUDED
#include "filesystem/SpecialProtocol.h"
#endif

#ifndef TEST_FILEITEM_H_INCLUDED
#define TEST_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef TEST_UTILS_URIUTILS_H_INCLUDED
#define TEST_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef TEST_TEST_TESTUTILS_H_INCLUDED
#define TEST_TEST_TESTUTILS_H_INCLUDED
#include "test/TestUtils.h"
#endif


#ifndef TEST_GTEST_GTEST_H_INCLUDED
#define TEST_GTEST_GTEST_H_INCLUDED
#include "gtest/gtest.h"
#endif


TEST(TestDirectory, General)
{
  CStdString tmppath1, tmppath2, tmppath3;
  CFileItemList items;
  CFileItemPtr itemptr;
  tmppath1 = CSpecialProtocol::TranslatePath("special://temp/");
  tmppath1 = URIUtils::AddFileToFolder(tmppath1, "TestDirectory");
  tmppath2 = tmppath1;
  tmppath2 = URIUtils::AddFileToFolder(tmppath2, "subdir");
  EXPECT_TRUE(XFILE::CDirectory::Create(tmppath1));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath1));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Create(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::GetDirectory(tmppath1, items));
  XFILE::CDirectory::FilterFileDirectories(items, "");
  tmppath3 = tmppath2;
  URIUtils::AddSlashAtEnd(tmppath3);
  itemptr = items[0];
  EXPECT_STREQ(tmppath3.c_str(), itemptr->GetPath());
  EXPECT_TRUE(XFILE::CDirectory::Remove(tmppath2));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath1));
  EXPECT_TRUE(XFILE::CDirectory::Remove(tmppath1));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath1));
}
