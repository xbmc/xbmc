/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
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
