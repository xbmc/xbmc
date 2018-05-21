/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "utils/Mime.h"
#include "FileItem.h"

#include "gtest/gtest.h"

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
