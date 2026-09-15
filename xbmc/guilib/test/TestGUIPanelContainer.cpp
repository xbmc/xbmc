/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIListItem.h"
#include "guilib/GUIListItemLayout.h"
#include "guilib/GUIPanelContainer.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

class TestGUIPanelContainer : public CGUIPanelContainer
{
public:
  TestGUIPanelContainer(
      float height, float screenStart, float screenEnd, float itemHeight, float focusedItemHeight)
    : CGUIPanelContainer(0, 1, 0, 0, 800, height, VERTICAL, CScroller(0), 0),
      m_screenStart(screenStart),
      m_screenEnd(screenEnd)
  {
    m_layout = &m_layouts.emplace_back();
    m_layout->SetWidth(800);
    m_layout->SetHeight(itemHeight);

    m_focusedLayout = &m_focusedLayouts.emplace_back();
    m_focusedLayout->SetWidth(800);
    m_focusedLayout->SetHeight(focusedItemHeight);
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
    CalculateLayout();
  }

private:
  float m_screenStart;
  float m_screenEnd;
};

TEST(TestGUIPanelContainer, CalculateLayoutUpdatesPageNavigationSize)
{
  TestGUIPanelContainer container(960, 0, 960, 80, 120);
  container.AddItems(30);
  container.PrepareForAction();

  EXPECT_EQ(container.GetPageSizeForTest(), 12);
}
