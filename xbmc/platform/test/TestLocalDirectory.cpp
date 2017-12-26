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

#include "platform/LocalDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "FileItem.h"
#include "utils/URIUtils.h"
#include "test/TestUtils.h"

#include "gtest/gtest.h"

using namespace KODI::PLATFORM;

TEST(TestLocalDirectory, General)
{
  std::string tmppath1, tmppath2, tmppath3;
  std::vector<std::string> items;
  tmppath1 = CLocalDirectory::CreateSystemTempDirectory();
  tmppath1 = URIUtils::AddFileToFolder(tmppath1, "TestDirectory");
  tmppath2 = tmppath1;
  tmppath2 = URIUtils::AddFileToFolder(tmppath2, "subdir");
  EXPECT_TRUE(CLocalDirectory::Create(tmppath1));
  EXPECT_TRUE(CLocalDirectory::Exists(tmppath1));
  EXPECT_FALSE(CLocalDirectory::Exists(tmppath2));
  EXPECT_TRUE(CLocalDirectory::Create(tmppath2));
  EXPECT_TRUE(CLocalDirectory::Exists(tmppath2));
  EXPECT_TRUE(CLocalDirectory::GetDirectory(tmppath1, items));
  tmppath3 = tmppath2;
  URIUtils::AddSlashAtEnd(tmppath3);
  //EXPECT_STREQ(tmppath3.c_str(), itemptr->GetPath().c_str());
  EXPECT_TRUE(CLocalDirectory::Remove(tmppath2));
  EXPECT_FALSE(CLocalDirectory::Exists(tmppath2));
  EXPECT_TRUE(CLocalDirectory::Exists(tmppath1));
  EXPECT_TRUE(CLocalDirectory::Remove(tmppath1));
  EXPECT_FALSE(CLocalDirectory::Exists(tmppath1));
}

TEST(TestLocalDirectory, CreateRecursive)
{
  auto path1 = URIUtils::AddFileToFolder(
    CLocalDirectory::CreateSystemTempDirectory(),
    "level1");
  auto path2 = URIUtils::AddFileToFolder(path1,
    "level2",
    "level3");

  EXPECT_TRUE(CLocalDirectory::Create(path2));
  EXPECT_TRUE(CLocalDirectory::RemoveRecursive(path1));
}
