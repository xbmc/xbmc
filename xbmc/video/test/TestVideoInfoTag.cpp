/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/XBMCTinyXML.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <string>

#include <gtest/gtest.h>

TEST(TestVideoInfoTag, SetUniqueIDs)
{
  // initial state: no default, empty list.
  CVideoInfoTag details;
  std::map<std::string, std::string, std::less<>> reference = {};

  EXPECT_EQ(details.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // usual flow: initialize from initial state with a list.
  // entries with blank type or uniqueid are ignored
  std::map<std::string, std::string, std::less<>> test = {
      {"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}, {"", "123456"}, {"foo", ""}};
  reference = {{"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}};

  details.SetUniqueIDs(test);
  details.SetUniqueID("64043", "tmdb", true);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // current update behavior, not sure why:
  // the former default type and value from the previous list of uniqueids is added back when
  // omitted from the new list - instead of reverting to "unknown" default and setting the list as is.
  test = {{"imdb", "tt4577466"}, {"tvdb", "299350"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // setting a blank list clears all except the previous default
  test = {};
  reference = {{"tmdb", "64043"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // except when there is no explicit default, then setting a blank list clears the list.
  CVideoInfoTag details2;
  details2.SetUniqueIDs(reference);
  details2.SetUniqueIDs(test);

  EXPECT_EQ(details2.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details2.GetUniqueIDs(), test);
}
