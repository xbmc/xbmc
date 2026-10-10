/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "media/MediaType.h"
#include "utils/ContentNames.h"

#include <gtest/gtest.h>

using namespace KODI;
using KODI::MEDIA::PluralNameOf;

// A list is given its media type's plural as its content in places, so the two must agree
TEST(TestContentNames, AMediaTypesPluralIsItsContent)
{
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::ALBUM), MEDIA::CONTENT::ALBUMS);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::ARTIST), MEDIA::CONTENT::ARTISTS);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::SONG), MEDIA::CONTENT::SONGS);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::MOVIE), MEDIA::CONTENT::MOVIES);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::TV_SHOW), MEDIA::CONTENT::TVSHOWS);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::SEASON), MEDIA::CONTENT::SEASONS);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::EPISODE), MEDIA::CONTENT::EPISODES);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::MUSIC_VIDEO), MEDIA::CONTENT::MUSICVIDEOS);
  EXPECT_EQ(PluralNameOf(MEDIA::TYPE::VIDEO_COLLECTION), MEDIA::CONTENT::SETS);
}
