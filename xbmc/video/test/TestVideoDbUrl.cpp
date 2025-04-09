/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"

#include <gtest/gtest.h>

TEST(TestVideoDbUrl, UrlOptions)
{
  CVideoDbUrl videoUrl;
  videoUrl.FromString("videodb://movies?foo=bar&baz=123");
  const CUrlOptions::UrlOptions& options = videoUrl.GetOptions();

  auto option = options.find("foo");
  ASSERT_NE(option, options.end());
  ASSERT_EQ(option->second.asString(), "bar");

  option = options.find("baz");
  ASSERT_NE(option, options.end());
  ASSERT_EQ(option->second.asInteger(), 123);
}

TEST(TestVideoDbUrl, AssetTypes)
{
  CVideoDbUrl videoUrl;
  videoUrl.FromString("videodb://movies/titles/123/456");
  const CUrlOptions::UrlOptions& options = videoUrl.GetOptions();

  auto option = options.find("movieid");
  ASSERT_NE(option, options.end());
  ASSERT_EQ(option->second.asInteger(), 123);

  option = options.find("assetType");
  ASSERT_NE(option, options.end());
  ASSERT_EQ(option->second.asInteger(), 456);
}
