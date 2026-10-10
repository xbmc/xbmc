/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

#include <string>

#include <gtest/gtest.h>

class TestInputManager : public testing::Test
{
protected:
  static bool AlwaysProcess(const std::string& builtin)
  {
    CInputManager inputManager;
    return inputManager.AlwaysProcess(CAction(ACTION_NONE, builtin));
  }
};

TEST_F(TestInputManager, AlwaysProcessesTheBuiltinsItLists)
{
  for (const char* builtin :
       {"PowerDown", "Reboot", "Restart", "RestartApp", "Suspend", "Hibernate", "Quit",
        "ShutDown", "VolumeUp", "VolumeDown", "Mute", "RunAppleScript(script.scpt)",
        "RunAddon(script.example)", "RunPlugin(plugin://plugin.video.example/)",
        "RunScript(script.example)", "System.Exec(/bin/true)", "System.ExecWait(/bin/true)"})
    EXPECT_TRUE(AlwaysProcess(builtin)) << builtin;
}

TEST_F(TestInputManager, LeavesOtherBuiltinsToTheScreensaver)
{
  EXPECT_FALSE(AlwaysProcess("ActivateWindow(Home)"));
  EXPECT_FALSE(AlwaysProcess(""));
}
