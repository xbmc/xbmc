/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "gtest/gtest.h"
#include "music/tags/TagLoaderTagLib.h"

TEST(TestTagLoaderTagLib, SplitMBID)
{
  CTagLoaderTagLib lib;

  // SplitMBID should return the vector if it's empty or longer than 1
  std::vector<std::string> values;
  EXPECT_TRUE(lib.SplitMBID(values).empty());

  values.push_back("1");
  values.push_back("2");
  EXPECT_EQ(values, lib.SplitMBID(values));

  // length 1 and invalid should return empty
  values.clear();
  values.push_back("invalid");
  EXPECT_TRUE(lib.SplitMBID(values).empty());

  // length 1 and valid should return the valid id
  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_EQ(lib.SplitMBID(values), values);

  // case shouldn't matter
  values.clear();
  values.push_back("0383DaDf-2A4e-4d10-a46a-e9e041da8eb3");
  EXPECT_EQ(lib.SplitMBID(values).size(), 1u);
  EXPECT_STREQ(lib.SplitMBID(values)[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");

  // valid with some stuff off the end or start should return valid
  values.clear();
  values.push_back("foo0383dadf-2a4e-4d10-a46a-e9e041da8eb3 blah");
  EXPECT_EQ(lib.SplitMBID(values).size(), 1u);
  EXPECT_STREQ(lib.SplitMBID(values)[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");

  // two valid with various separators
  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3;53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");
  std::vector<std::string> result = lib.SplitMBID(values);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_STREQ(result[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_STREQ(result[1].c_str(), "53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");

  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3/53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");
  result = lib.SplitMBID(values);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_STREQ(result[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_STREQ(result[1].c_str(), "53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");

  values.clear();
  values.push_back("0383dadf-2a4e-4d10-a46a-e9e041da8eb3 / 53b106e7-0cc6-42cc-ac95-ed8d30a3a98e; ");
  result = lib.SplitMBID(values);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_STREQ(result[0].c_str(), "0383dadf-2a4e-4d10-a46a-e9e041da8eb3");
  EXPECT_STREQ(result[1].c_str(), "53b106e7-0cc6-42cc-ac95-ed8d30a3a98e");
}
