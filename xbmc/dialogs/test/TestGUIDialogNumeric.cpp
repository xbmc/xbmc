/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dialogs/GUIDialogNumeric.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

#include <gtest/gtest.h>

TEST(TestGUIDialogNumeric, BackspaceInTheSecondsRemovesASecondsDigit)
{
  KODI::TIME::SystemTime initial{};
  initial.hour = 1;
  initial.minute = 42;

  CGUIDialogNumeric dialog;
  dialog.SetMode(KODI::DIALOGS::NUMERIC_MODE::TIME_SECONDS, initial);

  dialog.OnAction(CAction(ACTION_NEXT_ITEM));
  dialog.OnAction(CAction(ACTION_NEXT_ITEM));
  dialog.OnAction(CAction(REMOTE_3));
  dialog.OnAction(CAction(ACTION_BACKSPACE));

  const KODI::TIME::SystemTime time{dialog.GetOutput()};
  EXPECT_EQ(1, time.hour);
  EXPECT_EQ(42, time.minute);
  EXPECT_EQ(0, time.second);
}

TEST(TestGUIDialogNumeric, TheSecondsCanBeReachedAfterTextInput)
{
  CGUIDialogNumeric dialog;
  dialog.SetMode(KODI::DIALOGS::NUMERIC_MODE::TIME_SECONDS, "01:42:00");

  dialog.OnAction(CAction(ACTION_NEXT_ITEM));
  dialog.OnAction(CAction(ACTION_NEXT_ITEM));
  dialog.OnAction(CAction(REMOTE_3));

  const KODI::TIME::SystemTime time{dialog.GetOutput()};
  EXPECT_EQ(42, time.minute);
  EXPECT_EQ(3, time.second);
}
