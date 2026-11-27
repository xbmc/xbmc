/*
 *  Copyright (C) 2018 Tyler Szabo
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "playlists/PlayListFactory.h"
#include "playlists/PlayListFile.h"
#include "playlists/PlayListXSPF.h"
#include "test/TestUtils.h"

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestPlayListFactory, XSPF)
{
  std::string filename = XBMC_REF_FILE_PATH("/xbmc/playlists/test/newfile.xspf");
  CURL url("http://example.com/playlists/playlist.xspf");
  PLAYLIST::CPlayListFile* playlist = nullptr;

  EXPECT_TRUE(PLAYLIST::CPlayListFactory::IsPlaylist(url));
  EXPECT_TRUE(PLAYLIST::CPlayListFactory::IsPlaylist(filename));

  playlist = PLAYLIST::CPlayListFactory::Create(filename);
  EXPECT_NE(playlist, nullptr);

  if (playlist)
  {
    EXPECT_NE(dynamic_cast<PLAYLIST::CPlayListXSPF*>(playlist), nullptr);
    delete playlist;
  }
}

TEST(TestPlayListFactory, LoadReadsAPlayListFile)
{
  const auto playlist =
      PLAYLIST::CPlayListFactory::Load(XBMC_REF_FILE_PATH("/xbmc/playlists/test/test.xspf"));
  ASSERT_NE(nullptr, playlist);
  EXPECT_EQ(5, playlist->Size());
}

TEST(TestPlayListFactory, LoadGivesNothingForWhatIsNotAReadablePlayList)
{
  EXPECT_EQ(nullptr, PLAYLIST::CPlayListFactory::Load("/media/film.mkv"));
  EXPECT_EQ(nullptr, PLAYLIST::CPlayListFactory::Load(
                         XBMC_REF_FILE_PATH("/xbmc/playlists/test/missing.xspf")));
}
