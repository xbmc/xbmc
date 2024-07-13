/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/Map.h"

#include <algorithm>

#include <gtest/gtest.h>

// CMap with sortable keys. Keys are sorted at compile time and find uses binary search
TEST(TestMap, SortableKey)
{
  constexpr auto map = make_map<int, std::string_view>({{3, "three"}, {1, "one"}, {2, "two"}});
  EXPECT_EQ(map.find(1)->second, "one");
  EXPECT_EQ(map.find(2)->second, "two");
  EXPECT_EQ(map.find(3)->second, "three");
  EXPECT_EQ(map.find(0), map.cend());
  EXPECT_EQ(map.find(4), map.cend());
  // Ensure content is sorted
  EXPECT_TRUE(
      std::is_sorted(map.cbegin(), map.cend(), [](auto& a, auto& b) { return a.first < b.first; }));
}

// CMap with non sortable keys. Performs linear search in find
TEST(TestMap, NonSortableKey)
{
  struct Dummy
  {
    int i;
    bool operator==(const Dummy& other) const { return i == other.i; }
  };

  constexpr auto map = make_map<Dummy, std::string_view>(
      {{Dummy{3}, "three"}, {Dummy{1}, "one"}, {Dummy{2}, "two"}});
  EXPECT_EQ(map.find(Dummy{1})->second, "one");
  EXPECT_EQ(map.find(Dummy{2})->second, "two");
  EXPECT_EQ(map.find(Dummy{3})->second, "three");
  EXPECT_EQ(map.find(Dummy{0}), map.cend());
  EXPECT_EQ(map.find(Dummy{4}), map.cend());
}

// CMap compile time tests (not really a unit test...)
TEST(TestMap, EnumConstexpr)
{
  enum class ENUM
  {
    ONE,
    TWO,
    THREE,
    ENUM_MAX,
  };

  constexpr auto map = make_map<ENUM, std::string_view>(
      {{ENUM::ONE, "ONE"}, {ENUM::TWO, "TWO"}, {ENUM::THREE, "THREE"}});
  static_assert(map.find(ENUM::ONE)->second == "ONE");
  static_assert(map.find(ENUM::TWO)->second == "TWO");
  static_assert(map.find(ENUM::THREE)->second == "THREE");
  static_assert(map.find(ENUM::ENUM_MAX) == map.cend());
  static_assert(map.size() == size_t(ENUM::ENUM_MAX));
}
