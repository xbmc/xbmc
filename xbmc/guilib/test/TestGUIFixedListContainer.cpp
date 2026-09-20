/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIFixedListContainer.h"
#include "guilib/GUIListItem.h"
#include "guilib/GUIListItemLayout.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

#include <gtest/gtest.h>

class TestGUIFixedListContainer : public CGUIFixedListContainer
{
public:
  TestGUIFixedListContainer(float left,
                            float width,
                            float screenStart,
                            float screenEnd,
                            float itemWidth,
                            float focusedItemWidth,
                            int focusPosition)
    : CGUIFixedListContainer(0,
                             1,
                             left,
                             0,
                             width,
                             100,
                             HORIZONTAL,
                             CScroller(0),
                             0,
                             focusPosition,
                             0,
                             0,
                             FixedListAlignY::CENTER),
      m_screenStart(screenStart),
      m_screenEnd(screenEnd)
  {
    m_layout = &m_layouts.emplace_back();
    m_layout->SetWidth(itemWidth);
    m_layout->SetHeight(100);

    m_focusedLayout = &m_focusedLayouts.emplace_back();
    m_focusedLayout->SetWidth(focusedItemWidth);
    m_focusedLayout->SetHeight(100);
  }

  bool CalculateScreenRange() override
  {
    CGUIBaseContainer::m_screenStart = m_screenStart;
    CGUIBaseContainer::m_screenEnd = m_screenEnd;
    CGUIBaseContainer::m_hasScreenRange = true;
    return true;
  }

  void AddItems(int count)
  {
    for (int i = 0; i < count; ++i)
      m_items.emplace_back(std::make_shared<CGUIListItem>(std::to_string(i)));
  }

  int GetPageSizeForTest() const { return GetPageSize(); }

  void PrepareForAction()
  {
    m_wasReset = true;
    CalculatePageSize();
  }

private:
  float m_screenStart;
  float m_screenEnd;
};

TEST(TestGUIFixedListContainer, PageActionsUseVisiblePageSizeFromFixedCursor)
{
  for (int focusPosition = 1; focusPosition <= 4; ++focusPosition)
  {
    TestGUIFixedListContainer container(-100, 2200, 0, 1920, 300, 400, focusPosition);
    container.AddItems(20);
    container.PrepareForAction();

    EXPECT_EQ(container.GetPageSizeForTest(), 5);
    EXPECT_EQ(container.GetSelectedItem(), focusPosition);

    EXPECT_TRUE(container.OnAction(CAction(ACTION_PAGE_DOWN)));
    EXPECT_EQ(container.GetSelectedItem(), focusPosition + 5);

    EXPECT_TRUE(container.OnAction(CAction(ACTION_PAGE_UP)));
    EXPECT_EQ(container.GetSelectedItem(), focusPosition);
  }
}

TEST(TestGUIFixedListContainer, PageActionsSkipPartiallyVisibleSlots)
{
  TestGUIFixedListContainer container(-100, 2000, 0, 1920, 400, 400, 1);
  container.AddItems(20);
  container.PrepareForAction();

  EXPECT_EQ(container.GetPageSizeForTest(), 4);
  EXPECT_EQ(container.GetSelectedItem(), 1);

  EXPECT_TRUE(container.OnAction(CAction(ACTION_PAGE_DOWN)));
  EXPECT_EQ(container.GetSelectedItem(), 5);
}
