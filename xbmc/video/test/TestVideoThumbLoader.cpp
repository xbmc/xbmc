/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "imagefiles/ImageFileURL.h"
#include "video/VideoThumbLoader.h"

#include <gtest/gtest.h>

TEST(TestVideoThumbLoader, EmbeddedThumbUsesDynamicPath)
{
  const std::string mediaPath{"smb://server/Videos/Movie.mkv"};
  CFileItem item{"smb://server/Displayed source/Movie.mkv", false};
  item.SetDynPath(mediaPath);

  EXPECT_EQ(CVideoThumbLoader::GetEmbeddedThumbURL(item),
            IMAGE_FILES::URLFromFile(mediaPath, "video"));
}

TEST(TestVideoThumbLoader, EmbeddedThumbIsNotExtractedFromAStackOfDiscPlaylists)
{
  const CFileItem files{"stack:///movies/Movie Part 1.mkv , /movies/Movie Part 2.mkv", false};
  EXPECT_TRUE(CVideoThumbLoader::CanExtractEmbeddedThumb(files));

  // Read for its details, but the thumb would be taken from a bluray:// playlist on its own
  const CFileItem playlists{
      "stack://bluray://%2fmovies%2fMovie%2fPart%201%2f/BDMV/PLAYLIST/00800.mpls"
      " , bluray://%2fmovies%2fMovie%2fPart%202%2f/BDMV/PLAYLIST/00800.mpls",
      false};
  EXPECT_TRUE(CDVDFileInfo::CanExtract(playlists));
  EXPECT_FALSE(CVideoThumbLoader::CanExtractEmbeddedThumb(playlists));
}
