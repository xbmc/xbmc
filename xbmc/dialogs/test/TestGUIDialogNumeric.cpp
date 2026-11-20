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
#if defined(TARGET_DARWIN_OSX) || defined(TARGET_FREEBSD)
  GTEST_SKIP() << "mktime does not handle dates before 1900 on this platform";
#endif

  CGUIDialogNumeric dialog;
  dialog.SetMode(KODI::DIALOGS::NUMERIC_MODE::TIME_SECONDS, "01:42:00");

  dialog.OnAction(CAction(ACTION_NEXT_ITEM));
  dialog.OnAction(CAction(ACTION_NEXT_ITEM));
  dialog.OnAction(CAction(REMOTE_3));

  const KODI::TIME::SystemTime time{dialog.GetOutput()};
  EXPECT_EQ(42, time.minute);
  EXPECT_EQ(3, time.second);
}

namespace
{
//! The day the dialog settles on once \p blocks of a date have been confirmed.
unsigned short DayAfterConfirming(unsigned short day,
                                  unsigned short month,
                                  unsigned short year,
                                  int blocks)
{
  KODI::TIME::SystemTime date{};
  date.day = day;
  date.month = month;
  date.year = year;

  CGUIDialogNumeric dialog;
  dialog.SetMode(KODI::DIALOGS::NUMERIC_MODE::DATE, date);
  for (int i = 0; i < blocks; ++i)
    dialog.OnAction(CAction(ACTION_NEXT_ITEM));
  return dialog.GetOutput().day;
}
} // namespace

TEST(TestGUIDialogNumeric, ADateIsKeptWithinItsMonth)
{
  EXPECT_EQ(30, DayAfterConfirming(31, 4, 2024, 1));
  EXPECT_EQ(31, DayAfterConfirming(31, 5, 2024, 1));
  EXPECT_EQ(29, DayAfterConfirming(30, 2, 2023, 1)) << "the year is not known yet";
  EXPECT_EQ(28, DayAfterConfirming(29, 2, 2023, 2));
  EXPECT_EQ(29, DayAfterConfirming(29, 2, 2024, 2));
  EXPECT_EQ(28, DayAfterConfirming(29, 2, 1900, 2));
  EXPECT_EQ(29, DayAfterConfirming(29, 2, 2000, 2));
  EXPECT_EQ(31, DayAfterConfirming(31, 13, 2024, 1)) << "a month out of range is left alone";
}
