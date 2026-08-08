/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "music/Artist.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderMatroska.h"
#include "test/TestUtils.h"

#include <string>

#include <gtest/gtest.h>

/*!
 * The single file path: a Matroska holding one song rather than an album, which never reaches
 * CAudioBookFileDirectory. The fixture comes from tools/testdata/mkmka.py, and what is asserted
 * holds for either reader.
 */
using namespace MUSIC_INFO;

TEST(TestMusicInfoTagLoaderMatroska, ReadsAFileWithNoChapters)
{
  const std::string path = XBMC_REF_FILE_PATH("xbmc/filesystem/test/data/audiobook/singlefile.mka");

  CMusicInfoTag tag;
  CMusicInfoTagLoaderMatroska loader;
  ASSERT_TRUE(loader.Load(path, tag, nullptr));

  EXPECT_TRUE(tag.Loaded());
  EXPECT_EQ("Live At The Test Venue", tag.GetAlbum());
  EXPECT_EQ("Live At The Test Venue", tag.GetTitle());
  // The file tags it at TargetTypeValue 50, which is the album's artist rather than the track's.
  EXPECT_EQ("The Test Band", tag.GetAlbumArtistString());
  EXPECT_EQ("2026", tag.GetReleaseDate());

  bool composed = false;
  for (const auto& c : tag.GetContributors())
    composed = composed || (c.GetRoleDesc() == "Composer" && c.GetArtist() == "Bill Evans");
  EXPECT_TRUE(composed);
}
