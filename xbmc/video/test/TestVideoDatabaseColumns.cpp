/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/VideoDatabaseColumns.h"

#include <algorithm>
#include <array>

#include <gtest/gtest.h>

// The column index tables in VideoDatabaseColumns.h are maintained by hand and must stay in step
// with the SELECT list of the views in VideoDatabaseDDL.cpp. Inserting a column means renumbering
// every constant after it, and getting that wrong reads a value into the wrong field rather than
// failing loudly. These checks catch the mechanical half of that (duplicates, gaps, wrong order);
// the SQL side is not reachable from a unit test.

TEST(TestVideoDatabaseColumns, SetOffsetsMatchColumnEnum)
{
  // GetDetailsFromDB walks offsets[i] for i in (MIN, MAX), so the table must be MAX entries long
  static_assert(DbSetOffsets.size() == VIDEODB_ID_SET_MAX);
  static_assert(DB_SET_OFFSETS_SIZE == VIDEODB_ID_SET_MAX);

  // and every entry must actually be mapped to a member
  for (int i = VIDEODB_ID_SET_MIN + 1; i < VIDEODB_ID_SET_MAX; ++i)
  {
    EXPECT_NE(DbSetOffsets[i].member, nullptr) << "unmapped `sets` column at index " << i;
    EXPECT_EQ(DbSetOffsets[i].type, VIDEODB_TYPE_STRING) << "unexpected type at index " << i;
  }
}

TEST(TestVideoDatabaseColumns, MovieSetColumnsAreContiguous)
{
  // the movie_view SELECT list puts the `sets` columns together, ahead of the files/path columns
  static_assert(VIDEODB_DETAILS_MOVIE_SET_NAME + 1 == VIDEODB_DETAILS_MOVIE_SET_OVERVIEW);
  static_assert(VIDEODB_DETAILS_MOVIE_SET_OVERVIEW + 1 == VIDEODB_DETAILS_MOVIE_SET_ORIGINALNAME);
  static_assert(VIDEODB_DETAILS_MOVIE_SET_ORIGINALNAME + 1 == VIDEODB_DETAILS_MOVIE_SET_SORTNAME);
  static_assert(VIDEODB_DETAILS_MOVIE_SET_SORTNAME + 1 == VIDEODB_DETAILS_MOVIE_FILE);
}

TEST(TestVideoDatabaseColumns, MovieViewIndicesAreUnique)
{
  // a botched renumber typically leaves two constants sharing an index
  constexpr std::array indices{VIDEODB_DETAILS_MOVIE_SET_ID,
                               VIDEODB_DETAILS_MOVIE_USER_RATING,
                               VIDEODB_DETAILS_MOVIE_PREMIERED,
                               VIDEODB_DETAILS_MOVIE_ORIGINAL_LANGUAGE,
                               VIDEODB_DETAILS_MOVIE_SET_NAME,
                               VIDEODB_DETAILS_MOVIE_SET_OVERVIEW,
                               VIDEODB_DETAILS_MOVIE_SET_ORIGINALNAME,
                               VIDEODB_DETAILS_MOVIE_SET_SORTNAME,
                               VIDEODB_DETAILS_MOVIE_FILE,
                               VIDEODB_DETAILS_MOVIE_PATH,
                               VIDEODB_DETAILS_MOVIE_PLAYCOUNT,
                               VIDEODB_DETAILS_MOVIE_LASTPLAYED,
                               VIDEODB_DETAILS_MOVIE_DATEADDED,
                               VIDEODB_DETAILS_MOVIE_RESUME_TIME,
                               VIDEODB_DETAILS_MOVIE_TOTAL_TIME,
                               VIDEODB_DETAILS_MOVIE_PLAYER_STATE,
                               VIDEODB_DETAILS_MOVIE_RATING,
                               VIDEODB_DETAILS_MOVIE_VOTES,
                               VIDEODB_DETAILS_MOVIE_RATING_TYPE,
                               VIDEODB_DETAILS_MOVIE_UNIQUEID_VALUE,
                               VIDEODB_DETAILS_MOVIE_UNIQUEID_TYPE,
                               VIDEODB_DETAILS_MOVIE_HASVERSIONS,
                               VIDEODB_DETAILS_MOVIE_HASEXTRAS,
                               VIDEODB_DETAILS_MOVIE_ISDEFAULTVERSION,
                               VIDEODB_DETAILS_MOVIE_VERSION_FILEID,
                               VIDEODB_DETAILS_MOVIE_VERSION_TYPEID,
                               VIDEODB_DETAILS_MOVIE_VERSION_TYPENAME,
                               VIDEODB_DETAILS_MOVIE_VERSION_ITEMTYPE};

  auto sorted = indices;
  std::ranges::sort(sorted);

  EXPECT_TRUE(std::ranges::adjacent_find(sorted) == sorted.end())
      << "two movie_view column constants share an index";

  // the constants must also cover a gap-free run, as they map to consecutive SELECT columns
  EXPECT_EQ(sorted.back() - sorted.front() + 1, static_cast<int>(sorted.size()))
      << "movie_view column constants are not contiguous";
}
