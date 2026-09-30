/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureCacheJob.h"

#include <gtest/gtest.h>

TEST(TestTextureCacheJob, MayBeAnImage)
{
  EXPECT_TRUE(CTextureCacheJob::MayBeAnImage("image/jpeg"));
  EXPECT_TRUE(CTextureCacheJob::MayBeAnImage("image/png"));
  EXPECT_TRUE(CTextureCacheJob::MayBeAnImage("IMAGE/JPEG"));

  // What a source says when it doesn't know, which an image still has to be tried against
  EXPECT_TRUE(CTextureCacheJob::MayBeAnImage("application/octet-stream"));

  EXPECT_FALSE(CTextureCacheJob::MayBeAnImage("text/html"));
  EXPECT_FALSE(CTextureCacheJob::MayBeAnImage("text/asp"));
  EXPECT_FALSE(CTextureCacheJob::MayBeAnImage("video/mp4"));

  // Nothing has said what it is, so there is nothing here to rule it in either
  EXPECT_FALSE(CTextureCacheJob::MayBeAnImage(""));
}

TEST(TestTextureCacheJob, GetImageHashFromStatSkipsAddonsAndPlugins)
{
  CTextureCacheJob job("addons://x", {}, "");
  EXPECT_EQ("", job.GetImageHashFromStat("addons://x"));
  EXPECT_EQ("", job.GetImageHashFromStat("plugin://y"));
  EXPECT_EQ("", job.GetImageHashFromStat("upnp://z"));
}

TEST(TestTextureCacheJob, GetImageHashFromStatReturnsEmptyOnMissingFile)
{
  CTextureCacheJob job("missing", {}, "");
  // A path that cannot be stat'ed must produce an empty hash, not crash
  EXPECT_EQ("", job.GetImageHashFromStat("/this/path/does/not/exist/nope.png"));
}

TEST(TestTextureCacheJob, FormatImageHashEmptyWhenZero)
{
  CTextureCacheJob job("any", {}, "");
  EXPECT_EQ("", job.FormatImageHash(0, 0));
  // Sanity: non-zero inputs do produce a non-empty hash
  EXPECT_NE("", job.FormatImageHash(1700000000, 12345));
}
