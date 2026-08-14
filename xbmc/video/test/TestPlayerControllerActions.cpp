/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/PlayerControllerActions.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO;

namespace
{
struct SubtitleTrackTestParam
{
  int count;
  int index;
  bool visible;
  SubtitleTrackAction action;
  SubtitleTrackResult expected;
};

const SubtitleTrackTestParam SubtitleTrackTests[] = {
    // single stream
    // prev/next make visible
    {1, 0, true, SubtitleTrackAction::NEXT, {0, false}},
    {1, 0, true, SubtitleTrackAction::PREV, {0, false}},

    // currently hidden
    {1, 0, false, SubtitleTrackAction::NEXT, {0, true}},
    {1, 0, false, SubtitleTrackAction::PREV, {0, true}},

    // 3 streams
    // next advances a visible track then hides then shows first again
    {3, 0, true, SubtitleTrackAction::NEXT, {1, true}},
    {3, 1, true, SubtitleTrackAction::NEXT, {2, true}},
    {3, 2, true, SubtitleTrackAction::NEXT, {0, false}},
    {3, 0, false, SubtitleTrackAction::NEXT, {0, true}},

    // previous goes back then hides then shows last again
    {3, 2, true, SubtitleTrackAction::PREV, {1, true}},
    {3, 1, true, SubtitleTrackAction::PREV, {0, true}},
    {3, 0, true, SubtitleTrackAction::PREV, {0, false}},
    {3, 0, false, SubtitleTrackAction::PREV, {2, true}},

    // cycle advances and returns to first
    {3, 0, true, SubtitleTrackAction::CYCLE, {1, true}},
    {3, 1, true, SubtitleTrackAction::CYCLE, {2, true}},
    {3, 2, true, SubtitleTrackAction::CYCLE, {0, true}},

    // prev and next make visible
    {3, 1, false, SubtitleTrackAction::NEXT, {1, true}},
    {3, 1, false, SubtitleTrackAction::PREV, {1, true}},

    // corner case: no stream is a no-op
    {0, 0, false, SubtitleTrackAction::NEXT, {0, false}},
    {0, 0, true, SubtitleTrackAction::NEXT, {0, true}},
    {0, 0, false, SubtitleTrackAction::PREV, {0, false}},
    {0, 0, true, SubtitleTrackAction::PREV, {0, true}},
    {0, 0, false, SubtitleTrackAction::CYCLE, {0, false}},
    {0, 0, true, SubtitleTrackAction::CYCLE, {0, true}},

    // corner case: ouf of bounds index is a no-op
    {1, 1, false, SubtitleTrackAction::NEXT, {1, false}},
    {1, 1, true, SubtitleTrackAction::NEXT, {1, true}},
    {1, 1, false, SubtitleTrackAction::PREV, {1, false}},
    {1, 1, true, SubtitleTrackAction::PREV, {1, true}},
    {1, 1, false, SubtitleTrackAction::CYCLE, {1, false}},
    {1, 1, true, SubtitleTrackAction::CYCLE, {1, true}},
};
} // namespace

class SubtitleTrackTest : public testing::Test,
                          public testing::WithParamInterface<SubtitleTrackTestParam>
{
};

TEST_P(SubtitleTrackTest, Navigate)
{
  const auto& params = GetParam();
  auto [index, visible] = CPlayerControllerActions::ExecSubtitleTrackAction(
      params.count, params.index, params.visible, params.action);
  EXPECT_EQ(index, params.expected.newIndex);
  EXPECT_EQ(visible, params.expected.newVisible);
}

INSTANTIATE_TEST_SUITE_P(TestPlayerControllerActions,
                         SubtitleTrackTest,
                         testing::ValuesIn(SubtitleTrackTests));
