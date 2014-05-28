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

#ifndef TEST_UTILS_MIME_H_INCLUDED
#define TEST_UTILS_MIME_H_INCLUDED
#include "utils/Mime.h"
#endif

#ifndef TEST_FILEITEM_H_INCLUDED
#define TEST_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif


#ifndef TEST_GTEST_GTEST_H_INCLUDED
#define TEST_GTEST_GTEST_H_INCLUDED
#include "gtest/gtest.h"
#endif


TEST(TestMime, GetMimeType_string)
{
  EXPECT_STREQ("video/avi",       CMime::GetMimeType("avi").c_str());
  EXPECT_STRNE("video/x-msvideo", CMime::GetMimeType("avi").c_str());
  EXPECT_STRNE("video/avi",       CMime::GetMimeType("xvid").c_str());
}

TEST(TestMime, GetMimeType_CFileItem)
{
  std::string refstr, varstr;
  CFileItem item("testfile.mp4", false);

  refstr = "video/mp4";
  varstr = CMime::GetMimeType(item);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}
