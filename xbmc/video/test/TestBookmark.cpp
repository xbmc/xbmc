/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/Bookmark.h"

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
CBookmark BuildBookmark(CBookmark::EType type, double timeInSeconds)
{
  CBookmark b;
  b.type = type;
  b.timeInSeconds = timeInSeconds;
  return b;
}
} // namespace

TEST(TestBookmark, BookmarksToPositions)
{
  // Sort by timestamps, only standard bookmarks.
  VECBOOKMARKS bookmarks{
      BuildBookmark(CBookmark::STANDARD, 200),
      BuildBookmark(CBookmark::STANDARD, 100),
      BuildBookmark(CBookmark::EPISODE, 300),
      BuildBookmark(CBookmark::RESUME, 400),
  };

  auto positions = CBookmark::BookmarksToPositions(bookmarks);

  ASSERT_EQ(2, positions.size());
  ASSERT_EQ(positions[0], 100s);
  ASSERT_EQ(positions[1], 200s);
}

TEST(TestBookmark, AddToPositions)
{
  // Standard bookmark, time converted from seconds.
  CBookmark b = BuildBookmark(CBookmark::STANDARD, 123);

  std::vector<std::chrono::milliseconds> positions{100s, 200s};

  CBookmark::AddToPositions(b, positions);

  ASSERT_EQ(3, positions.size());
  ASSERT_EQ(positions[0], 100s);
  ASSERT_EQ(positions[1], 123s);
  ASSERT_EQ(positions[2], 200s);

  // Episode bookmark ignored
  b = BuildBookmark(CBookmark::EPISODE, 234);

  CBookmark::AddToPositions(b, positions);

  ASSERT_EQ(3, positions.size());
  ASSERT_EQ(positions[0], 100s);
  ASSERT_EQ(positions[1], 123s);
  ASSERT_EQ(positions[2], 200s);
}
