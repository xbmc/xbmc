/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "dialogs/ImageChoices.h"
#include "favourites/FavouritesUtils.h"

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
CFileItem FavouriteWithThumb()
{
  CFileItem item{"favourites://test", false};
  item.SetArt("thumb", "special://home/current.png");
  return item;
}
} // namespace

TEST(TestFavouritesUtils, KeepingTheCurrentThumbChangesNothing)
{
  CFileItem item{FavouriteWithThumb()};
  EXPECT_FALSE(FAVOURITES_UTILS::SetChosenThumbnail(item, std::string{IMAGE_CHOICE::CURRENT}));
  EXPECT_EQ(item.GetArt("thumb"), "special://home/current.png");
}

TEST(TestFavouritesUtils, NoThumbClearsIt)
{
  CFileItem item{FavouriteWithThumb()};
  EXPECT_TRUE(FAVOURITES_UTILS::SetChosenThumbnail(item, std::string{IMAGE_CHOICE::NONE}));
  EXPECT_EQ(item.GetArt("thumb"), "");
}

TEST(TestFavouritesUtils, AnImageReplacesIt)
{
  CFileItem item{FavouriteWithThumb()};
  EXPECT_TRUE(FAVOURITES_UTILS::SetChosenThumbnail(item, "special://home/new.png"));
  EXPECT_EQ(item.GetArt("thumb"), "special://home/new.png");
}
