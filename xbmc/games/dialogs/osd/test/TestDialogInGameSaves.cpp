/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/dialogs/DialogGameDefines.h"
#include "games/dialogs/osd/DialogInGameSaves.h"
#include "guilib/GUIMessage.h"

#include <gtest/gtest.h>

namespace
{
class CTestDialogInGameSaves : public KODI::GAME::CDialogInGameSaves
{
public:
  using CDialogInGameSaves::GetFocusedItem;
};
} // namespace

TEST(TestDialogInGameSaves, ContainerFocusKeepsTheSaveItemSelected)
{
  CTestDialogInGameSaves dialog;
  CGUIMessage message(GUI_MSG_FOCUSED, dialog.GetID(), CONTROL_VIDEO_THUMBS);

  ASSERT_TRUE(dialog.OnMessage(message));
  EXPECT_EQ(dialog.GetFocusedItem(), 0u);
}
