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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "FileItem.h"
#include "test/TestUtils.h"

#include "gtest/gtest.h"

TEST(TestDirectory, General)
{
  CStdString tmppath1, tmppath2, tmppath3;
  CFileItemList items;
  CFileItemPtr itemptr;
  tmppath1 = CSpecialProtocol::TranslatePath("special://temp/");
  tmppath1 += "TestDirectory";
  tmppath2 = tmppath1;
  tmppath2 += "/subdir";
  EXPECT_TRUE(XFILE::CDirectory::Create(tmppath1));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath1));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Create(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::GetDirectory(tmppath1, items));
  XFILE::CDirectory::FilterFileDirectories(items, "");
  tmppath3 = tmppath2;
  tmppath3 += "/";
  itemptr = items[0];
  EXPECT_STREQ(tmppath3.c_str(), itemptr->GetPath());
  EXPECT_TRUE(XFILE::CDirectory::Remove(tmppath2));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath2));
  EXPECT_TRUE(XFILE::CDirectory::Exists(tmppath1));
  EXPECT_TRUE(XFILE::CDirectory::Remove(tmppath1));
  EXPECT_FALSE(XFILE::CDirectory::Exists(tmppath1));
}
