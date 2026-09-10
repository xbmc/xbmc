/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "games/dialogs/osd/DialogGameCheats.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIScrollBarControl.h"
#include "guilib/GUITexture.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "windowing/WinSystem.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
constexpr int LIST = 1083901;
constexpr int BUTTON_TEMPLATE = 1083903;
constexpr int RADIO_TEMPLATE = 1083904;
constexpr int CLOSE = 1083905;
constexpr int SCROLLBAR = 1083906;
constexpr int FIRST_CHEAT = 1083909;

// Empty textures let the real button controls run without a renderer.
class CTestTexture : public CGUITexture
{
public:
  CTestTexture(float x, float y, float width, float height, const CTextureInfo& texture)
    : CGUITexture(x, y, width, height, texture)
  {
  }
  CTestTexture* Clone() const override { return new CTestTexture(*this); }
  void Begin(KODI::UTILS::COLOR::Color) override {}
  void Draw(float*, float*, float*, const CRect&, const CRect&, int) override {}
  void End() override {}
};

class CTestWinSystem : public CWinSystemBase
{
public:
  bool CreateNewWindow(const std::string&, bool, RESOLUTION_INFO&) override { return true; }
  bool ResizeWindow(int, int, int, int) override { return true; }
  bool SetFullScreen(bool, RESOLUTION_INFO&, bool) override { return true; }
  void Register(IDispResource*) override {}
  void Unregister(IDispResource*) override {}
};

class CTestGUI : public CGUIComponent
{
public:
  CTestGUI() : CGUIComponent(false)
  {
    CServiceBroker::RegisterWinSystem(&m_winSystem);
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
    CGUITexture::Register(
        [](float x, float y, float width, float height, const CTextureInfo& texture)
        { return new CTestTexture(x, y, width, height, texture); }, {});
  }
  ~CTestGUI() override
  {
    m_pWindowManager.reset();
    CGUITexture::Register({}, {});
    if (m_previousWinSystem)
      CServiceBroker::RegisterWinSystem(m_previousWinSystem);
    else
      CServiceBroker::UnregisterWinSystem();
  }

private:
  CWinSystemBase* m_previousWinSystem{CServiceBroker::GetWinSystem()};
  CTestWinSystem m_winSystem;
};

class CTestCheatsList : public CGUIControlGroupList
{
public:
  using CGUIControlGroupList::CGUIControlGroupList;
  void FinishScroll()
  {
    AdvanceScroll(1);
    AdvanceScroll(m_scroller.GetDuration());
  }
  void AdvanceScroll(unsigned int elapsed)
  {
    m_time += elapsed;
    m_scroller.Update(m_time);
  }
  bool IsOnScreen(int id)
  {
    const auto* control = GetControl(id);
    return control && IsControlOnScreen(GetControlOffset(control), control);
  }
  float ScrollOffset() const { return m_scroller.GetValue(); }
  int ItemCount() const { return GetNumItems(); }

private:
  unsigned int m_time{0};
};

class CTestDialogGameCheats : public KODI::GAME::CDialogGameCheats, public IMsgTargetCallback
{
public:
  explicit CTestDialogGameCheats(bool showOnePage = true)
  {
    auto* list = new CTestCheatsList(GetID(), LIST, 0, 0, 1160, 770, 0, SCROLLBAR, VERTICAL, false,
                                     0, CScroller(200));
    list->SetAction(ACTION_MOVE_UP, CGUIAction(LIST));
    list->SetAction(ACTION_MOVE_DOWN, CGUIAction(LIST));
    list->SetAction(ACTION_MOVE_LEFT, CGUIAction(CLOSE));
    list->SetAction(ACTION_MOVE_RIGHT, CGUIAction(SCROLLBAR));
    AddControl(list);
    const CTextureInfo texture;
    const CLabelInfo label;
    AddControl(new CGUIRadioButtonControl(GetID(), RADIO_TEMPLATE, 0, 0, 1160, 70, texture, texture,
                                          label, texture, texture, texture, texture, texture,
                                          texture));
    for (int id : {BUTTON_TEMPLATE, CLOSE})
      AddControl(new CGUIButtonControl(GetID(), id, 0, 0, 1160, 70, texture, texture, label));
    AddControl(new GUIScrollBarControl(GetID(), SCROLLBAR, 0, 0, 20, 770, texture, texture, texture,
                                       texture, texture, VERTICAL, showOnePage));
    GetControl(SCROLLBAR)->SetAction(ACTION_MOVE_LEFT, CGUIAction(LIST));
    GetControl(SCROLLBAR)->SetAction(ACTION_MOVE_RIGHT, CGUIAction(CLOSE));
    GetControl(CLOSE)->SetAction(ACTION_MOVE_LEFT, CGUIAction(SCROLLBAR));
    GetControl(CLOSE)->SetAction(ACTION_MOVE_RIGHT, CGUIAction(LIST));
    SetDefaultControl(3, true);
    CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(this);
  }

