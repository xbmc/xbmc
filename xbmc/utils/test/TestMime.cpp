/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "utils/Mime.h"

#include <gtest/gtest.h>

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
