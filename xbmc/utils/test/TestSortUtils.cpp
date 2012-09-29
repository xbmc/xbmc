/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/SortUtils.h"
#include "utils/Variant.h"

#include "gtest/gtest.h"

TEST(TestSortUtils, Sort_SortBy)
{
  SortItems items;

  CVariant variant1("M Artist");
  SortItem item1;
  item1[FieldArtist] = variant1;
  CVariant variant2("B Artist");
  SortItem item2;
  item2[FieldArtist] = variant2;
  CVariant variant3("R Artist");
  SortItem item3;
  item3[FieldArtist] = variant3;
  CVariant variant4("R Artist");
  SortItem item4;
  item4[FieldArtist] = variant4;
  CVariant variant5("I Artist");
  SortItem item5;
  item5[FieldArtist] = variant5;
  CVariant variant6("A Artist");
  SortItem item6;
  item6[FieldArtist] = variant6;
  CVariant variant7("G Artist");
  SortItem item7;
  item7[FieldArtist] = variant7;

  items.push_back(item1);
  items.push_back(item2);
  items.push_back(item3);
  items.push_back(item4);
  items.push_back(item5);
  items.push_back(item6);
  items.push_back(item7);

  SortUtils::Sort(SortByArtist, SortOrderAscending, SortAttributeNone, items);

  EXPECT_STREQ("A Artist", items.at(0)[FieldArtist].asString().c_str());
  EXPECT_STREQ("B Artist", items.at(1)[FieldArtist].asString().c_str());
  EXPECT_STREQ("G Artist", items.at(2)[FieldArtist].asString().c_str());
  EXPECT_STREQ("I Artist", items.at(3)[FieldArtist].asString().c_str());
  EXPECT_STREQ("M Artist", items.at(4)[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", items.at(5)[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", items.at(6)[FieldArtist].asString().c_str());
}

TEST(TestSortUtils, Sort_SortDescription)
{
  SortItems items;

  CVariant variant1("M Artist");
  SortItem item1;
  item1[FieldArtist] = variant1;
  CVariant variant2("B Artist");
  SortItem item2;
  item2[FieldArtist] = variant2;
  CVariant variant3("R Artist");
  SortItem item3;
  item3[FieldArtist] = variant3;
  CVariant variant4("R Artist");
  SortItem item4;
  item4[FieldArtist] = variant4;
  CVariant variant5("I Artist");
  SortItem item5;
  item5[FieldArtist] = variant5;
  CVariant variant6("A Artist");
  SortItem item6;
  item6[FieldArtist] = variant6;
  CVariant variant7("G Artist");
  SortItem item7;
  item7[FieldArtist] = variant7;

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

  EXPECT_STREQ("A Artist", items.at(0)[FieldArtist].asString().c_str());
  EXPECT_STREQ("B Artist", items.at(1)[FieldArtist].asString().c_str());
  EXPECT_STREQ("G Artist", items.at(2)[FieldArtist].asString().c_str());
  EXPECT_STREQ("I Artist", items.at(3)[FieldArtist].asString().c_str());
  EXPECT_STREQ("M Artist", items.at(4)[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", items.at(5)[FieldArtist].asString().c_str());
  EXPECT_STREQ("R Artist", items.at(6)[FieldArtist].asString().c_str());
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
  it = fields.find(FieldYear);
  EXPECT_EQ(FieldYear, *it);
  it = fields.find(FieldTrackNumber);
  EXPECT_EQ(FieldTrackNumber, *it);
  EXPECT_EQ((unsigned int)4, fields.size());
}
