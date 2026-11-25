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
#include "test/TestLibrary.h"
#include "video/VideoDbUrl.h"
#include "video/VideoLibraryQuery.h"

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

const MEDIA::TYPE ITEM_TYPES[] = {MEDIA::TYPE::MOVIE, MEDIA::TYPE::TV_SHOW, MEDIA::TYPE::EPISODE,
                                  MEDIA::TYPE::MUSIC_VIDEO};
} // namespace

TEST(TestVideoLibraryQuery, AGroupIsListedByAPathWhoseContentIsTheGroups)
{
  int listed{0};
  for (const MEDIA::TYPE itemType : ITEM_TYPES)
  {
    for (const GROUP group : GROUPS)
    {
      const std::optional<std::string_view> path{
          VIDEO::LIBRARY_QUERY::ListingPath(itemType, group)};
      if (!path)
      {
        continue;
      }
      ++listed;
      CVideoDbUrl url;
      ASSERT_TRUE(url.FromString(std::string{*path})) << *path;
      EXPECT_EQ(VIDEO::LIBRARY_QUERY::GroupContent(group), url.GetItemType()) << *path;
    }
  }
  EXPECT_EQ(21, listed);
}

TEST(TestVideoLibraryQuery, TheValuesOfAKindAreListedForAFieldToBeGivenOne)
{
  using MEDIA::LIBRARY_QUERY::BROWSE;
  CTestLibrary library{"TestVideoLibraryQuery"};
  ASSERT_GT(library.AddMovie("Movie"), 0);
  const int idShow{library.AddTvShow("Show")};
  ASSERT_GT(idShow, 0);
  ASSERT_GT(library.AddEpisode(idShow, "Show", "Episode", 1), 0);
  ASSERT_GT(library.AddMusicVideo("Clip", "Band"), 0);

  const auto labelsOf = [&library](BROWSE browse, MEDIA::TYPE itemType)
  {
    CFileItemList items;
    VIDEO::LIBRARY_QUERY::BrowseValues(library.Video(), browse, itemType, "", CDatabase::Filter{},
                                       false, items);
    std::vector<std::string> labels;
    for (const auto& item : items)
    {
      labels.push_back(item->GetLabel());
    }
    return labels;
  };

  EXPECT_EQ(std::vector<std::string>{"Movie"}, labelsOf(BROWSE::TITLES, MEDIA::TYPE::MOVIE));
  EXPECT_EQ(std::vector<std::string>{"Show"}, labelsOf(BROWSE::TITLES, MEDIA::TYPE::TV_SHOW));
  EXPECT_EQ(std::vector<std::string>{"Show"}, labelsOf(BROWSE::TVSHOWS, MEDIA::TYPE::EPISODE));
  EXPECT_EQ(std::vector<std::string>{"Episode"}, labelsOf(BROWSE::TITLES, MEDIA::TYPE::EPISODE))
      << "an episode is browsed by its title alone";
  EXPECT_EQ(std::vector<std::string>{"Clip"}, labelsOf(BROWSE::TITLES, MEDIA::TYPE::MUSIC_VIDEO));
  EXPECT_EQ(std::vector<std::string>{"Band"}, labelsOf(BROWSE::ARTISTS, MEDIA::TYPE::MUSIC_VIDEO));
  EXPECT_TRUE(labelsOf(BROWSE::ARTISTS, MEDIA::TYPE::MOVIE).empty());
  EXPECT_TRUE(labelsOf(BROWSE::SOURCES, MEDIA::TYPE::MOVIE).empty());
}
