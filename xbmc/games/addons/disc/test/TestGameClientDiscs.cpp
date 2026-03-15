/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscs, RemovedIndicesMustBeAppliedDescendingToPreserveOriginalSlots)
{
  const std::vector<int> coreSlots{0, 1, 2, 3, 4};
  std::vector<size_t> removedIndices{1, 3};

  auto removeByIndices = [](std::vector<int> slots, const std::vector<size_t>& indices)
  {
    for (const size_t index : indices)
      slots.erase(slots.begin() + static_cast<std::ptrdiff_t>(index));

    return slots;
  };

  const std::vector<int> ascendingResult = removeByIndices(coreSlots, removedIndices);
  EXPECT_EQ(ascendingResult, (std::vector<int>{0, 2, 3}));

  std::sort(removedIndices.rbegin(), removedIndices.rend());
  const std::vector<int> descendingResult = removeByIndices(coreSlots, removedIndices);
  EXPECT_EQ(descendingResult, (std::vector<int>{0, 2, 4}));
}
