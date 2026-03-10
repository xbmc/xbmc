/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/SortUtils.h"
#include "utils/Variant.h"

#include <gtest/gtest.h>

TEST(TestSortUtils, Sort_SortBy)
{
  SortItems items;

  CVariant variant1("M Artist");
  auto item1 = std::make_shared<SortItem>();
  (*item1)[Field::ARTIST] = variant1;
  CVariant variant2("B Artist");
  auto item2 = std::make_shared<SortItem>();
  (*item2)[Field::ARTIST] = variant2;
  CVariant variant3("R Artist");
  auto item3 = std::make_shared<SortItem>();
  (*item3)[Field::ARTIST] = variant3;
  CVariant variant4("R Artist");
  auto item4 = std::make_shared<SortItem>();
  (*item4)[Field::ARTIST] = variant4;
  CVariant variant5("I Artist");
  auto item5 = std::make_shared<SortItem>();
  (*item5)[Field::ARTIST] = variant5;
  CVariant variant6("A Artist");
  auto item6 = std::make_shared<SortItem>();
  (*item6)[Field::ARTIST] = variant6;
  CVariant variant7("G Artist");
  auto item7 = std::make_shared<SortItem>();
  (*item7)[Field::ARTIST] = variant7;

  items.push_back(item1);
  items.push_back(item2);
  items.push_back(item3);
  items.push_back(item4);
  items.push_back(item5);
  items.push_back(item6);
  items.push_back(item7);

  SortUtils::Sort(SortBy::ARTIST, SortOrder::ASCENDING, SortAttributeNone, items);

  EXPECT_STREQ("A Artist", (*items.at(0))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("B Artist", (*items.at(1))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("G Artist", (*items.at(2))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("I Artist", (*items.at(3))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("M Artist", (*items.at(4))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(5))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(6))[Field::ARTIST].asString().c_str());
}

TEST(TestSortUtils, Sort_SortDescription)
{
  SortItems items;

  CVariant variant1("M Artist");
  auto item1 = std::make_shared<SortItem>();
  (*item1)[Field::ARTIST] = variant1;
  CVariant variant2("B Artist");
  auto item2 = std::make_shared<SortItem>();
  (*item2)[Field::ARTIST] = variant2;
  CVariant variant3("R Artist");
  auto item3 = std::make_shared<SortItem>();
  (*item3)[Field::ARTIST] = variant3;
  CVariant variant4("R Artist");
  auto item4 = std::make_shared<SortItem>();
  (*item4)[Field::ARTIST] = variant4;
  CVariant variant5("I Artist");
  auto item5 = std::make_shared<SortItem>();
  (*item5)[Field::ARTIST] = variant5;
  CVariant variant6("A Artist");
  auto item6 = std::make_shared<SortItem>();
  (*item6)[Field::ARTIST] = variant6;
  CVariant variant7("G Artist");
  auto item7 = std::make_shared<SortItem>();
  (*item7)[Field::ARTIST] = variant7;

  items.push_back(item1);
  items.push_back(item2);
  items.push_back(item3);
  items.push_back(item4);
  items.push_back(item5);
  items.push_back(item6);
  items.push_back(item7);

  SortDescription desc;
  desc.sortBy = SortBy::ARTIST;
  SortUtils::Sort(desc, items);

  EXPECT_STREQ("A Artist", (*items.at(0))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("B Artist", (*items.at(1))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("G Artist", (*items.at(2))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("I Artist", (*items.at(3))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("M Artist", (*items.at(4))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(5))[Field::ARTIST].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(6))[Field::ARTIST].asString().c_str());
}

TEST(TestSortUtils, GetFieldsForSorting)
{
  Fields fields;

  fields = SortUtils::GetFieldsForSorting(SortBy::ARTIST);
  Fields::iterator it;
  it = fields.find(Field::ALBUM);
  EXPECT_EQ(Field::ALBUM, *it);
  it = fields.find(Field::ARTIST);
  EXPECT_EQ(Field::ARTIST, *it);
  it = fields.find(Field::ARTIST_SORT);
  EXPECT_EQ(Field::ARTIST_SORT, *it);
  it = fields.find(Field::YEAR);
  EXPECT_EQ(Field::YEAR, *it);
  it = fields.find(Field::TRACK_NUMBER);
  EXPECT_EQ(Field::TRACK_NUMBER, *it);
  EXPECT_EQ(5U, fields.size());
}