  ~CTestDialogGameCheats() override
  {
    CServiceBroker::GetGUI()->GetWindowManager().RemoveMsgTarget(this);
  }

  bool OnMessage(CGUIMessage& message) override { return CDialogGameCheats::OnMessage(message); }

  void OpenForTest()
  {
    m_active = true;
    InitializeControls();
    Focus(m_defaultControl);
  }

  void Focus(int id)
  {
    CGUIMessage message(GUI_MSG_SETFOCUS, GetID(), id);
    OnMessage(message);
  }

  void Refresh()
  {
    CGUIMessage message(GUI_MSG_UPDATE, GetID(), -1);
    OnMessage(message);
  }

  CTestCheatsList& List() { return *static_cast<CTestCheatsList*>(GetControl(LIST)); }
  void ScrollPage(int action)
  {
    auto* scrollbar = static_cast<GUIScrollBarControl*>(GetControl(SCROLLBAR));
    scrollbar->SetRange(static_cast<int>(List().Size()), static_cast<int>(List().GetTotalSize()));
    scrollbar->SetValue(static_cast<int>(List().ScrollOffset()));
    ASSERT_TRUE(scrollbar->OnAction(CAction(action)));
  }
  void MarkClosing() { m_closing = true; }
  void MarkClosed() { m_active = false; }
  int cheatCount{331};
  bool getMore{false};

protected:
  void InitializeControls() override
  {
    std::vector<KODI::GAME::Cheat> cheats(cheatCount);
    for (int i = 0; i < cheatCount; ++i)
      cheats[i].description = "Cheat " + std::to_string(i);
    CreateControls(std::move(cheats), getMore);
  }
};

class TestDialogGameCheats : public testing::Test
{
protected:
  CTestGUI m_gui;
};
} // namespace

TEST_F(TestDialogGameCheats, LongListKeepsAllRowsAndNavigationDistinctFromSkinControls)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  ASSERT_EQ(dialog.List().ItemCount(), 331);
  EXPECT_FALSE(dialog.GetControl(BUTTON_TEMPLATE)->CanFocus());
  EXPECT_FALSE(dialog.GetControl(RADIO_TEMPLATE)->CanFocus());

  // Visit every generated row, including the old ID 0 boundary and static-ID collisions.
  for (int i = 0; i < 331; ++i)
  {
    EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + i);
    ASSERT_TRUE(dialog.GetControl(FIRST_CHEAT + i)->CanFocus());
    EXPECT_TRUE(dialog.OnMove(dialog.GetFocusedControlID(), ACTION_MOVE_DOWN));
  }
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT);
  EXPECT_TRUE(dialog.OnMove(dialog.GetFocusedControlID(), ACTION_MOVE_UP));
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 330);
}

TEST_F(TestDialogGameCheats, HorizontalNavigationReturnsToTheSelectedRow)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  const int row = FIRST_CHEAT + 240;
  dialog.Focus(row);
  dialog.List().FinishScroll();
  const float offset = dialog.List().ScrollOffset();
  ASSERT_GT(offset, 0);

  EXPECT_TRUE(dialog.OnMove(row, ACTION_MOVE_RIGHT));
  EXPECT_EQ(dialog.GetFocusedControlID(), SCROLLBAR);
  EXPECT_TRUE(dialog.OnMove(SCROLLBAR, ACTION_MOVE_RIGHT));
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
  EXPECT_TRUE(dialog.OnMove(CLOSE, ACTION_MOVE_RIGHT));
  EXPECT_EQ(dialog.GetFocusedControlID(), row);
  dialog.List().FinishScroll();
  EXPECT_EQ(dialog.List().ScrollOffset(), offset);
}

TEST_F(TestDialogGameCheats, HorizontalReturnDuringScrollingKeepsTheSelectedRow)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  const int row = FIRST_CHEAT + 10;
  dialog.Focus(row);

  EXPECT_TRUE(dialog.OnMove(row, ACTION_MOVE_LEFT));
  EXPECT_TRUE(dialog.OnMove(CLOSE, ACTION_MOVE_RIGHT));
  EXPECT_EQ(dialog.GetFocusedControlID(), row);
}

TEST_F(TestDialogGameCheats, ScrollbarMovementAllowsFocusToFollowTheNewPage)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.Focus(SCROLLBAR);

  dialog.ScrollPage(ACTION_MOVE_DOWN);
  dialog.List().FinishScroll();
  EXPECT_TRUE(dialog.OnMove(SCROLLBAR, ACTION_MOVE_LEFT));
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 241);
}

