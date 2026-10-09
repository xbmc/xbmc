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

// A list is given its media type's plural as its content in places, so the two must agree
TEST(TestContentNames, AMediaTypesPluralIsItsContent)
{
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeAlbum), MEDIA::CONTENT::ALBUMS);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeArtist), MEDIA::CONTENT::ARTISTS);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeSong), MEDIA::CONTENT::SONGS);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeMovie), MEDIA::CONTENT::MOVIES);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeTvShow), MEDIA::CONTENT::TVSHOWS);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeSeason), MEDIA::CONTENT::SEASONS);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeEpisode), MEDIA::CONTENT::EPISODES);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeMusicVideo), MEDIA::CONTENT::MUSICVIDEOS);
  EXPECT_EQ(CMediaTypes::ToPlural(MediaTypeVideoCollection), MEDIA::CONTENT::SETS);
}
