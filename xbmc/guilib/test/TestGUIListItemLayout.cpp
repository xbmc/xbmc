/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroup.h"
#include "guilib/GUIListContainer.h"
#include "guilib/GUIListItem.h"
#include "guilib/GUIListItemLayout.h"
#include "guilib/GUIWindowManager.h"
#include "utils/XBMCTinyXML.h"
#include "windowing/WinSystem.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
using namespace std::string_literals;

class CTestGroup : public CGUIControlGroup
{
public:
  using CGUIControlLookup::GetLookup;
};

class CTestContainer : public CGUIListContainer
{
public:
  CTestContainer() : CGUIListContainer(0, 1, 0, 0, 100, 100, VERTICAL, CScroller(0), 0) {}
  explicit CTestContainer(const CTestContainer& other) : CGUIListContainer(other) {}

  using CGUIBaseContainer::Reset;

  void AddItem(const std::shared_ptr<CGUIListItem>& item)
  {
    m_items.emplace_back(item);
    ASSERT_FALSE(m_layouts.empty());
    ASSERT_FALSE(m_focusedLayouts.empty());
    item->SetLayout(std::make_unique<CGUIListItemLayout>(m_layouts.front(), this));
    item->SetFocusedLayout(std::make_unique<CGUIListItemLayout>(m_focusedLayouts.front(), this));
  }
};

class CGUITestComponent : public CGUIComponent
{
public:
  CGUITestComponent() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
  }
};

class CTestWinSystem : public CWinSystemBase
{
public:
  CTestWinSystem() : m_previousWinSystem(CServiceBroker::GetWinSystem())
  {
    CServiceBroker::RegisterWinSystem(this);
  }

  ~CTestWinSystem() override
  {
    if (m_previousWinSystem)
      CServiceBroker::RegisterWinSystem(m_previousWinSystem);
    else
      CServiceBroker::UnregisterWinSystem();
  }

  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override
  {
    return true;
  }
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override { return true; }
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override
  {
    return true;
  }
  void Register(IDispResource* resource) override {}
  void Unregister(IDispResource* resource) override {}

private:
  CWinSystemBase* m_previousWinSystem;
};

class TestGUIListItemLayout : public testing::Test
{
protected:
  void LoadLayouts(CTestContainer& container)
  {
    CXBMCTinyXML xml;
    ASSERT_TRUE(xml.Parse(R"(<control type="list">
      <itemlayout width="100" height="20">
        <control type="group" id="10">
          <control type="group" id="11"><control type="group" id="15"/></control>
          <control type="list" id="12">
            <width>100</width><height>20</height>
            <itemlayout width="100" height="20"><control type="group" id="13"/></itemlayout>
            <focusedlayout width="100" height="20"><control type="group" id="14"/></focusedlayout>
          </control>
        </control>
      </itemlayout>
      <focusedlayout width="100" height="20">
        <control type="group" id="20"><control type="group" id="21"/></control>
      </focusedlayout>
    </control>)"s));
    container.LoadLayout(xml.RootElement());
  }

private:
  CTestWinSystem m_winSystem;
  CGUITestComponent m_gui;
};
} // namespace

TEST_F(TestGUIListItemLayout, ResetReleasesRetainedItemLayouts)
{
  auto item = std::make_shared<CFileItem>();
  auto ancestor = std::make_unique<CTestGroup>();
  auto* container = new CTestContainer;
  LoadLayouts(*container);
  ancestor->AddControl(container);
  const auto initialLookup = ancestor->GetLookup();
  container->AddItem(item);
  ASSERT_NE(item->GetLayout(), nullptr);
  ASSERT_NE(item->GetFocusedLayout(), nullptr);
  CGUIControl* control = container;
  EXPECT_EQ(dynamic_cast<CGUIControlLookup*>(control), nullptr);
  EXPECT_EQ(ancestor->GetLookup(), initialLookup);

  container->Reset();

  EXPECT_EQ(item->GetLayout(), nullptr);
  EXPECT_EQ(item->GetFocusedLayout(), nullptr);
  EXPECT_EQ(ancestor->GetLookup(), initialLookup);
  ancestor.reset();
  item.reset();
}