TEST_F(TestDialogGameCheats, EmptyAndGetMoreOnlyDialogsHaveActionableInitialFocus)
{
  CTestDialogGameCheats dialog;
  dialog.cheatCount = 0;
  dialog.OpenForTest();
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);

  dialog.getMore = true;
  dialog.OpenForTest();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT);
  EXPECT_EQ(dialog.List().ItemCount(), 1);
}

TEST_F(TestDialogGameCheats, RefreshPreservesRowsAndCloseAndHandlesRemovedGetMore)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 240);
  dialog.Focus(SCROLLBAR);
  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), SCROLLBAR);
  dialog.Focus(CLOSE);
  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);

  dialog.cheatCount = 0;
  dialog.getMore = true;
  dialog.OpenForTest();
  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT);
  dialog.getMore = false;
  dialog.cheatCount = 331;
  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT);
  dialog.cheatCount = 0;
  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
}

TEST_F(TestDialogGameCheats, RefreshAfterClosingDoesNotRebuildOrReactivateTheDialog)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  const float offset = dialog.List().ScrollOffset();
  dialog.MarkClosed();
  dialog.cheatCount = 0;
  dialog.Refresh();
  EXPECT_FALSE(dialog.IsDialogRunning());
  EXPECT_EQ(dialog.List().ItemCount(), 331);
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 240);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.OpenForTest();
  EXPECT_EQ(dialog.List().ItemCount(), 0);
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 0);
}

TEST_F(TestDialogGameCheats, RefreshPreservesMiddleOfPageRowAndOffset)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  for (int i = 0; i < 4; ++i)
    ASSERT_TRUE(dialog.OnMove(dialog.GetFocusedControlID(), ACTION_MOVE_UP));
  const float offset = dialog.List().ScrollOffset();

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 236);
  EXPECT_TRUE(dialog.GetControl(FIRST_CHEAT + 236)->HasFocus());
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  EXPECT_TRUE(dialog.List().IsOnScreen(FIRST_CHEAT + 236));
}

TEST_F(TestDialogGameCheats, RefreshPreservesCloseFocusAndRememberedRowWithoutScrolling)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.Focus(FIRST_CHEAT + 236);
  const float offset = dialog.List().ScrollOffset();
  dialog.Focus(CLOSE);

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
  EXPECT_TRUE(dialog.GetControl(CLOSE)->HasFocus());
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  ASSERT_TRUE(dialog.OnMove(CLOSE, ACTION_MOVE_RIGHT));
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 236);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
}

TEST_F(TestDialogGameCheats, RefreshClampsRowIndexToLastRemainingRow)
{
  for (bool getMore : {false, true})
  {
    SCOPED_TRACE(getMore);
    CTestDialogGameCheats dialog;
    dialog.getMore = getMore;
    dialog.OpenForTest();
    dialog.Focus(FIRST_CHEAT + 240);
    dialog.List().FinishScroll();
    dialog.cheatCount = 100;

    dialog.Refresh();
    const int lastRow = FIRST_CHEAT + (getMore ? 100 : 99);
    EXPECT_EQ(dialog.GetFocusedControlID(), lastRow);
    EXPECT_TRUE(dialog.GetControl(lastRow)->HasFocus());
    EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), getMore ? 6300 : 6230);
    dialog.List().FinishScroll();
    EXPECT_TRUE(dialog.List().IsOnScreen(lastRow));

    dialog.cheatCount = 0;
    dialog.getMore = false;
    dialog.Refresh();
    EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
    EXPECT_TRUE(dialog.GetControl(CLOSE)->HasFocus());
    dialog.List().FinishScroll();
    EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 0);
  }
}

TEST_F(TestDialogGameCheats, RefreshAfterRemovingGetMoreFocusesLastRow)
{
  CTestDialogGameCheats dialog;
  dialog.getMore = true;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + dialog.cheatCount);
  dialog.List().FinishScroll();
  dialog.getMore = false;

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 330);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 22400);
  dialog.List().FinishScroll();
  EXPECT_TRUE(dialog.List().IsOnScreen(FIRST_CHEAT + 330));
}

TEST_F(TestDialogGameCheats, RefreshWithCloseFocusClampsRememberedRow)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.Focus(CLOSE);
  dialog.cheatCount = 100;

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
  EXPECT_TRUE(dialog.GetControl(CLOSE)->HasFocus());
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 6230);
  ASSERT_TRUE(dialog.OnMove(CLOSE, ACTION_MOVE_RIGHT));
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 99);
  dialog.List().FinishScroll();
  EXPECT_TRUE(dialog.List().IsOnScreen(FIRST_CHEAT + 99));
}

