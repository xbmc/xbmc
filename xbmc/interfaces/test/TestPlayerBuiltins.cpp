/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "application/ApplicationPlayLists.h"
#include "guilib/test/TestGUIStubs.h"
#include "interfaces/builtins/Builtins.h"
#include "playlists/PlayListTypes.h"

#include <gtest/gtest.h>

using namespace KODI;

// Disabled: the builtin reaches services the test environment does not provide.
TEST(TestPlayerBuiltins, DISABLED_QueuingVideoWhileIdleLeavesNothingPlaying)
{
  GUILIB::TEST::CTestGUIComponent gui;
  const auto playLists = CServiceBroker::GetPlayLists();
  playLists->ClearPlayLists();

  EXPECT_EQ(0, CBuiltins::GetInstance().Execute("QueueMedia(smb://server/films/film.mkv)"));

  // video does not start when queued, so no playlist is playing
  EXPECT_FALSE(playLists->GetPlayingType().has_value());

  playLists->ClearPlayLists();
}
