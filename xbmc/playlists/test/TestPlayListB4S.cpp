/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayListB4S.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <fstream>
#include <gtest/gtest.h>

using namespace PLAYLIST;


TEST(TestPlayListB4S, LoadData)
{
  const std::string filename = XBMC_REF_FILE_PATH("/xbmc/playlists/test/test.b4s");
  CPlayListB4S playlist;

  std::filebuf fb;
  if (!fb.open(filename, std::ios::in))
  {
    // Test Failure
    EXPECT_TRUE(false);
  }

  std::istream is(&fb);
  EXPECT_TRUE(playlist.LoadData(is));
  fb.close();

  EXPECT_EQ(playlist.size(), 3);
  EXPECT_STREQ(playlist.GetName().c_str(), "Playlist 001");

  EXPECT_STREQ(playlist[1]->GetLabel().c_str(), "demo 2");
  // It would be nice to test path, however cross platform handling makes this difficult
  // windows strips the leading /
  //EXPECT_STREQ(playlist[2]->GetPath().c_str(), "/Users/Shared/test.mp3");

  const CFileItemPtr item {playlist[2]};
  EXPECT_EQ(item->GetMusicInfoTag()->GetDuration(), 264);
}
