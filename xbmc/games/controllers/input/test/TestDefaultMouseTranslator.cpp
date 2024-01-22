/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/input/DefaultMouseTranslator.h"
#include "games/controllers/input/PhysicalFeature.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
// Helper functions
JOYSTICK::FEATURE_TYPE GetButtonType(MOUSE::BUTTON_ID buttonId,
                                     const ControllerPtr& controller,
                                     unsigned int& count)
{
  std::string buttonName = CDefaultMouseTranslator::TranslateMouseButton(buttonId);
  ++count;
  return controller->GetFeature(buttonName).Type();
}

JOYSTICK::FEATURE_TYPE GetPointerType(MOUSE::POINTER_DIRECTION direction,
                                      const ControllerPtr& controller)
{
  std::string pointerName = CDefaultMouseTranslator::TranslateMousePointer(direction);
  return controller->GetFeature(pointerName).Type();
}

GAME::ControllerPtr GetController()
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Load add-on info
  ADDON::AddonPtr addon;
  EXPECT_TRUE(addonManager.GetAddon(DEFAULT_MOUSE_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));

  // Convert to game controller
  GAME::ControllerPtr controller = std::static_pointer_cast<GAME::CController>(addon);
  EXPECT_NE(controller.get(), nullptr);
  EXPECT_EQ(controller->ID(), DEFAULT_MOUSE_ID);

  // Load controller profile
  EXPECT_TRUE(controller->LoadLayout());
  EXPECT_EQ(controller->Features().size(), 10);

  return controller;
}
} // namespace

TEST(TestDefaultMouseTranslator, TranslateButton)
{
  GAME::ControllerPtr controller = GetController();

  //
  // Spec: Should translate all mouse buttons
  //
  unsigned int count = 0;

  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::LEFT, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::RIGHT, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::MIDDLE, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::BUTTON4, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::BUTTON5, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::WHEEL_UP, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::WHEEL_DOWN, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::HORIZ_WHEEL_LEFT, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::HORIZ_WHEEL_RIGHT, controller, count),
            JOYSTICK::FEATURE_TYPE::SCALAR);

  EXPECT_EQ(count, 9);
}

TEST(TestDefaultMouseTranslator, TranslatePointer)
{
  GAME::ControllerPtr controller = GetController();

  //
  // Spec: Should translate mouse pointer
  //
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::UP, controller),
            JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::DOWN, controller),
            JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::RIGHT, controller),
            JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::LEFT, controller),
            JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::NONE, controller),
            JOYSTICK::FEATURE_TYPE::UNKNOWN);
}
