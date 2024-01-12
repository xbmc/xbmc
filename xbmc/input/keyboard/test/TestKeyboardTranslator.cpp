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
#include "games/controllers/input/PhysicalFeature.h"
#include "input/keyboard/KeyboardTranslator.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace KEYBOARD;

TEST(TestKeyboardTranslator, TranslateKeys)
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Load add-on info
  ADDON::AddonPtr addon;
  EXPECT_TRUE(addonManager.GetAddon(DEFAULT_KEYBOARD_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));

  // Convert to game controller
  GAME::ControllerPtr controller = std::static_pointer_cast<GAME::CController>(addon);
  ASSERT_NE(controller.get(), nullptr);
  EXPECT_EQ(controller->ID(), DEFAULT_KEYBOARD_ID);

  // Load controller profile
  EXPECT_TRUE(controller->LoadLayout());
  EXPECT_EQ(controller->Features().size(), 140);

  //
  // Spec: Should translate all keyboard symbols
  //
  unsigned int count = 0;

  for (const GAME::CPhysicalFeature& feature : controller->Features())
  {
    const KEYBOARD::XBMCKey keycode = feature.Keycode();
    EXPECT_NE(keycode, KEYBOARD::XBMCKey::XBMCK_UNKNOWN);

    const char* symbolName = CKeyboardTranslator::TranslateKeycode(keycode);
    EXPECT_STRNE(symbolName, "");

    const KEYBOARD::XBMCKey keycode2 = CKeyboardTranslator::TranslateKeysym(symbolName);
    EXPECT_EQ(keycode, keycode2);

    ++count;
  }

  EXPECT_EQ(count, 140);
}
