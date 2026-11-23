/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryDatabase.h"
#include "test/TestLibrary.h"
#include "utils/SortUtils.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
namespace LIBRARY_QUERY = KODI::MEDIA::LIBRARY_QUERY;

namespace
{
constexpr LIBRARY_QUERY::Source SONGS{MEDIA::MediaSection::MUSIC, MEDIA::TYPE::SONG};

class TestLibraryQueryDatabase : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_songOne = m_library.AddSong("Song one", "Album one", "Artist one");
    m_songTwo = m_library.AddSong("Song two", "Album two", "Artist two");
    ASSERT_GT(m_songOne, 0);
    ASSERT_GT(m_songTwo, 0);
  }

  CTestLibrary m_library{"TestLibraryQueryDatabase"};
  int m_songOne{-1};
  int m_songTwo{-1};
};
} // namespace

TEST_F(TestLibraryQueryDatabase, IdsComeInTheOrderAsked)
{
  SortDescription byTitle;
  byTitle.sortBy = SortBy::TITLE;
  byTitle.sortOrder = SortOrder::DESCENDING;
  EXPECT_EQ((std::vector<int>{m_songTwo, m_songOne}),
            LIBRARY_QUERY::IdsOf(SONGS, nullptr, nullptr, byTitle));

  SortDescription shuffled;
  shuffled.sortBy = SortBy::RANDOM;
  std::optional<std::vector<int>> ids{LIBRARY_QUERY::IdsOf(SONGS, nullptr, nullptr, shuffled)};
  ASSERT_TRUE(ids);
  std::ranges::sort(*ids);
  EXPECT_EQ((std::vector<int>{m_songOne, m_songTwo}), *ids);
}

TEST_F(TestLibraryQueryDatabase, AFieldsRangeIsReadFromItsLibrary)
{
  ASSERT_TRUE(m_library.Music().ExecuteQuery(
      "UPDATE song SET iDuration = CASE strTitle WHEN 'Song one' THEN 90 ELSE 300 END"));

  EXPECT_EQ(std::make_pair(90, 300),
            LIBRARY_QUERY::RangeOf(SONGS, Field::TIME, "musicdb://songs/"));
  EXPECT_FALSE(LIBRARY_QUERY::RangeOf(SONGS, Field::TITLE, "musicdb://songs/"));
}

TEST_F(TestLibraryQueryDatabase, BrowsedValuesAreThoseOfWhatTheQueryMatches)
{
  MEDIA::CLibraryQuery query;
  ASSERT_TRUE(query.LoadFromJson(
      R"({"type":"songs","rules":{"and":[{"field":"title","operator":"is","value":"Song one"}]}})"));

  CFileItemList artists;
  ASSERT_TRUE(LIBRARY_QUERY::BrowseValues(SONGS, LIBRARY_QUERY::BROWSE::ARTISTS, "musicdb://songs/",
                                          &query, false, artists));
  ASSERT_EQ(1, artists.Size());
  EXPECT_EQ("Artist one", artists[0]->GetLabel());
}