TEST_F(TestGUIListItemLayout, LiveFreeMemoryPreservesContainerAncestorLookup)
{
  for (bool immediately : {false, true})
  {
    SCOPED_TRACE(immediately);
    CTestGroup ancestor;
    auto* container = new CTestContainer;
    LoadLayouts(*container);
    ancestor.AddControl(container);
    const auto initialLookup = ancestor.GetLookup();
    auto item = std::make_shared<CGUIListItem>();
    container->AddItem(item);
    EXPECT_EQ(ancestor.GetLookup(), initialLookup);

    item->FreeMemory(immediately);

    EXPECT_EQ(ancestor.GetLookup(), initialLookup);
    EXPECT_EQ(item->GetLayout(), nullptr);
    EXPECT_EQ(item->GetFocusedLayout(), nullptr);
  }
}

TEST_F(TestGUIListItemLayout, LiveFreeMemoryUnregistersDescendantsFromLookupParents)
{
  for (bool immediately : {false, true})
  {
    SCOPED_TRACE(immediately);
    CTestGroup ancestor;
    auto* parent = new CTestGroup;
    ancestor.AddControl(parent);
    CXBMCTinyXML xml;
    ASSERT_TRUE(xml.Parse(R"(<itemlayout width="100" height="20">
      <control type="group" id="10"/>
    </itemlayout>)"s));
    const auto createLayout = [&]
    {
      auto layout = std::make_unique<CGUIListItemLayout>();
      layout->SetParentControl(parent);
      layout->LoadLayout(xml.RootElement(), 0, false, 100, 100);
      return layout;
    };
    CGUIListItem retained;
    retained.SetLayout(createLayout());
    const auto retainedParentLookup = parent->GetLookup();
    const auto retainedAncestorLookup = ancestor.GetLookup();
    ASSERT_EQ(retainedParentLookup.count(10), 1);
    ASSERT_EQ(retainedAncestorLookup.count(10), 1);
    CGUIListItem item;
    item.SetLayout(createLayout());
    item.SetFocusedLayout(createLayout());
    ASSERT_EQ(parent->GetLookup().count(10), 3);
    ASSERT_EQ(ancestor.GetLookup().count(10), 3);

    item.FreeMemory(immediately);

    EXPECT_EQ(item.GetLayout(), nullptr);
    EXPECT_EQ(item.GetFocusedLayout(), nullptr);
    EXPECT_EQ(parent->GetLookup(), retainedParentLookup);
    EXPECT_EQ(ancestor.GetLookup(), retainedAncestorLookup);
    retained.FreeMemory(immediately);
    EXPECT_TRUE(parent->GetLookup().empty());
    EXPECT_TRUE(ancestor.GetLookup().empty());
  }
}

TEST_F(TestGUIListItemLayout, ContainerCopyDoesNotRegisterWithSourceAncestor)
{
  CTestGroup sourceAncestor;
  auto* source = new CTestContainer;
  LoadLayouts(*source);
  sourceAncestor.AddControl(source);
  source->AddItem(std::make_shared<CGUIListItem>());
  const auto sourceLookup = sourceAncestor.GetLookup();
  auto copy = std::make_unique<CTestContainer>(*source);
  EXPECT_EQ(sourceAncestor.GetLookup(), sourceLookup);
  CTestGroup destinationAncestor;
  auto* destination = copy.get();
  destinationAncestor.AddControl(copy.release());
  const auto destinationLookup = destinationAncestor.GetLookup();
  auto item = std::make_shared<CGUIListItem>();
  destination->AddItem(item);
  ASSERT_NE(item->GetLayout(), nullptr);
  ASSERT_NE(item->GetFocusedLayout(), nullptr);
  EXPECT_EQ(destinationAncestor.GetLookup(), destinationLookup);
  EXPECT_EQ(sourceAncestor.GetLookup(), sourceLookup);

  item->FreeMemory();

  EXPECT_EQ(destinationAncestor.GetLookup(), destinationLookup);
  EXPECT_EQ(sourceAncestor.GetLookup(), sourceLookup);
  destinationAncestor.ClearAll();
  EXPECT_TRUE(destinationAncestor.GetLookup().empty());
  EXPECT_EQ(item->GetLayout(), nullptr);
  EXPECT_EQ(item->GetFocusedLayout(), nullptr);
  EXPECT_EQ(sourceAncestor.GetLookup(), sourceLookup);
}
