/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIControlGroupList.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"

#include <gtest/gtest.h>

namespace
{
class CTestGroupList : public CGUIControlGroupList
{
public:
  // 10 rows of 40 in a list 200 high, so the maximum offset is 200
  CTestGroupList()
    : CGUIControlGroupList(0, 1, 0, 0, 100, 200, 0, 0, VERTICAL, false, 0, CScroller(200))
  {
    // CGUIControlGroupList::AddControl sets up navigation, which needs the GUI component
    for (int i = 0; i < 10; ++i)
      CGUIControlGroup::AddControl(new CGUIControlGroup(0, 10 + i, 0, 0, 100, 40));
    ValidateOffset();
  }

  EVENT_RESULT Wheel(int actionId)
  {
    return OnMouseEvent(CPoint(), KODI::MOUSE::CMouseEvent(actionId));
  }

  // each tick is followed by two frames 30 ms apart, so the next tick lands mid-scroll
  void WheelTicks(int actionId, int count)
  {
    for (int i = 0; i < count; ++i)
    {
      Wheel(actionId);
      m_scroller.Update(m_time);
      m_scroller.Update(m_time + 30);
      m_time += 31;
    }
  }

  void FinishScrolling()
  {
    m_scroller.Update(m_time);
    m_scroller.Update(m_time + 1000);
  }

private:
  unsigned int m_time = 1;
};
} // namespace

TEST(TestGUIControlGroupList, WheelDownTicksAccumulate)
{
  CTestGroupList list;
  list.WheelTicks(ACTION_MOUSE_WHEEL_DOWN, 3);
  list.FinishScrolling();
  EXPECT_FLOAT_EQ(list.GetScrollOffset(), 120.0f);
}

TEST(TestGUIControlGroupList, WheelUpTicksAccumulate)
{
  CTestGroupList list;
  list.SetScrollOffset(200);
  list.WheelTicks(ACTION_MOUSE_WHEEL_UP, 3);
  list.FinishScrolling();
  EXPECT_FLOAT_EQ(list.GetScrollOffset(), 80.0f);
}

TEST(TestGUIControlGroupList, WheelDownClampsAtEnd)
{
  CTestGroupList list;
  for (int i = 0; i < 5; ++i)
    EXPECT_EQ(list.Wheel(ACTION_MOUSE_WHEEL_DOWN), EVENT_RESULT_HANDLED);
  for (int i = 0; i < 3; ++i)
    EXPECT_EQ(list.Wheel(ACTION_MOUSE_WHEEL_DOWN), EVENT_RESULT_UNHANDLED);
  list.FinishScrolling();
  EXPECT_FLOAT_EQ(list.GetScrollOffset(), 200.0f);
}
