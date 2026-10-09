/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "media/MediaType.h"

#include <gtest/gtest.h>

using namespace KODI::MEDIA;

namespace
{
constexpr KODI::MEDIA::TYPE FIRST{KODI::MEDIA::TYPE::MUSIC};
constexpr KODI::MEDIA::TYPE LAST{KODI::MEDIA::TYPE::VIDEO_VERSION};

KODI::MEDIA::TYPE Next(KODI::MEDIA::TYPE type)
{
  return static_cast<KODI::MEDIA::TYPE>(static_cast<int>(type) + 1);
}
} // namespace

TEST(TestMediaType, EveryTypeReadsBackFromItsNames)
{
  for (KODI::MEDIA::TYPE type{FIRST}; type <= LAST; type = Next(type))
  {
    EXPECT_FALSE(NameOf(type).empty());
    EXPECT_EQ(MediaTypeFromName(NameOf(type)), type) << NameOf(type);
    EXPECT_EQ(MediaTypeFromName(PluralNameOf(type)), type) << PluralNameOf(type);
  }
}

TEST(TestMediaType, NamesAreTheStoredSpellings)
{
  EXPECT_EQ(NameOf(KODI::MEDIA::TYPE::VIDEO_COLLECTION), "set");
  EXPECT_EQ(PluralNameOf(KODI::MEDIA::TYPE::VIDEO_COLLECTION), "sets");
  EXPECT_EQ(NameOf(KODI::MEDIA::TYPE::TV_SHOW), "tvshow");
  EXPECT_EQ(PluralNameOf(KODI::MEDIA::TYPE::MUSIC), "music");
  EXPECT_EQ(NameOf(KODI::MEDIA::TYPE::NONE), "");
}

TEST(TestMediaType, AFacetOrUnknownNameIsNoType)
{
  EXPECT_EQ(MediaTypeFromName(""), KODI::MEDIA::TYPE::NONE);
  EXPECT_EQ(MediaTypeFromName("genre"), KODI::MEDIA::TYPE::NONE);
  EXPECT_EQ(MediaTypeFromName("actor"), KODI::MEDIA::TYPE::NONE);
  EXPECT_EQ(MediaTypeFromName("year"), KODI::MEDIA::TYPE::NONE);
}

TEST(TestMediaType, ANameIsReadInAnyCase)
{
  EXPECT_EQ(MediaTypeFromName("Movie"), KODI::MEDIA::TYPE::MOVIE);
  EXPECT_EQ(MediaTypeFromName("TVSHOWS"), KODI::MEDIA::TYPE::TV_SHOW);
}

TEST(TestMediaType, OnlyTheStoredNameIsTheTypeExactly)
{
  for (KODI::MEDIA::TYPE type{FIRST}; type <= LAST; type = Next(type))
    EXPECT_EQ(MediaTypeOf(NameOf(type)), type) << NameOf(type);
  EXPECT_EQ(MediaTypeOf("movies"), KODI::MEDIA::TYPE::NONE);
  EXPECT_EQ(MediaTypeOf("Movie"), KODI::MEDIA::TYPE::NONE);
  EXPECT_EQ(MediaTypeOf("genre"), KODI::MEDIA::TYPE::NONE);
  EXPECT_EQ(MediaTypeOf(""), KODI::MEDIA::TYPE::NONE);
}

TEST(TestMediaType, ContainersHoldOtherItems)
{
  EXPECT_TRUE(IsContainer(KODI::MEDIA::TYPE::ALBUM));
  EXPECT_TRUE(IsContainer(KODI::MEDIA::TYPE::TV_SHOW));
  EXPECT_TRUE(IsContainer(KODI::MEDIA::TYPE::VIDEO_COLLECTION));
  EXPECT_FALSE(IsContainer(KODI::MEDIA::TYPE::MOVIE));
  EXPECT_FALSE(IsContainer(KODI::MEDIA::TYPE::SONG));
  EXPECT_FALSE(IsContainer(KODI::MEDIA::TYPE::NONE));
}