TEST_F(TestDialogGameCheats, RefreshPreservesScrollbarPageAndInvalidatedRowRestoration)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.Focus(SCROLLBAR);
  dialog.ScrollPage(ACTION_MOVE_DOWN);
  dialog.List().FinishScroll();
  const float offset = dialog.List().ScrollOffset();

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), SCROLLBAR);
  EXPECT_TRUE(dialog.GetControl(SCROLLBAR)->HasFocus());
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  ASSERT_TRUE(dialog.OnMove(SCROLLBAR, ACTION_MOVE_LEFT));
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 241);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
}

TEST_F(TestDialogGameCheats, RefreshMovesAnInvalidatedFocusedRowToTheScrollbarPage)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.ScrollPage(ACTION_MOVE_DOWN);
  dialog.List().FinishScroll();
  ASSERT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 240);
  const float offset = dialog.List().ScrollOffset();

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 241);
  EXPECT_TRUE(dialog.GetControl(FIRST_CHEAT + 241)->HasFocus());
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
}

TEST_F(TestDialogGameCheats, RefreshDuringScrollingKeepsOffsetAndBringsFocusedRowIntoView)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.Focus(FIRST_CHEAT + 260);
  dialog.List().AdvanceScroll(1);
  dialog.List().AdvanceScroll(60);
  const float offset = dialog.List().ScrollOffset();
  ASSERT_GT(offset, 16100);
  ASSERT_LT(offset, 17500);

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 260);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.List().FinishScroll();
  EXPECT_TRUE(dialog.List().IsOnScreen(FIRST_CHEAT + 260));
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 17500);
}

TEST_F(TestDialogGameCheats, RefreshWithCloseFocusStopsThePreviousScroll)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().AdvanceScroll(1);
  dialog.List().AdvanceScroll(63);
  const float offset = dialog.List().ScrollOffset();
  ASSERT_GT(offset, 0);
  dialog.Focus(CLOSE);

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  ASSERT_TRUE(dialog.OnMove(CLOSE, ACTION_MOVE_RIGHT));
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  EXPECT_TRUE(dialog.List().IsOnScreen(FIRST_CHEAT + 240));
}

TEST_F(TestDialogGameCheats, RefreshDuringScrollbarPagingReturnsToTheVisiblePage)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.Focus(SCROLLBAR);
  dialog.ScrollPage(ACTION_MOVE_DOWN);
  dialog.List().AdvanceScroll(1);
  dialog.List().AdvanceScroll(63);
  const float offset = dialog.List().ScrollOffset();

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), SCROLLBAR);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
  ASSERT_TRUE(dialog.OnMove(SCROLLBAR, ACTION_MOVE_LEFT));
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 234);
  EXPECT_TRUE(dialog.List().IsOnScreen(FIRST_CHEAT + 234));
}

TEST_F(TestDialogGameCheats, RefreshToEmptyStopsThePreviousScroll)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().AdvanceScroll(1);
  dialog.List().AdvanceScroll(60);
  dialog.Focus(SCROLLBAR);
  dialog.cheatCount = 0;

  dialog.Refresh();
  EXPECT_EQ(dialog.GetFocusedControlID(), CLOSE);
  EXPECT_TRUE(dialog.GetControl(CLOSE)->HasFocus());
  EXPECT_EQ(dialog.List().ItemCount(), 0);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 0);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 0);
}

TEST_F(TestDialogGameCheats, RefreshMovesFocusWhenScrollbarBecomesHidden)
{
  CTestDialogGameCheats dialog(false);
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  dialog.Focus(SCROLLBAR);
  dialog.cheatCount = 5;

  dialog.Refresh();
  EXPECT_FALSE(dialog.GetControl(SCROLLBAR)->CanFocus());
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 4);
  EXPECT_TRUE(dialog.GetControl(FIRST_CHEAT + 4)->HasFocus());
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 0);
  dialog.List().FinishScroll();
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), 0);
}

TEST_F(TestDialogGameCheats, RefreshWhileClosingKeepsFocusAndOffset)
{
  CTestDialogGameCheats dialog;
  dialog.OpenForTest();
  dialog.Focus(FIRST_CHEAT + 240);
  dialog.List().FinishScroll();
  const float offset = dialog.List().ScrollOffset();
  dialog.MarkClosing();
  dialog.cheatCount = 0;

  dialog.Refresh();
  EXPECT_EQ(dialog.List().ItemCount(), 331);
  EXPECT_EQ(dialog.GetFocusedControlID(), FIRST_CHEAT + 240);
  EXPECT_FLOAT_EQ(dialog.List().ScrollOffset(), offset);
}
