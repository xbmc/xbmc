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
#include "games/controllers/input/DefaultKeyboardTranslator.h"
#include "games/controllers/input/PhysicalFeature.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
// Helper functions
KEYBOARD::XBMCKey GetKeycode(KEYBOARD::XBMCKey keysym,
                             const ControllerPtr& controller,
                             unsigned int& count)
{
  KEYBOARD::KeyName keyName = CDefaultKeyboardTranslator::TranslateKeycode(keysym);
  ++count;
  return controller->GetFeature(keyName).Keycode();
}

const GAME::ControllerPtr GetController()
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Load add-on info
  ADDON::AddonPtr addon;
  EXPECT_TRUE(addonManager.GetAddon(DEFAULT_KEYBOARD_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));

  // Convert to game controller
  GAME::ControllerPtr controller = std::static_pointer_cast<GAME::CController>(addon);
  EXPECT_NE(controller.get(), nullptr);
  EXPECT_EQ(controller->ID(), DEFAULT_KEYBOARD_ID);

  // Load controller profile
  EXPECT_TRUE(controller->LoadLayout());
  EXPECT_EQ(controller->Features().size(), 140);

  return controller;
}
} // namespace

TEST(TestDefaultKeyboardTranslator, TranslateKeycode)
{
  GAME::ControllerPtr controller = GetController();

  //
  // Spec: Should translate all keyboard keys
  //
  unsigned int count = 0;

  EXPECT_EQ(GetKeycode(XBMCK_BACKSPACE, controller, count), XBMCK_BACKSPACE);
  EXPECT_EQ(GetKeycode(XBMCK_TAB, controller, count), XBMCK_TAB);
  EXPECT_EQ(GetKeycode(XBMCK_CLEAR, controller, count), XBMCK_CLEAR);
  EXPECT_EQ(GetKeycode(XBMCK_RETURN, controller, count), XBMCK_RETURN);
  EXPECT_EQ(GetKeycode(XBMCK_PAUSE, controller, count), XBMCK_PAUSE);
  EXPECT_EQ(GetKeycode(XBMCK_ESCAPE, controller, count), XBMCK_ESCAPE);
  EXPECT_EQ(GetKeycode(XBMCK_SPACE, controller, count), XBMCK_SPACE);
  EXPECT_EQ(GetKeycode(XBMCK_EXCLAIM, controller, count), XBMCK_EXCLAIM);
  EXPECT_EQ(GetKeycode(XBMCK_QUOTEDBL, controller, count), XBMCK_QUOTEDBL);
  EXPECT_EQ(GetKeycode(XBMCK_HASH, controller, count), XBMCK_HASH);
  EXPECT_EQ(GetKeycode(XBMCK_DOLLAR, controller, count), XBMCK_DOLLAR);
  EXPECT_EQ(GetKeycode(XBMCK_AMPERSAND, controller, count), XBMCK_AMPERSAND);
  EXPECT_EQ(GetKeycode(XBMCK_QUOTE, controller, count), XBMCK_QUOTE);
  EXPECT_EQ(GetKeycode(XBMCK_LEFTPAREN, controller, count), XBMCK_LEFTPAREN);
  EXPECT_EQ(GetKeycode(XBMCK_RIGHTPAREN, controller, count), XBMCK_RIGHTPAREN);
  EXPECT_EQ(GetKeycode(XBMCK_ASTERISK, controller, count), XBMCK_ASTERISK);
  EXPECT_EQ(GetKeycode(XBMCK_PLUS, controller, count), XBMCK_PLUS);
  EXPECT_EQ(GetKeycode(XBMCK_COMMA, controller, count), XBMCK_COMMA);
  EXPECT_EQ(GetKeycode(XBMCK_MINUS, controller, count), XBMCK_MINUS);
  EXPECT_EQ(GetKeycode(XBMCK_PERIOD, controller, count), XBMCK_PERIOD);
  EXPECT_EQ(GetKeycode(XBMCK_SLASH, controller, count), XBMCK_SLASH);
  EXPECT_EQ(GetKeycode(XBMCK_0, controller, count), XBMCK_0);
  EXPECT_EQ(GetKeycode(XBMCK_1, controller, count), XBMCK_1);
  EXPECT_EQ(GetKeycode(XBMCK_2, controller, count), XBMCK_2);
  EXPECT_EQ(GetKeycode(XBMCK_3, controller, count), XBMCK_3);
  EXPECT_EQ(GetKeycode(XBMCK_4, controller, count), XBMCK_4);
  EXPECT_EQ(GetKeycode(XBMCK_5, controller, count), XBMCK_5);
  EXPECT_EQ(GetKeycode(XBMCK_6, controller, count), XBMCK_6);
  EXPECT_EQ(GetKeycode(XBMCK_7, controller, count), XBMCK_7);
  EXPECT_EQ(GetKeycode(XBMCK_8, controller, count), XBMCK_8);
  EXPECT_EQ(GetKeycode(XBMCK_9, controller, count), XBMCK_9);
  EXPECT_EQ(GetKeycode(XBMCK_COLON, controller, count), XBMCK_COLON);
  EXPECT_EQ(GetKeycode(XBMCK_SEMICOLON, controller, count), XBMCK_SEMICOLON);
  EXPECT_EQ(GetKeycode(XBMCK_LESS, controller, count), XBMCK_LESS);
  EXPECT_EQ(GetKeycode(XBMCK_EQUALS, controller, count), XBMCK_EQUALS);
  EXPECT_EQ(GetKeycode(XBMCK_GREATER, controller, count), XBMCK_GREATER);
  EXPECT_EQ(GetKeycode(XBMCK_QUESTION, controller, count), XBMCK_QUESTION);
  EXPECT_EQ(GetKeycode(XBMCK_AT, controller, count), XBMCK_AT);
  EXPECT_EQ(GetKeycode(XBMCK_LEFTBRACKET, controller, count), XBMCK_LEFTBRACKET);
  EXPECT_EQ(GetKeycode(XBMCK_BACKSLASH, controller, count), XBMCK_BACKSLASH);
  EXPECT_EQ(GetKeycode(XBMCK_RIGHTBRACKET, controller, count), XBMCK_RIGHTBRACKET);
  EXPECT_EQ(GetKeycode(XBMCK_CARET, controller, count), XBMCK_CARET);
  EXPECT_EQ(GetKeycode(XBMCK_UNDERSCORE, controller, count), XBMCK_UNDERSCORE);
  EXPECT_EQ(GetKeycode(XBMCK_BACKQUOTE, controller, count), XBMCK_BACKQUOTE);
  EXPECT_EQ(GetKeycode(XBMCK_a, controller, count), XBMCK_a);
  EXPECT_EQ(GetKeycode(XBMCK_b, controller, count), XBMCK_b);
  EXPECT_EQ(GetKeycode(XBMCK_c, controller, count), XBMCK_c);
  EXPECT_EQ(GetKeycode(XBMCK_d, controller, count), XBMCK_d);
  EXPECT_EQ(GetKeycode(XBMCK_e, controller, count), XBMCK_e);
  EXPECT_EQ(GetKeycode(XBMCK_f, controller, count), XBMCK_f);
  EXPECT_EQ(GetKeycode(XBMCK_g, controller, count), XBMCK_g);
  EXPECT_EQ(GetKeycode(XBMCK_h, controller, count), XBMCK_h);
  EXPECT_EQ(GetKeycode(XBMCK_i, controller, count), XBMCK_i);
  EXPECT_EQ(GetKeycode(XBMCK_j, controller, count), XBMCK_j);
  EXPECT_EQ(GetKeycode(XBMCK_k, controller, count), XBMCK_k);
  EXPECT_EQ(GetKeycode(XBMCK_l, controller, count), XBMCK_l);
  EXPECT_EQ(GetKeycode(XBMCK_m, controller, count), XBMCK_m);
  EXPECT_EQ(GetKeycode(XBMCK_n, controller, count), XBMCK_n);
  EXPECT_EQ(GetKeycode(XBMCK_o, controller, count), XBMCK_o);
  EXPECT_EQ(GetKeycode(XBMCK_p, controller, count), XBMCK_p);
  EXPECT_EQ(GetKeycode(XBMCK_q, controller, count), XBMCK_q);
  EXPECT_EQ(GetKeycode(XBMCK_r, controller, count), XBMCK_r);
  EXPECT_EQ(GetKeycode(XBMCK_s, controller, count), XBMCK_s);
  EXPECT_EQ(GetKeycode(XBMCK_t, controller, count), XBMCK_t);
  EXPECT_EQ(GetKeycode(XBMCK_u, controller, count), XBMCK_u);
  EXPECT_EQ(GetKeycode(XBMCK_v, controller, count), XBMCK_v);
  EXPECT_EQ(GetKeycode(XBMCK_w, controller, count), XBMCK_w);
  EXPECT_EQ(GetKeycode(XBMCK_x, controller, count), XBMCK_x);
  EXPECT_EQ(GetKeycode(XBMCK_y, controller, count), XBMCK_y);
  EXPECT_EQ(GetKeycode(XBMCK_z, controller, count), XBMCK_z);
  EXPECT_EQ(GetKeycode(XBMCK_LEFTBRACE, controller, count), XBMCK_LEFTBRACE);
  EXPECT_EQ(GetKeycode(XBMCK_PIPE, controller, count), XBMCK_PIPE);
  EXPECT_EQ(GetKeycode(XBMCK_RIGHTBRACE, controller, count), XBMCK_RIGHTBRACE);
  EXPECT_EQ(GetKeycode(XBMCK_TILDE, controller, count), XBMCK_TILDE);
  EXPECT_EQ(GetKeycode(XBMCK_DELETE, controller, count), XBMCK_DELETE);
  EXPECT_EQ(GetKeycode(XBMCK_KP0, controller, count), XBMCK_KP0);
  EXPECT_EQ(GetKeycode(XBMCK_KP1, controller, count), XBMCK_KP1);
  EXPECT_EQ(GetKeycode(XBMCK_KP2, controller, count), XBMCK_KP2);
  EXPECT_EQ(GetKeycode(XBMCK_KP3, controller, count), XBMCK_KP3);
  EXPECT_EQ(GetKeycode(XBMCK_KP4, controller, count), XBMCK_KP4);
  EXPECT_EQ(GetKeycode(XBMCK_KP5, controller, count), XBMCK_KP5);
  EXPECT_EQ(GetKeycode(XBMCK_KP6, controller, count), XBMCK_KP6);
  EXPECT_EQ(GetKeycode(XBMCK_KP7, controller, count), XBMCK_KP7);
  EXPECT_EQ(GetKeycode(XBMCK_KP8, controller, count), XBMCK_KP8);
  EXPECT_EQ(GetKeycode(XBMCK_KP9, controller, count), XBMCK_KP9);
  EXPECT_EQ(GetKeycode(XBMCK_KP_PERIOD, controller, count), XBMCK_KP_PERIOD);
  EXPECT_EQ(GetKeycode(XBMCK_KP_DIVIDE, controller, count), XBMCK_KP_DIVIDE);
  EXPECT_EQ(GetKeycode(XBMCK_KP_MULTIPLY, controller, count), XBMCK_KP_MULTIPLY);
  EXPECT_EQ(GetKeycode(XBMCK_KP_MINUS, controller, count), XBMCK_KP_MINUS);
  EXPECT_EQ(GetKeycode(XBMCK_KP_PLUS, controller, count), XBMCK_KP_PLUS);
  EXPECT_EQ(GetKeycode(XBMCK_KP_ENTER, controller, count), XBMCK_KP_ENTER);
  EXPECT_EQ(GetKeycode(XBMCK_KP_EQUALS, controller, count), XBMCK_KP_EQUALS);
  EXPECT_EQ(GetKeycode(XBMCK_UP, controller, count), XBMCK_UP);
  EXPECT_EQ(GetKeycode(XBMCK_DOWN, controller, count), XBMCK_DOWN);
  EXPECT_EQ(GetKeycode(XBMCK_RIGHT, controller, count), XBMCK_RIGHT);
  EXPECT_EQ(GetKeycode(XBMCK_LEFT, controller, count), XBMCK_LEFT);
  EXPECT_EQ(GetKeycode(XBMCK_INSERT, controller, count), XBMCK_INSERT);
  EXPECT_EQ(GetKeycode(XBMCK_HOME, controller, count), XBMCK_HOME);
  EXPECT_EQ(GetKeycode(XBMCK_END, controller, count), XBMCK_END);
  EXPECT_EQ(GetKeycode(XBMCK_PAGEUP, controller, count), XBMCK_PAGEUP);
  EXPECT_EQ(GetKeycode(XBMCK_PAGEDOWN, controller, count), XBMCK_PAGEDOWN);
  EXPECT_EQ(GetKeycode(XBMCK_F1, controller, count), XBMCK_F1);
  EXPECT_EQ(GetKeycode(XBMCK_F2, controller, count), XBMCK_F2);
  EXPECT_EQ(GetKeycode(XBMCK_F3, controller, count), XBMCK_F3);
  EXPECT_EQ(GetKeycode(XBMCK_F4, controller, count), XBMCK_F4);
  EXPECT_EQ(GetKeycode(XBMCK_F5, controller, count), XBMCK_F5);
  EXPECT_EQ(GetKeycode(XBMCK_F6, controller, count), XBMCK_F6);
  EXPECT_EQ(GetKeycode(XBMCK_F7, controller, count), XBMCK_F7);
  EXPECT_EQ(GetKeycode(XBMCK_F8, controller, count), XBMCK_F8);
  EXPECT_EQ(GetKeycode(XBMCK_F9, controller, count), XBMCK_F9);
  EXPECT_EQ(GetKeycode(XBMCK_F10, controller, count), XBMCK_F10);
  EXPECT_EQ(GetKeycode(XBMCK_F11, controller, count), XBMCK_F11);
  EXPECT_EQ(GetKeycode(XBMCK_F12, controller, count), XBMCK_F12);
  EXPECT_EQ(GetKeycode(XBMCK_F13, controller, count), XBMCK_F13);
  EXPECT_EQ(GetKeycode(XBMCK_F14, controller, count), XBMCK_F14);
  EXPECT_EQ(GetKeycode(XBMCK_F15, controller, count), XBMCK_F15);
  EXPECT_EQ(GetKeycode(XBMCK_NUMLOCK, controller, count), XBMCK_NUMLOCK);
  EXPECT_EQ(GetKeycode(XBMCK_CAPSLOCK, controller, count), XBMCK_CAPSLOCK);
  EXPECT_EQ(GetKeycode(XBMCK_SCROLLOCK, controller, count), XBMCK_SCROLLOCK);
  EXPECT_EQ(GetKeycode(XBMCK_LSHIFT, controller, count), XBMCK_LSHIFT);
  EXPECT_EQ(GetKeycode(XBMCK_RSHIFT, controller, count), XBMCK_RSHIFT);
  EXPECT_EQ(GetKeycode(XBMCK_LCTRL, controller, count), XBMCK_LCTRL);
  EXPECT_EQ(GetKeycode(XBMCK_RCTRL, controller, count), XBMCK_RCTRL);
  EXPECT_EQ(GetKeycode(XBMCK_LALT, controller, count), XBMCK_LALT);
  EXPECT_EQ(GetKeycode(XBMCK_RALT, controller, count), XBMCK_RALT);
  EXPECT_EQ(GetKeycode(XBMCK_LMETA, controller, count), XBMCK_LMETA);
  EXPECT_EQ(GetKeycode(XBMCK_RMETA, controller, count), XBMCK_RMETA);
  EXPECT_EQ(GetKeycode(XBMCK_LSUPER, controller, count), XBMCK_LSUPER);
  EXPECT_EQ(GetKeycode(XBMCK_RSUPER, controller, count), XBMCK_RSUPER);
  EXPECT_EQ(GetKeycode(XBMCK_MODE, controller, count), XBMCK_MODE);
  EXPECT_EQ(GetKeycode(XBMCK_COMPOSE, controller, count), XBMCK_COMPOSE);
  EXPECT_EQ(GetKeycode(XBMCK_HELP, controller, count), XBMCK_HELP);
  EXPECT_EQ(GetKeycode(XBMCK_PRINT, controller, count), XBMCK_PRINT);
  EXPECT_EQ(GetKeycode(XBMCK_SYSREQ, controller, count), XBMCK_SYSREQ);
  EXPECT_EQ(GetKeycode(XBMCK_BREAK, controller, count), XBMCK_BREAK);
  EXPECT_EQ(GetKeycode(XBMCK_MENU, controller, count), XBMCK_MENU);
  EXPECT_EQ(GetKeycode(XBMCK_POWER, controller, count), XBMCK_POWER);
  EXPECT_EQ(GetKeycode(XBMCK_EURO, controller, count), XBMCK_EURO);
  EXPECT_EQ(GetKeycode(XBMCK_UNDO, controller, count), XBMCK_UNDO);
  EXPECT_EQ(GetKeycode(XBMCK_OEM_102, controller, count), XBMCK_OEM_102);

  EXPECT_EQ(count, 140);
}
