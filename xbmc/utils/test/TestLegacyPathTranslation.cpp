/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LegacyPathTranslation.h"

#include <gtest/gtest.h>

TEST(TestLegacyPathTranslation, ALongerLegacyPathIsNotTakenForItsPrefix)
{
  EXPECT_EQ(CLegacyPathTranslation::TranslateMusicDbPath("musicdb://10/"), "musicdb://singles/");
  EXPECT_EQ(CLegacyPathTranslation::TranslateMusicDbPath("musicdb://1/"), "musicdb://genres/");
  EXPECT_EQ(CLegacyPathTranslation::TranslateMusicDbPath("musicdb://5/1/"),
            "musicdb://top100/albums/");
  EXPECT_EQ(CLegacyPathTranslation::TranslateMusicDbPath("musicdb://5/"), "musicdb://top100/");

  EXPECT_EQ(CLegacyPathTranslation::TranslateVideoDbPath("videodb://1/2/"),
            "videodb://movies/titles/");
  EXPECT_EQ(CLegacyPathTranslation::TranslateVideoDbPath("videodb://1/"), "videodb://movies/");
  EXPECT_EQ(CLegacyPathTranslation::TranslateVideoDbPath("videodb://3/9/"),
            "videodb://musicvideos/tags/");
}

TEST(TestLegacyPathTranslation, ACurrentPathIsLeftAsItIs)
{
  EXPECT_EQ(CLegacyPathTranslation::TranslateVideoDbPath("videodb://movies/titles/"),
            "videodb://movies/titles/");
  EXPECT_EQ(CLegacyPathTranslation::TranslateMusicDbPath("musicdb://albums/"), "musicdb://albums/");
}
