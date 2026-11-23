/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "media/LibraryQueryType.h"
#include "media/MediaType.h"
#include "music/MusicDbUrl.h"
#include "music/MusicLibraryQuery.h"
#include "settings/Settings.h"
#include "test/TestLibrary.h"
#include "test/TestUtils.h"
#include "utils/SortUtils.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using KODI::MEDIA::LIBRARY_QUERY::GROUP;

namespace
{
const GROUP GROUPS[] = {GROUP::SET,      GROUP::GENRE,  GROUP::YEAR,          GROUP::PERFORMER,
                        GROUP::DIRECTOR, GROUP::WRITER, GROUP::STUDIO,        GROUP::COUNTRY,
                        GROUP::ALBUM,    GROUP::TAG,    GROUP::ORIGINAL_YEAR, GROUP::VIDEO_VERSION};
} // namespace

TEST(TestMusicLibraryQuery, AGroupIsListedByAPathWhoseContentIsTheGroups)
{
  int listed{0};
  for (const GROUP group : GROUPS)
  {
    const std::optional<std::string_view> path{
        MUSIC::LIBRARY_QUERY::ListingPath(MEDIA::TYPE::ALBUM, group)};
    const std::string_view content{MUSIC::LIBRARY_QUERY::GroupContent(group)};
    EXPECT_EQ(path.has_value(), !content.empty());
    if (!path)
    {
      continue;
    }
    ++listed;
    CMusicDbUrl url;
    ASSERT_TRUE(url.FromString(std::string{*path})) << *path;
    EXPECT_EQ(content, url.GetType()) << *path;
  }
  EXPECT_EQ(5, listed);
  EXPECT_EQ("musicdb://originalyears/",
            MUSIC::LIBRARY_QUERY::ListingPath(MEDIA::TYPE::ALBUM, GROUP::ORIGINAL_YEAR));
  EXPECT_EQ("musicdb://artists/",
            MUSIC::LIBRARY_QUERY::ListingPath(MEDIA::TYPE::SONG, GROUP::PERFORMER));
}

TEST(TestMusicLibraryQuery, APlaylistThatStatesNoDirectionLeavesTheListingsOrderAlone)
{
  CTestLibrary library{"TestMusicLibraryQuery"};
  ASSERT_GT(library.AddSong("Song a", "Album", "Artist"), 0);
  ASSERT_GT(library.AddSong("Song b", "Album", "Artist"), 0);

  CMusicDbUrl url;
  ASSERT_TRUE(url.FromString("musicdb://songs/"));
  url.AddOption("xsp", R"({"rules":{"and":[{"field":"title","operator":"startswith",)"
                       R"("value":["Song"]}]},"type":"songs"})");
  SortDescription sorting;
  sorting.sortBy = SortBy::TITLE;
  sorting.sortOrder = SortOrder::DESCENDING;

  CFileItemList items;
  ASSERT_TRUE(library.Music().GetItems(url.ToString(), items, sorting, CDatabase::Filter{}));
  ASSERT_EQ(2, items.Size());
  EXPECT_EQ("Song b", items[0]->GetLabel());
  EXPECT_EQ("Song a", items[1]->GetLabel());
}

TEST(TestMusicLibraryQuery, TheOriginalDateSettingDecidesWhichYearAYearRuleMeans)
{
  CTestLibrary library{"TestMusicLibraryQuery"};
  ASSERT_GT(library.AddSong("Song", "Album", "Artist"), 0);
  ASSERT_TRUE(library.Music().ExecuteQuery(
      "UPDATE song SET strReleaseDate = '2000', strOrigReleaseDate = '1990'"));

  const auto songsOfYear = [&library](const std::string& year)
  {
    CMusicDbUrl url;
    EXPECT_TRUE(url.FromString("musicdb://songs/"));
    url.AddOption("xsp", R"({"rules":{"and":[{"field":"year","operator":"is","value":[")" + year +
                             R"("]}]},"type":"songs"})");
    CFileItemList items;
    EXPECT_TRUE(
        library.Music().GetItems(url.ToString(), items, SortDescription{}, CDatabase::Filter{}));
    return items.Size();
  };

  {
    const CScopedSetting released{CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE, false};
    EXPECT_EQ(1, songsOfYear("2000"));
    EXPECT_EQ(0, songsOfYear("1990"));
  }
  {
    const CScopedSetting original{CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE, true};
    EXPECT_EQ(0, songsOfYear("2000"));
    EXPECT_EQ(1, songsOfYear("1990"));
  }
}

TEST(TestMusicLibraryQuery, TheValuesOfAKindAreListedForAFieldToBeGivenOne)
{
  using MEDIA::LIBRARY_QUERY::BROWSE;
  CTestLibrary library{"TestMusicLibraryQuery"};
  ASSERT_GT(library.AddSong("Song", "Album", "Artist"), 0);

  const auto labelsOf = [&library](BROWSE browse, MEDIA::TYPE itemType)
  {
    CFileItemList items;
    MUSIC::LIBRARY_QUERY::BrowseValues(library.Music(), browse, itemType, "", CDatabase::Filter{},
                                       false, items);
    std::vector<std::string> labels;
    for (const auto& item : items)
    {
      labels.push_back(item->GetLabel());
    }
    return labels;
  };

  EXPECT_EQ(std::vector<std::string>{"Album"}, labelsOf(BROWSE::ALBUMS, MEDIA::TYPE::SONG));
  EXPECT_EQ(std::vector<std::string>{"Artist"}, labelsOf(BROWSE::ARTISTS, MEDIA::TYPE::SONG));
  EXPECT_EQ(std::vector<std::string>{"Song"}, labelsOf(BROWSE::TITLES, MEDIA::TYPE::SONG));
  EXPECT_TRUE(labelsOf(BROWSE::TITLES, MEDIA::TYPE::ALBUM).empty());
  EXPECT_TRUE(labelsOf(BROWSE::ACTORS, MEDIA::TYPE::SONG).empty());
}
