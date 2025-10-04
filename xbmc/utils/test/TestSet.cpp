/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/Set.h"

#include <gtest/gtest.h>

TEST(TestSet, Lookup)
{
  static constexpr CSet someNumbers{4, 2, 3, 1};

  // compile time tests
  static_assert(someNumbers.contains(1));
  static_assert(someNumbers.contains(2));
  static_assert(someNumbers.contains(3));
  static_assert(someNumbers.contains(4));
  static_assert(!someNumbers.contains(0));
  static_assert(!someNumbers.contains(5));

  // runtime tests
  EXPECT_TRUE(someNumbers.contains(1));
  EXPECT_TRUE(someNumbers.contains(2));
  EXPECT_TRUE(someNumbers.contains(3));
  EXPECT_TRUE(someNumbers.contains(4));
  EXPECT_FALSE(someNumbers.contains(0));
  EXPECT_FALSE(someNumbers.contains(5));
}

TEST(TestSet, IsSorted)
{
  static constexpr CSet someNumbers{4, 2, 3, 1};
  static constexpr std::array sortedNumbers{1, 2, 3, 4};

  // compile time check
  static_assert(std::ranges::equal(someNumbers, sortedNumbers));

  // runtime check
  EXPECT_TRUE(std::ranges::equal(someNumbers, sortedNumbers));
}
