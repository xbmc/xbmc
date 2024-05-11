/*
 *  Copyright (C) 2018 Tyler Szabo
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "playlists/PlayListXSPF.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI;

TEST(TestPlayListXSPF, Load)
{
  std::string filename = XBMC_REF_FILE_PATH("/xbmc/playlists/test/test.xspf");
  PLAYLIST::CPlayListXSPF playlist;
  std::vector<std::string> pathparts;
  std::vector<std::string>::reverse_iterator it;

  EXPECT_TRUE(playlist.Load(filename));

  EXPECT_EQ(playlist.size(), 5);
  EXPECT_STREQ(playlist.GetName().c_str(), "Various Music");


  ASSERT_GT(playlist.size(), 0);
  EXPECT_STREQ(playlist[0]->GetLabel().c_str(), "");
  EXPECT_STREQ(playlist[0]->GetURL().Get().c_str(), "http://example.com/song_1.mp3");


  ASSERT_GT(playlist.size(), 1);
  EXPECT_STREQ(playlist[1]->GetLabel().c_str(), "Relative local file");
  pathparts = URIUtils::SplitPath(playlist[1]->GetPath());
  it = pathparts.rbegin();
  EXPECT_STREQ((*it++).c_str(), "song_2.mp3");
  EXPECT_STREQ((*it++).c_str(), "path_to");
  EXPECT_STREQ((*it++).c_str(), "relative");
  EXPECT_STREQ((*it++).c_str(), "test");
  EXPECT_STREQ((*it++).c_str(), "playlists");
  EXPECT_STREQ((*it++).c_str(), "xbmc");


  ASSERT_GT(playlist.size(), 2);
  EXPECT_STREQ(playlist[2]->GetLabel().c_str(), "Don\xC2\x92t Worry, We\xC2\x92ll Be Watching You");
  pathparts = URIUtils::SplitPath(playlist[2]->GetPath());
  it = pathparts.rbegin();
  EXPECT_STREQ((*it++).c_str(), "09 - Don't Worry, We'll Be Watching You.mp3");
  EXPECT_STREQ((*it++).c_str(), "Making Mirrors");
  EXPECT_STREQ((*it++).c_str(), "Gotye");
  EXPECT_STREQ((*it++).c_str(), "Music");
  EXPECT_STREQ((*it++).c_str(), "jane");
  EXPECT_STREQ((*it++).c_str(), "Users");
  EXPECT_STREQ((*it++).c_str(), "C:");


  ASSERT_GT(playlist.size(), 3);
  EXPECT_STREQ(playlist[3]->GetLabel().c_str(), "Rollin' & Scratchin'");
  pathparts = URIUtils::SplitPath(playlist[3]->GetPath());
  it = pathparts.rbegin();
  EXPECT_STREQ((*it++).c_str(), "08 - Rollin' & Scratchin'.mp3");
  EXPECT_STREQ((*it++).c_str(), "Homework");
  EXPECT_STREQ((*it++).c_str(), "Daft Punk");
  EXPECT_STREQ((*it++).c_str(), "Music");
  EXPECT_STREQ((*it++).c_str(), "jane");
  EXPECT_STREQ((*it++).c_str(), "home");


  ASSERT_GT(playlist.size(), 4);
  EXPECT_STREQ(playlist[4]->GetLabel().c_str(), "");
  EXPECT_STREQ(playlist[4]->GetURL().Get().c_str(), "http://example.com/song_2.mp3");
}
