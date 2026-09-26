/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/actions/interfaces/IActionListener.h"
#include "input/keymaps/KeymapTypes.h"
#include "input/keymaps/generic/KeyHandler.h"
#include "input/keymaps/interfaces/IKeymap.h"
#include "input/keymaps/interfaces/IKeymapHandler.h"

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KEYMAP;

namespace
{
class CTestActionListener : public ACTION::IActionListener
{
public:
  bool OnAction(const CAction& action) override
  {
    actions.push_back(action);
    return true;
  }

  std::vector<CAction> actions;
};

class CTestKeymap : public IKeymap
{
public:
  std::string ControllerID() const override { return "game.controller.default"; }
  const IKeymapEnvironment* Environment() const override { return nullptr; }
  const KeymapActionGroup& GetActions(const std::string& keyName) const override { return group; }

  KeymapActionGroup group{0, {{ACTION_MOVE_DOWN, "Down", 0, {}}}};
};

class CTestKeymapHandler : public IKeymapHandler
{
public:
  bool HotkeysPressed(const std::set<std::string>& keyNames) const override
  {
    return keyNames.empty();
  }
  std::string GetLastPressed() const override { return m_lastPressed; }
  void OnPress(const std::string& keyName) override { m_lastPressed = keyName; }

private:
  std::string m_lastPressed;
};
} // namespace

class TestKeyHandler : public testing::Test
{
protected:
  CTestActionListener m_listener;
  CTestKeymap m_keymap;
  CTestKeymapHandler m_keymapHandler;
  CKeyHandler m_handler{"down", &m_listener, &m_keymap, &m_keymapHandler};
};

TEST_F(TestKeyHandler, InitialDigitalPress)
{
  m_handler.OnDigitalMotion(true, 0);

  ASSERT_EQ(m_listener.actions.size(), 1u);
  EXPECT_EQ(m_listener.actions[0].GetID(), ACTION_MOVE_DOWN);
  EXPECT_EQ(m_listener.actions[0].GetHoldTime(), 0u);
}

TEST_F(TestKeyHandler, DuplicateZeroTimePress)
{
  m_handler.OnDigitalMotion(true, 0);
  m_handler.OnDigitalMotion(true, 0);

  ASSERT_EQ(m_listener.actions.size(), 1u);
  EXPECT_EQ(m_listener.actions[0].GetID(), ACTION_MOVE_DOWN);
  EXPECT_EQ(m_listener.actions[0].GetHoldTime(), 0u);
}

TEST_F(TestKeyHandler, ReleaseThenPress)
{
  m_handler.OnDigitalMotion(true, 0);
  m_handler.OnDigitalMotion(false, 0);
  ASSERT_EQ(m_listener.actions.size(), 1u);

  m_handler.OnDigitalMotion(true, 0);

  ASSERT_EQ(m_listener.actions.size(), 2u);
  for (const auto& action : m_listener.actions)
  {
    EXPECT_EQ(action.GetID(), ACTION_MOVE_DOWN);
    EXPECT_EQ(action.GetHoldTime(), 0u);
  }
}

TEST_F(TestKeyHandler, HeldPressRepeatsAfterDelayAndInterval)
{
  m_handler.OnDigitalMotion(true, 0);
  ASSERT_EQ(m_listener.actions.size(), 1u);

  m_handler.OnDigitalMotion(true, 1);
  m_handler.OnDigitalMotion(true, 250);
  m_handler.OnDigitalMotion(true, 499);
  ASSERT_EQ(m_listener.actions.size(), 1u);

  m_handler.OnDigitalMotion(true, 500);
  ASSERT_EQ(m_listener.actions.size(), 2u);
  EXPECT_EQ(m_listener.actions[1].GetHoldTime(), 500u);

  m_handler.OnDigitalMotion(true, 549);
  ASSERT_EQ(m_listener.actions.size(), 2u);

  m_handler.OnDigitalMotion(true, 550);
  ASSERT_EQ(m_listener.actions.size(), 3u);
  EXPECT_EQ(m_listener.actions[2].GetHoldTime(), 550u);

  m_handler.OnDigitalMotion(true, 599);
  ASSERT_EQ(m_listener.actions.size(), 3u);

  m_handler.OnDigitalMotion(true, 600);
  ASSERT_EQ(m_listener.actions.size(), 4u);
  EXPECT_EQ(m_listener.actions[3].GetHoldTime(), 600u);

  for (const auto& action : m_listener.actions)
    EXPECT_EQ(action.GetID(), ACTION_MOVE_DOWN);
}

TEST_F(TestKeyHandler, ShortReleaseWithLongPressAction)
{
  m_keymap.group.actions.insert({ACTION_CONTEXT_MENU, "ContextMenu", 1000, {}});

  m_handler.OnDigitalMotion(true, 0);
  m_handler.OnDigitalMotion(true, 999);
  EXPECT_TRUE(m_listener.actions.empty());

  m_handler.OnDigitalMotion(false, 0);

  ASSERT_EQ(m_listener.actions.size(), 1u);
  EXPECT_EQ(m_listener.actions[0].GetID(), ACTION_MOVE_DOWN);
  EXPECT_EQ(m_listener.actions[0].GetHoldTime(), 0u);
}

TEST_F(TestKeyHandler, DuplicateLongPressThreshold)
{
  m_keymap.group.actions.insert({ACTION_CONTEXT_MENU, "ContextMenu", 1000, {}});

  m_handler.OnDigitalMotion(true, 0);
  m_handler.OnDigitalMotion(true, 999);
  EXPECT_TRUE(m_listener.actions.empty());

  m_handler.OnDigitalMotion(true, 1000);
  ASSERT_EQ(m_listener.actions.size(), 1u);
  EXPECT_EQ(m_listener.actions[0].GetID(), ACTION_CONTEXT_MENU);
  EXPECT_EQ(m_listener.actions[0].GetHoldTime(), 0u);

  // Both threshold callbacks have an effective hold time of zero for the long action.
  m_handler.OnDigitalMotion(true, 1000);
  ASSERT_EQ(m_listener.actions.size(), 1u);

  m_handler.OnDigitalMotion(false, 0);
  EXPECT_EQ(m_listener.actions.size(), 1u);
}

TEST_F(TestKeyHandler, AnalogMotionDispatchesContinuously)
{
  m_keymap.group.actions = {{ACTION_ANALOG_MOVE, "AnalogMove", 0, {}}};

  m_handler.OnAnalogMotion(0.75f, 0);
  m_handler.OnAnalogMotion(1.0f, 0);
  m_handler.OnAnalogMotion(0.6f, 10);

  ASSERT_EQ(m_listener.actions.size(), 3u);
  EXPECT_FLOAT_EQ(m_listener.actions[0].GetAmount(), 0.75f);
  EXPECT_FLOAT_EQ(m_listener.actions[1].GetAmount(), 1.0f);
  EXPECT_FLOAT_EQ(m_listener.actions[2].GetAmount(), 0.6f);

  m_handler.OnAnalogMotion(0.0f, 0);

  ASSERT_EQ(m_listener.actions.size(), 4u);
  EXPECT_FLOAT_EQ(m_listener.actions[3].GetAmount(), 0.0f);
  for (const auto& action : m_listener.actions)
    EXPECT_EQ(action.GetID(), ACTION_ANALOG_MOVE);
}
