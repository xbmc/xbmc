/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "playlists/SmartFeed.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using KODI::PLAYLIST::CSmartFeed;

namespace
{
std::shared_ptr<CSmartFeed> Feed(int total, std::vector<std::vector<int>>& fetched)
{
  return std::make_shared<CSmartFeed>(
      total,
      [&fetched](const std::vector<int>& slice)
      {
        fetched.push_back(slice);
        std::vector<std::shared_ptr<CFileItem>> items;
        for (const int entry : slice)
          items.push_back(std::make_shared<CFileItem>(std::to_string(entry), false));
        return items;
      });
}

std::vector<int> Entries(const std::vector<std::shared_ptr<CFileItem>>& items)
{
  std::vector<int> entries;
  for (const auto& item : items)
    entries.push_back(std::stoi(item->GetPath()));
  return entries;
}

std::vector<int> Sorted(std::vector<int> entries)
{
  std::sort(entries.begin(), entries.end());
  return entries;
}
} // namespace

TEST(TestSmartFeed, DealsEveryEntryOnceThenRunsOut)
{
  std::vector<std::vector<int>> fetched;
  const auto feed = Feed(5, fetched);

  std::vector<int> dealt = Entries(feed->Take(3));
  EXPECT_EQ(3u, dealt.size());
  EXPECT_EQ(2, feed->GetLeft());
  const std::vector<int> rest = Entries(feed->Take(3));
  dealt.insert(dealt.end(), rest.begin(), rest.end());

  EXPECT_EQ((std::vector<int>{0, 1, 2, 3, 4}), Sorted(dealt));
  EXPECT_EQ(5, feed->GetTotal());
  EXPECT_EQ(0, feed->GetLeft());
  EXPECT_TRUE(feed->Take(3).empty());
  EXPECT_EQ(2u, fetched.size()) << "a feed that has run out fetches nothing";
}

TEST(TestSmartFeed, TheCreatorFetchesWhatIsTakenInDealingOrder)
{
  std::vector<std::vector<int>> fetched;
  const auto feed = Feed(4, fetched);

  const std::vector<int> dealt = Entries(feed->Take(4));

  ASSERT_EQ(1u, fetched.size());
  EXPECT_EQ(fetched[0], dealt);
}

TEST(TestSmartFeed, RestartDealsEverythingAgain)
{
  std::vector<std::vector<int>> fetched;
  const auto feed = Feed(4, fetched);
  feed->Take(4);

  feed->Restart();

  EXPECT_EQ(4, feed->GetLeft());
  EXPECT_EQ((std::vector<int>{0, 1, 2, 3}), Sorted(Entries(feed->Take(4))));
}
