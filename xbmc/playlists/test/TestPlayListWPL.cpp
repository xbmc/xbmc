/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "Util.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayListWPL.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <fstream>

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestPlayListWPL, LoadData)
{
  const std::string filename = XBMC_REF_FILE_PATH("/xbmc/playlists/test/test.wpl");
  PLAYLIST::CPlayListWPL playlist;

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
  EXPECT_STREQ(playlist[0]->GetLabel().c_str(), "track01.mp3");

  std::string track_url = "\\\\server\\vol\\music\\Classical\\Composer\\OrganWorks\\cd03\\track01.mp3";
  std::string track_path = "";
  CUtil::GetQualifiedFilename(track_path, track_url);

  EXPECT_STREQ(playlist[0]->GetURL().Get().c_str(),
               (track_path + track_url).c_str());


  track_url = "\\\\server\\vol\\music\\Classical\\Composer\\OrganWorks\\cd03\\track02.mp3";
  track_path = "";
  CUtil::GetQualifiedFilename(track_path, track_url);
  EXPECT_STREQ(playlist[1]->GetURL().Get().c_str(),
               (track_path + track_url).c_str());
}
