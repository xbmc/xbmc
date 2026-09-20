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

TEST(TestMime, GetMimeType_imageExtensions)
{
  // CTextureCacheJob takes the type of a remote image from its extension rather than asking the
  // source for it, so these have to be what the source would have answered - an image decoder
  // addon is chosen by an exact match on the type, and "image/jpg" is not a type anything uses
  EXPECT_EQ("image/jpeg", CMime::GetMimeType("jpg"));
  EXPECT_EQ("image/jpeg", CMime::GetMimeType("jpeg"));
  EXPECT_EQ("image/tiff", CMime::GetMimeType("tif"));
  EXPECT_EQ("image/tiff", CMime::GetMimeType("tiff"));
  EXPECT_EQ("image/png", CMime::GetMimeType("png"));
  EXPECT_EQ("image/webp", CMime::GetMimeType("webp"));
  EXPECT_EQ("image/heic", CMime::GetMimeType("heic"));

  // Kodi's own thumbnail extensions say nothing about the contents, so the source is still asked
  EXPECT_EQ("", CMime::GetMimeType("tbn"));
  EXPECT_EQ("", CMime::GetMimeType("dds"));

  // An extension whose type isn't an image, so an image served under one has to be asked about
  // rather than taken at its name
  EXPECT_EQ("text/asp", CMime::GetMimeType("asp"));
  EXPECT_EQ("text/html", CMime::GetMimeType("html"));
}
