/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Scraper.h"
#include "utils/ContentNames.h"

#include <gtest/gtest.h>

using namespace ADDON;

namespace CONTENT = KODI::MEDIA::CONTENT;

TEST(TestScraper, TranslateContentNamesEachType)
{
  EXPECT_EQ(TranslateContent(ContentType::NONE), CONTENT::UNKNOWN);
  EXPECT_EQ(TranslateContent(ContentType::ALBUMS), CONTENT::ALBUMS);
  EXPECT_EQ(TranslateContent(ContentType::ARTISTS), CONTENT::ARTISTS);
  EXPECT_EQ(TranslateContent(ContentType::MOVIES), CONTENT::MOVIES);
  EXPECT_EQ(TranslateContent(ContentType::TVSHOWS), CONTENT::TVSHOWS);
  EXPECT_EQ(TranslateContent(ContentType::MUSICVIDEOS), CONTENT::MUSICVIDEOS);
}

TEST(TestScraper, TranslateContentReadsOnlyTheNamesItWrites)
{
  for (const auto type : {ContentType::NONE, ContentType::ALBUMS, ContentType::ARTISTS,
                          ContentType::MOVIES, ContentType::TVSHOWS, ContentType::MUSICVIDEOS})
    EXPECT_EQ(TranslateContent(TranslateContent(type)), type);

  EXPECT_EQ(TranslateContent("music"), ContentType::NONE);
}
