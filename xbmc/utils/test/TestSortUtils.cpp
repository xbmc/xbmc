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
  (*item1)[FieldArtist] = variant1;
  CVariant variant2("B Artist");
  auto item2 = std::make_shared<SortItem>();
  (*item2)[FieldArtist] = variant2;
  CVariant variant3("R Artist");
  auto item3 = std::make_shared<SortItem>();
  (*item3)[FieldArtist] = variant3;
  CVariant variant4("R Artist");
  auto item4 = std::make_shared<SortItem>();
  (*item4)[FieldArtist] = variant4;
  CVariant variant5("I Artist");
  auto item5 = std::make_shared<SortItem>();
  (*item5)[FieldArtist] = variant5;
  CVariant variant6("A Artist");
  auto item6 = std::make_shared<SortItem>();
  (*item6)[FieldArtist] = variant6;
  CVariant variant7("G Artist");
  auto item7 = std::make_shared<SortItem>();
  (*item7)[FieldArtist] = variant7;

  items.push_back(item1);
  items.push_back(item2);
  items.push_back(item3);
  items.push_back(item4);
  items.push_back(item5);
  items.push_back(item6);
  items.push_back(item7);

  SortUtils::Sort(SortByArtist, SortOrder::ASCENDING, SortAttributeNone, items);

  EXPECT_STREQ("A Artist", (*items.at(0))[FieldArtist].asString().c_str());
  EXPECT_STREQ("B Artist", (*items.at(1))[FieldArtist].asString().c_str());
  EXPECT_STREQ("G Artist", (*items.at(2))[FieldArtist].asString().c_str());
  EXPECT_STREQ("I Artist", (*items.at(3))[FieldArtist].asString().c_str());
  EXPECT_STREQ("M Artist", (*items.at(4))[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(5))[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(6))[FieldArtist].asString().c_str());
}

TEST(TestSortUtils, Sort_SortDescription)
{
  SortItems items;

  CVariant variant1("M Artist");
  auto item1 = std::make_shared<SortItem>();
  (*item1)[FieldArtist] = variant1;
  CVariant variant2("B Artist");
  auto item2 = std::make_shared<SortItem>();
  (*item2)[FieldArtist] = variant2;
  CVariant variant3("R Artist");
  auto item3 = std::make_shared<SortItem>();
  (*item3)[FieldArtist] = variant3;
  CVariant variant4("R Artist");
  auto item4 = std::make_shared<SortItem>();
  (*item4)[FieldArtist] = variant4;
  CVariant variant5("I Artist");
  auto item5 = std::make_shared<SortItem>();
  (*item5)[FieldArtist] = variant5;
  CVariant variant6("A Artist");
  auto item6 = std::make_shared<SortItem>();
  (*item6)[FieldArtist] = variant6;
  CVariant variant7("G Artist");
  auto item7 = std::make_shared<SortItem>();
  (*item7)[FieldArtist] = variant7;

  items.push_back(item1);
  items.push_back(item2);
  items.push_back(item3);
  items.push_back(item4);
  items.push_back(item5);
  items.push_back(item6);
  items.push_back(item7);

  SortDescription desc;
  desc.sortBy = SortByArtist;
  SortUtils::Sort(desc, items);

  EXPECT_STREQ("A Artist", (*items.at(0))[FieldArtist].asString().c_str());
  EXPECT_STREQ("B Artist", (*items.at(1))[FieldArtist].asString().c_str());
  EXPECT_STREQ("G Artist", (*items.at(2))[FieldArtist].asString().c_str());
  EXPECT_STREQ("I Artist", (*items.at(3))[FieldArtist].asString().c_str());
  EXPECT_STREQ("M Artist", (*items.at(4))[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(5))[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", (*items.at(6))[FieldArtist].asString().c_str());
}

TEST(TestSortUtils, GetFieldsForSorting)
{
  Fields fields;

  fields = SortUtils::GetFieldsForSorting(SortByArtist);
  Fields::iterator it;
  it = fields.find(FieldAlbum);
  EXPECT_EQ(FieldAlbum, *it);
  it = fields.find(FieldArtist);
  EXPECT_EQ(FieldArtist, *it);
  it = fields.find(FieldArtistSort);
  EXPECT_EQ(FieldArtistSort, *it);
  it = fields.find(FieldYear);
  EXPECT_EQ(FieldYear, *it);
  it = fields.find(FieldTrackNumber);
  EXPECT_EQ(FieldTrackNumber, *it);
  EXPECT_EQ((unsigned int)5, fields.size());
}
