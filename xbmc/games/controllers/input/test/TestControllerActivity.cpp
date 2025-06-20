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

  // Sleep well beyond the 50ms motion timeout to ensure the value
  // resets even on sluggish systems
  std::this_thread::sleep_for(150ms);

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

  // Holding a button should keep the controller active regardless
  // of the motion timeout. Sleep for a generous amount of time to
  // avoid timing issues on slow machines.
  std::this_thread::sleep_for(150ms);

  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  activity.OnMouseButtonRelease("left_button");

  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}

//
// Spec: Mouse motion should reset the timeout when moved again
//
TEST(TestControllerActivity, MotionTimeoutReset)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer", 1, 0);
  activity.OnInputFrame();
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  // Wait briefly before moving again. Keep the delay tiny so that a
  // slow scheduler can't accidentally exceed the timeout.
  std::this_thread::sleep_for(5ms);
  activity.OnMouseMotion("pointer", 2, 0);
  activity.OnInputFrame();

  // Verify we're still active shortly after the second motion. Use a
  // minimal wait to prevent hitting the timeout on busy VMs.
  std::this_thread::sleep_for(5ms);
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  // Sleep long past the timeout so the activity definitely expires
  // even on slower machines.
  std::this_thread::sleep_for(150ms);
  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}

//
// Spec: Activation should persist with multiple held buttons
//
TEST(TestControllerActivity, MotionPersistsMultipleButtons)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer", 2, 0);
  activity.OnMouseButtonPress("left_button");
  activity.OnMouseButtonPress("right_button");
  activity.OnInputFrame();

  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  // Long sleep to prove that held buttons keep the controller active
  // regardless of the mouse motion timeout.
  std::this_thread::sleep_for(150ms);
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  activity.OnMouseButtonRelease("left_button");
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  activity.OnMouseButtonRelease("right_button");
  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}

//
// Spec: Multiple pointers should be tracked independently
//
TEST(TestControllerActivity, MultiplePointers)
{
  CControllerActivity activity;

  activity.OnMouseMotion("pointer1", 1, 0);
  activity.OnInputFrame();
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  // Short pause before moving the second pointer. Keep it tiny so we
  // don't inadvertently pass the motion timeout on slow systems.
  std::this_thread::sleep_for(5ms);
  activity.OnMouseMotion("pointer2", 1, 0);
  activity.OnInputFrame();

  // Ensure both pointers keep the controller active. Again, keep the
  // wait well under the timeout.
  std::this_thread::sleep_for(5ms);
  EXPECT_FLOAT_EQ(activity.GetActivation(), 1.0f);

  // Finally exceed the timeout so all pointer activity expires.
  std::this_thread::sleep_for(150ms);
  EXPECT_FLOAT_EQ(activity.GetActivation(), 0.0f);
}
