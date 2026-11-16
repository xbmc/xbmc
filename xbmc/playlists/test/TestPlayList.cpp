/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "playlists/PlayListShuffle.h"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KODI::PLAYLIST;

TEST(TestPlayListRandomShuffle, TheCurrentEntryLeadsAndEveryEntryIsPlayedOnce)
{
  CPlayListRandomShuffle shuffle;
  const std::vector<EntryId> listOrder{1, 2, 3, 4, 5, 6, 7, 8};
  shuffle.Reset(listOrder, 5);

  std::vector<EntryId> order;
  for (EntryId entry = shuffle.Following(NO_ENTRY); entry != NO_ENTRY;
       entry = shuffle.Following(entry))
    order.emplace_back(entry);

  ASSERT_EQ(listOrder.size(), order.size());
  EXPECT_EQ(5u, order.front());
  std::ranges::sort(order);
  EXPECT_EQ(listOrder, order);
}

TEST(TestPlayListRandomShuffle, AnAddedEntryComesAfterTheCurrentOne)
{
  for (int attempt = 0; attempt < 20; attempt++)
  {
    CPlayListRandomShuffle shuffle;
    shuffle.Reset({1, 2, 3, 4}, 3);
    shuffle.OnAdded(9, 4, 3);

    bool seenCurrent = false;
    for (EntryId entry = shuffle.Following(NO_ENTRY); entry != NO_ENTRY;
         entry = shuffle.Following(entry))
    {
      if (entry == 3)
        seenCurrent = true;
      if (entry == 9)
        EXPECT_TRUE(seenCurrent) << "the new entry was placed among those already played";
    }
  }
}

TEST(TestPlayListTypes, APlaylistIsNamedAsVideoOrAsAudioOrMusic)
{
  EXPECT_EQ(PLAYLIST::Video, PLAYLIST::TypeFromName("video"));
  EXPECT_EQ(PLAYLIST::Audio, PLAYLIST::TypeFromName("Audio"));
  EXPECT_EQ(PLAYLIST::Audio, PLAYLIST::TypeFromName("MUSIC"));
  EXPECT_EQ(std::nullopt, PLAYLIST::TypeFromName("picture"));
  EXPECT_EQ(std::nullopt, PLAYLIST::TypeFromName(""));
}
