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
  EXPECT_TRUE(PLACEHOLDER::IsNewPlaylist(PLACEHOLDER::NEW_PLAYLIST));
  EXPECT_TRUE(PLACEHOLDER::IsNewPlaylist(std::string{PLACEHOLDER::NEW_SMART_PLAYLIST} + "video"));
  EXPECT_TRUE(PLACEHOLDER::IsNewPlaylist("NewPlaylist://"));
  EXPECT_FALSE(PLACEHOLDER::IsNewPlaylist(PLACEHOLDER::NEW_TAG));
  EXPECT_FALSE(PLACEHOLDER::IsNewPlaylist("special://videoplaylists/"));
}

TEST(TestPlaceholderPaths, NewItemAddsTags)
{
  EXPECT_TRUE(PLACEHOLDER::IsNewItem(PLACEHOLDER::NEW_PLAYLIST));
  EXPECT_TRUE(PLACEHOLDER::IsNewItem(std::string{PLACEHOLDER::NEW_TAG} + "movies"));
  EXPECT_FALSE(PLACEHOLDER::IsNewItem(PLACEHOLDER::MUSIC_SEARCH));
  EXPECT_FALSE(PLACEHOLDER::IsNewItem(PLACEHOLDER::ADD_SOURCE));
}
