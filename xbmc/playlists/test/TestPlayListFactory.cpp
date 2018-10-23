/*
 *  Copyright (C) 2018 Tyler Szabo
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "playlists/PlayListFactory.h"

#include "playlists/PlayList.h"
#include "playlists/PlayListXSPF.h"
#include "test/TestUtils.h"
#include "URL.h"

#include "gtest/gtest.h"

using namespace PLAYLIST;


TEST(TestPlayListFactory, XSPF)
{
  std::string filename = XBMC_REF_FILE_PATH("/xbmc/playlists/test/newfile.xspf");
  CURL url("http://example.com/playlists/playlist.xspf");
  CPlayList* playlist = nullptr;

  EXPECT_TRUE(CPlayListFactory::IsPlaylist(url));
  EXPECT_TRUE(CPlayListFactory::IsPlaylist(filename));

  playlist = CPlayListFactory::Create(filename);
  EXPECT_NE(playlist, nullptr);

  if (playlist)
  {
    EXPECT_NE(dynamic_cast<CPlayListXSPF*>(playlist), nullptr);
    delete playlist;
  }
}
