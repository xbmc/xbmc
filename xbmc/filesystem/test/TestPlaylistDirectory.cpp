/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/PlaylistDirectory.h"

#include <gtest/gtest.h>

using namespace KODI;
using XFILE::CPlaylistDirectory;

TEST(TestPlaylistDirectory, PathNamesItsPlaylist)
{
  for (const PLAYLIST::Type type : {PLAYLIST::Audio, PLAYLIST::Video})
    EXPECT_EQ(CPlaylistDirectory::TypeOf(CURL(CPlaylistDirectory::PathOf(type))), type);
  EXPECT_EQ(CPlaylistDirectory::PathOf(PLAYLIST::Audio), "playlistmusic://");
  EXPECT_EQ(CPlaylistDirectory::PathOf(PLAYLIST::Video), "playlistvideo://");
}

TEST(TestPlaylistDirectory, OtherPathsAreNoPlaylist)
{
  EXPECT_FALSE(CPlaylistDirectory::TypeOf(CURL("musicdb://songs/")));
}
