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
#include "playlists/PlayListXML.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <fstream>

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestPlayListXML, LoadData)
{
  const std::string filename = XBMC_REF_FILE_PATH("/xbmc/playlists/test/test.pxml");
  PLAYLIST::CPlayListXML playlist;

  EXPECT_TRUE(playlist.Load(filename));

  EXPECT_EQ(playlist.size(), 3);

  // Test name and lang
  EXPECT_STREQ(playlist[0]->GetLabel().c_str(), "Евроспорт [RU]");

  // Test no name and no lang
  EXPECT_STREQ(playlist[1]->GetLabel().c_str(), "mms://video.rfn.ru/vesti_24");

  // Test name and no lang
  EXPECT_STREQ(playlist[2]->GetLabel().c_str(), "Test Name");

  /*
    std::string url = GetString( pSet, "url" );
    std::string name = GetString( pSet, "name" );
    std::string category = GetString( pSet, "category" );
    std::string lang = GetString( pSet, "lang" );
    std::string channel = GetString( pSet, "channel" );
    std::string lockpass = GetString( pSet, "lockpassword" );
*/
  //  EXPECT_STREQ(playlist[1]->GetLabel().c_str(), "demo 2");
}
