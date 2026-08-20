/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "media/MediaType.h"
#include "utils/GroupUtils.h"
#include "utils/SortUtils.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
std::shared_ptr<CFileItem> MakeMovieInSet(const std::string& movieTitle,
                                          int idSet,
                                          const std::string& setTitle,
                                          const std::string& setSortTitle)
{
  auto item{std::make_shared<CFileItem>(movieTitle)};
  item->SetPath("videodb://movies/titles/1");

  CVideoInfoTag* tag{item->GetVideoInfoTag()};
  tag->m_type = MediaTypeMovie;
  tag->m_strTitle = movieTitle;
  tag->m_set.SetID(idSet);
  tag->m_set.SetTitle(setTitle);
  if (!setSortTitle.empty())
    tag->m_set.SetSortTitle(setSortTitle);

  return item;
}

std::shared_ptr<CFileItem> MakeStandaloneMovie(const std::string& movieTitle)
{
  auto item{std::make_shared<CFileItem>(movieTitle)};
  item->SetPath("videodb://movies/titles/2");

  CVideoInfoTag* tag{item->GetVideoInfoTag()};
  tag->m_type = MediaTypeMovie;
  tag->m_strTitle = movieTitle;

  return item;
}
} // unnamed namespace

TEST(TestGroupUtils, GroupBySetCarriesSetSortTitle)
{
  CFileItemList items;
  items.Add(MakeMovieInSet("The Fellowship of the Ring", 7, "The Lord of the Rings Collection",
                           "Lord of the Rings, The"));
  items.Add(MakeMovieInSet("The Two Towers", 7, "The Lord of the Rings Collection",
                           "Lord of the Rings, The"));

  CFileItemList sets;
  ASSERT_TRUE(GroupUtils::Group(GroupBySet, "videodb://movies/sets/", items, sets));
  ASSERT_EQ(sets.Size(), 1);

  // the grouped set item is what the library list sorts, so it must carry the set's sort title
  EXPECT_EQ(sets[0]->GetVideoInfoTag()->m_strTitle, "The Lord of the Rings Collection");
  EXPECT_EQ(sets[0]->GetVideoInfoTag()->m_strSortTitle, "Lord of the Rings, The");
}

TEST(TestGroupUtils, GroupBySetWithoutSortTitleLeavesItEmpty)
{
  CFileItemList items;
  items.Add(MakeMovieInSet("A Movie", 3, "A Set", ""));
  items.Add(MakeMovieInSet("Another Movie", 3, "A Set", ""));

  CFileItemList sets;
  ASSERT_TRUE(GroupUtils::Group(GroupBySet, "videodb://movies/sets/", items, sets));
  ASSERT_EQ(sets.Size(), 1);

  // SortUtils falls back to the title, so an empty sort title must stay empty rather than
  // be filled in with the title
  EXPECT_EQ(sets[0]->GetVideoInfoTag()->m_strTitle, "A Set");
  EXPECT_TRUE(sets[0]->GetVideoInfoTag()->m_strSortTitle.empty());
}

TEST(TestGroupUtils, MixedSetsAndMoviesSortByTheirOwnSortTitles)
{
  // this is the movie titles list with "group movies into sets" enabled: set folders and
  // standalone movies side by side. CFileItemList::Sort adds SortAttributeIgnoreFolders for
  // SortBy::SORT_TITLE, so the two kinds interleave rather than folders being hoisted.
  CFileItemList items;
  items.Add(MakeMovieInSet("A Set Member", 7, "Aardvark Collection", "Zebra Collection"));
  items.Add(MakeStandaloneMovie("Middle Movie"));

  CFileItemList mixed;
  ASSERT_TRUE(GroupUtils::GroupAndMix(GroupBySet, "videodb://movies/titles/", items, mixed));
  ASSERT_EQ(mixed.Size(), 2);

  mixed.Sort(SortBy::SORT_TITLE, SortOrder::ASCENDING);

  // the set sorts under its sort title ("Zebra"), not its title ("Aardvark"), so it lands last
  EXPECT_EQ(mixed[0]->GetVideoInfoTag()->m_strTitle, "Middle Movie");
  EXPECT_EQ(mixed[1]->GetVideoInfoTag()->m_strTitle, "Aardvark Collection");
  EXPECT_EQ(mixed[1]->GetVideoInfoTag()->m_strSortTitle, "Zebra Collection");
}
