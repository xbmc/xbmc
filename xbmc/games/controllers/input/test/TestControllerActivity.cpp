/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/controllers/input/ControllerActivity.h"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;
using namespace std::chrono_literals;

//
// Spec: Mouse motion should activate the controller and time out
//
TEST(TestControllerActivity, MouseMotionActivates)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer", 5, 0);
  activity.OnInputFrame();

  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  std::this_thread::sleep_for(100ms);

  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}

//
// Spec: Activation should persist while a mouse button is held
//
TEST(TestControllerActivity, MotionPersistsWithButton)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer", 1, 0);
  activity.OnMouseButtonPress("left_button");
  activity.OnInputFrame();

  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  std::this_thread::sleep_for(100ms);

  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  activity.OnMouseButtonRelease("left_button");

  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}
