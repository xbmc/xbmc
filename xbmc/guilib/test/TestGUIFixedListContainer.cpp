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
#include "guilib/GUIMessage.h"
#include "guilib/GUIMessageIDs.h"
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
  int GetOffsetForTest() const { return GetOffset(); }
  int GetCursorForTest() const { return GetCursor(); }

  void SetScreenRangeForTest(float screenStart, float screenEnd)
  {
    m_screenStart = screenStart;
    m_screenEnd = screenEnd;
  }

  void RecalculateLayoutForTest() { CalculateLayout(); }
  void RefreshPageSizeForTest() { CalculatePageSize(false); }
  void ValidateOffsetForTest() { ValidateOffset(); }
  void SetItemWidthForTest(float width) { m_layout->SetWidth(width); }

  void SetItemLayoutCopyWidthForTest(float width)
  {
    CGUIListItemLayout itemLayout(*m_layout, this);
    itemLayout.SetWidth(width);
  }

  bool IsInvalidatedForTest() const { return m_bInvalidated; }

  void ProcessLayoutForTest()
  {
    if (m_bInvalidated)
      UpdateLayout();
    m_bInvalidated = false;
  }

  void PrepareForAction()
  {
    m_wasReset = true;
    CalculatePageSize();
  }

  void SelectItemForTest(int item) { SelectItem(item); }
  void SetPageControlForTest(int pageControl) { m_pageControl = pageControl; }
  void SetPageControlOffsetForTest(int offset) { m_lastPageControlOffset = offset; }

  bool SendPageChangeForTest(int offset)
  {
    CGUIMessage message(GUI_MSG_PAGE_CHANGE, m_pageControl, GetID(), offset);
    return OnMessage(message);
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

TEST(TestGUIFixedListContainer, PageSizeUsesProcessedParentCoordinates)
{
  TestGUIFixedListContainer container(-716, 1860, 0, 1920, 222, 222, 2);
  container.AddItems(20);
  container.PrepareForAction();

  EXPECT_EQ(container.GetPageSizeForTest(), 4);

  container.SetScreenRangeForTest(-788, 1132);
  container.RecalculateLayoutForTest();

  EXPECT_EQ(container.GetPageSizeForTest(), 8);
}

TEST(TestGUIFixedListContainer, PageSizeUpdatesAfterOpeningAnimation)
{
  TestGUIFixedListContainer container(0, 2112, -320, 1600, 317, 490, 1);
  container.AddItems(20);
  container.PrepareForAction();

  EXPECT_EQ(container.GetPageSizeForTest(), 4);

  container.SetScreenRangeForTest(0, 1920);
  container.RefreshPageSizeForTest();

  EXPECT_EQ(container.GetPageSizeForTest(), 5);
}

TEST(TestGUIFixedListContainer, PageSizeUpdatesAfterContainerResize)
{
  TestGUIFixedListContainer container(0, 1000, 0, 1000, 200, 200, 1);
  container.AddItems(20);
  container.RecalculateLayoutForTest();

  EXPECT_EQ(container.GetPageSizeForTest(), 5);

  container.SetWidth(1200);
  container.SetScreenRangeForTest(0, 1200);
  container.RecalculateLayoutForTest();

  EXPECT_EQ(container.GetPageSizeForTest(), 6);
}

TEST(TestGUIFixedListContainer, PageSizeUpdatesAfterCursorClamp)
{
  TestGUIFixedListContainer container(0, 200, 0, 100, 50, 100, 2);
  container.AddItems(20);
  container.RecalculateLayoutForTest();

  EXPECT_EQ(container.GetCursorForTest(), 2);

  container.SetWidth(150);
  container.RecalculateLayoutForTest();

  EXPECT_EQ(container.GetPageSizeForTest(), 2);

  container.ValidateOffsetForTest();
  container.RefreshPageSizeForTest();

  EXPECT_EQ(container.GetCursorForTest(), 1);
  EXPECT_EQ(container.GetPageSizeForTest(), 1);
}

TEST(TestGUIFixedListContainer, ActiveLayoutResizeInvalidatesPageSize)
{
  TestGUIFixedListContainer container(0, 1000, 0, 1000, 200, 200, 1);
  container.AddItems(20);
  container.ProcessLayoutForTest();

  EXPECT_EQ(container.GetPageSizeForTest(), 5);
  EXPECT_FALSE(container.IsInvalidatedForTest());

  container.SetItemWidthForTest(100);

  EXPECT_TRUE(container.IsInvalidatedForTest());

  container.ProcessLayoutForTest();

  EXPECT_EQ(container.GetPageSizeForTest(), 9);
  EXPECT_FALSE(container.IsInvalidatedForTest());

  container.SetItemLayoutCopyWidthForTest(50);

  EXPECT_FALSE(container.IsInvalidatedForTest());
}

TEST(TestGUIFixedListContainer, PageControlPreservesStartBoundaryOffset)
{
  TestGUIFixedListContainer container(0, 1000, 0, 1000, 100, 100, 5);
  container.AddItems(20);
  container.PrepareForAction();
  container.SetPageControlForTest(10);
  container.SelectItemForTest(0);

  EXPECT_EQ(container.GetPageSizeForTest(), 10);
  EXPECT_EQ(container.GetCursorForTest(), 5);
  EXPECT_EQ(container.GetOffsetForTest(), -5);

  EXPECT_TRUE(container.SendPageChangeForTest(10));
  EXPECT_EQ(container.GetSelectedItem(), 10);
}

TEST(TestGUIFixedListContainer, PageControlPreservesEndBoundaryOffset)
{
  TestGUIFixedListContainer container(0, 1000, 0, 1000, 100, 100, 5);
  container.AddItems(20);
  container.PrepareForAction();
  container.SetPageControlForTest(10);
  container.SelectItemForTest(19);

  EXPECT_EQ(container.GetPageSizeForTest(), 10);
  EXPECT_EQ(container.GetCursorForTest(), 5);
  EXPECT_EQ(container.GetOffsetForTest(), 14);

  EXPECT_TRUE(container.SendPageChangeForTest(0));
  EXPECT_EQ(container.GetSelectedItem(), 9);
}

TEST(TestGUIFixedListContainer, PageControlUsesPreviousControlOffsetForRepeatedPaging)
{
  TestGUIFixedListContainer container(0, 1000, 0, 1000, 100, 100, 5);
  container.AddItems(40);
  container.PrepareForAction();
  container.SetPageControlForTest(10);
  container.SelectItemForTest(0);
  container.SetPageControlOffsetForTest(0);

  EXPECT_TRUE(container.SendPageChangeForTest(10));
  EXPECT_EQ(container.GetSelectedItem(), 10);
  EXPECT_EQ(container.GetOffsetForTest(), 5);

  EXPECT_TRUE(container.SendPageChangeForTest(20));
  EXPECT_EQ(container.GetSelectedItem(), 20);
}
