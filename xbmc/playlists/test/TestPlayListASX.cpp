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
#include "playlists/PlayListASX.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <fstream>

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestPlayListASX, LoadData)
{
  const std::string filename = XBMC_REF_FILE_PATH("/xbmc/playlists/test/test.asx");
  PLAYLIST::CPlayListASX playlist;

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

  EXPECT_STREQ(playlist[1]->GetLabel().c_str(), "Example radio");

  EXPECT_STREQ(playlist[2]->GetLabel().c_str(), "Capital Title");
}
