/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/PlaceholderPaths.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestPlaceholderPaths, NewPlaylistCoversBothKindsWithTheirSection)
{
  EXPECT_TRUE(ITEM::PLACEHOLDER::IsNewPlaylist(ITEM::PLACEHOLDER::NEW_PLAYLIST));
  EXPECT_TRUE(ITEM::PLACEHOLDER::IsNewPlaylist(std::string{ITEM::PLACEHOLDER::NEW_SMART_PLAYLIST} +
                                               "video"));
  EXPECT_TRUE(ITEM::PLACEHOLDER::IsNewPlaylist("NewPlaylist://"));
  EXPECT_FALSE(ITEM::PLACEHOLDER::IsNewPlaylist(ITEM::PLACEHOLDER::NEW_TAG));
  EXPECT_FALSE(ITEM::PLACEHOLDER::IsNewPlaylist("special://videoplaylists/"));
}

TEST(TestPlaceholderPaths, NewItemAddsTags)
{
  EXPECT_TRUE(ITEM::PLACEHOLDER::IsNewItem(ITEM::PLACEHOLDER::NEW_PLAYLIST));
  EXPECT_TRUE(ITEM::PLACEHOLDER::IsNewItem(std::string{ITEM::PLACEHOLDER::NEW_TAG} + "movies"));
  EXPECT_FALSE(ITEM::PLACEHOLDER::IsNewItem(ITEM::PLACEHOLDER::MUSIC_SEARCH));
  EXPECT_FALSE(ITEM::PLACEHOLDER::IsNewItem(ITEM::PLACEHOLDER::ADD_SOURCE));
}
