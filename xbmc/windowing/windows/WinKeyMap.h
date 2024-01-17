/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceBroker.h"
#include "Util.h"
#include "input/keyboard/XBMC_keysym.h"
#include "input/keyboard/XBMC_vkeys.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <array>

namespace KODI
{
namespace WINDOWING
{
namespace WINDOWS
{

static std::array<XBMCKey, XBMCK_LAST> VK_keymap;

static void DIB_InitOSKeymap()
{
  /* Map the VK keysyms */
  VK_keymap.fill(XBMCK_UNKNOWN);

  VK_keymap[VK_BACK] = XBMCK_BACKSPACE;
  VK_keymap[VK_TAB] = XBMCK_TAB;
  VK_keymap[VK_CLEAR] = XBMCK_CLEAR;
  VK_keymap[VK_RETURN] = XBMCK_RETURN;
  VK_keymap[VK_PAUSE] = XBMCK_PAUSE;
  VK_keymap[VK_ESCAPE] = XBMCK_ESCAPE;
  VK_keymap[VK_SPACE] = XBMCK_SPACE;
  VK_keymap[VK_APOSTROPHE] = XBMCK_QUOTE;
  VK_keymap[VK_COMMA] = XBMCK_COMMA;
  VK_keymap[VK_MINUS] = XBMCK_MINUS;
  VK_keymap[VK_PERIOD] = XBMCK_PERIOD;
  VK_keymap[VK_SLASH] = XBMCK_SLASH;
  VK_keymap[VK_0] = XBMCK_0;
  VK_keymap[VK_1] = XBMCK_1;
  VK_keymap[VK_2] = XBMCK_2;
  VK_keymap[VK_3] = XBMCK_3;
  VK_keymap[VK_4] = XBMCK_4;
  VK_keymap[VK_5] = XBMCK_5;
  VK_keymap[VK_6] = XBMCK_6;
  VK_keymap[VK_7] = XBMCK_7;
  VK_keymap[VK_8] = XBMCK_8;
  VK_keymap[VK_9] = XBMCK_9;
  VK_keymap[VK_SEMICOLON] = XBMCK_SEMICOLON;
  VK_keymap[VK_EQUALS] = XBMCK_EQUALS;
  VK_keymap[VK_LBRACKET] = XBMCK_LEFTBRACKET;
  VK_keymap[VK_BACKSLASH] = XBMCK_BACKSLASH;
  VK_keymap[VK_OEM_102] = XBMCK_BACKSLASH;
  VK_keymap[VK_RBRACKET] = XBMCK_RIGHTBRACKET;
  VK_keymap[VK_GRAVE] = XBMCK_BACKQUOTE;
  VK_keymap[VK_BACKTICK] = XBMCK_BACKQUOTE;
  VK_keymap[VK_A] = XBMCK_a;
  VK_keymap[VK_B] = XBMCK_b;
  VK_keymap[VK_C] = XBMCK_c;
  VK_keymap[VK_D] = XBMCK_d;
  VK_keymap[VK_E] = XBMCK_e;
  VK_keymap[VK_F] = XBMCK_f;
  VK_keymap[VK_G] = XBMCK_g;
  VK_keymap[VK_H] = XBMCK_h;
  VK_keymap[VK_I] = XBMCK_i;
  VK_keymap[VK_J] = XBMCK_j;
  VK_keymap[VK_K] = XBMCK_k;
  VK_keymap[VK_L] = XBMCK_l;
  VK_keymap[VK_M] = XBMCK_m;
  VK_keymap[VK_N] = XBMCK_n;
  VK_keymap[VK_O] = XBMCK_o;
  VK_keymap[VK_P] = XBMCK_p;
  VK_keymap[VK_Q] = XBMCK_q;
  VK_keymap[VK_R] = XBMCK_r;
  VK_keymap[VK_S] = XBMCK_s;
  VK_keymap[VK_T] = XBMCK_t;
  VK_keymap[VK_U] = XBMCK_u;
  VK_keymap[VK_V] = XBMCK_v;
  VK_keymap[VK_W] = XBMCK_w;
  VK_keymap[VK_X] = XBMCK_x;
  VK_keymap[VK_Y] = XBMCK_y;
  VK_keymap[VK_Z] = XBMCK_z;
  VK_keymap[VK_DELETE] = XBMCK_DELETE;

  VK_keymap[VK_NUMPAD0] = XBMCK_KP0;
  VK_keymap[VK_NUMPAD1] = XBMCK_KP1;
  VK_keymap[VK_NUMPAD2] = XBMCK_KP2;
  VK_keymap[VK_NUMPAD3] = XBMCK_KP3;
  VK_keymap[VK_NUMPAD4] = XBMCK_KP4;
  VK_keymap[VK_NUMPAD5] = XBMCK_KP5;
  VK_keymap[VK_NUMPAD6] = XBMCK_KP6;
  VK_keymap[VK_NUMPAD7] = XBMCK_KP7;
  VK_keymap[VK_NUMPAD8] = XBMCK_KP8;
  VK_keymap[VK_NUMPAD9] = XBMCK_KP9;
  VK_keymap[VK_DECIMAL] = XBMCK_KP_PERIOD;
  VK_keymap[VK_DIVIDE] = XBMCK_KP_DIVIDE;
  VK_keymap[VK_MULTIPLY] = XBMCK_KP_MULTIPLY;
  VK_keymap[VK_SUBTRACT] = XBMCK_KP_MINUS;
  VK_keymap[VK_ADD] = XBMCK_KP_PLUS;

  VK_keymap[VK_UP] = XBMCK_UP;
  VK_keymap[VK_DOWN] = XBMCK_DOWN;
  VK_keymap[VK_RIGHT] = XBMCK_RIGHT;
  VK_keymap[VK_LEFT] = XBMCK_LEFT;
  VK_keymap[VK_INSERT] = XBMCK_INSERT;
  VK_keymap[VK_HOME] = XBMCK_HOME;
  VK_keymap[VK_END] = XBMCK_END;
  VK_keymap[VK_PRIOR] = XBMCK_PAGEUP;
  VK_keymap[VK_NEXT] = XBMCK_PAGEDOWN;

  VK_keymap[VK_F1] = XBMCK_F1;
  VK_keymap[VK_F2] = XBMCK_F2;
  VK_keymap[VK_F3] = XBMCK_F3;
  VK_keymap[VK_F4] = XBMCK_F4;
  VK_keymap[VK_F5] = XBMCK_F5;
  VK_keymap[VK_F6] = XBMCK_F6;
  VK_keymap[VK_F7] = XBMCK_F7;
  VK_keymap[VK_F8] = XBMCK_F8;
  VK_keymap[VK_F9] = XBMCK_F9;
  VK_keymap[VK_F10] = XBMCK_F10;
  VK_keymap[VK_F11] = XBMCK_F11;
  VK_keymap[VK_F12] = XBMCK_F12;
  VK_keymap[VK_F13] = XBMCK_F13;
  VK_keymap[VK_F14] = XBMCK_F14;
  VK_keymap[VK_F15] = XBMCK_F15;

  VK_keymap[VK_NUMLOCK] = XBMCK_NUMLOCK;
  VK_keymap[VK_CAPITAL] = XBMCK_CAPSLOCK;
  VK_keymap[VK_SCROLL] = XBMCK_SCROLLOCK;
  VK_keymap[VK_RSHIFT] = XBMCK_RSHIFT;
  VK_keymap[VK_LSHIFT] = XBMCK_LSHIFT;
  VK_keymap[VK_RCONTROL] = XBMCK_RCTRL;
  VK_keymap[VK_LCONTROL] = XBMCK_LCTRL;
  VK_keymap[VK_RMENU] = XBMCK_RALT;
  VK_keymap[VK_LMENU] = XBMCK_LALT;
  VK_keymap[VK_RWIN] = XBMCK_RSUPER;
  VK_keymap[VK_LWIN] = XBMCK_LSUPER;

  VK_keymap[VK_HELP] = XBMCK_HELP;
#ifdef VK_PRINT
  VK_keymap[VK_PRINT] = XBMCK_PRINT;
#endif
  VK_keymap[VK_SNAPSHOT] = XBMCK_PRINT;
  VK_keymap[VK_CANCEL] = XBMCK_BREAK;
  VK_keymap[VK_APPS] = XBMCK_MENU;

  // Only include the multimedia keys if they have been enabled in the
  // advanced settings
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_enableMultimediaKeys)
  {
    VK_keymap[VK_BROWSER_BACK] = XBMCK_BROWSER_BACK;
    VK_keymap[VK_BROWSER_FORWARD] = XBMCK_BROWSER_FORWARD;
    VK_keymap[VK_BROWSER_REFRESH] = XBMCK_BROWSER_REFRESH;
    VK_keymap[VK_BROWSER_STOP] = XBMCK_BROWSER_STOP;
    VK_keymap[VK_BROWSER_SEARCH] = XBMCK_BROWSER_SEARCH;
    VK_keymap[VK_BROWSER_FAVORITES] = XBMCK_BROWSER_FAVORITES;
    VK_keymap[VK_BROWSER_HOME] = XBMCK_BROWSER_HOME;
    VK_keymap[VK_VOLUME_MUTE] = XBMCK_VOLUME_MUTE;
    VK_keymap[VK_VOLUME_DOWN] = XBMCK_VOLUME_DOWN;
    VK_keymap[VK_VOLUME_UP] = XBMCK_VOLUME_UP;
    VK_keymap[VK_MEDIA_NEXT_TRACK] = XBMCK_MEDIA_NEXT_TRACK;
    VK_keymap[VK_MEDIA_PREV_TRACK] = XBMCK_MEDIA_PREV_TRACK;
    VK_keymap[VK_MEDIA_STOP] = XBMCK_MEDIA_STOP;
    VK_keymap[VK_MEDIA_PLAY_PAUSE] = XBMCK_MEDIA_PLAY_PAUSE;
    VK_keymap[VK_LAUNCH_MAIL] = XBMCK_LAUNCH_MAIL;
    VK_keymap[VK_LAUNCH_MEDIA_SELECT] = XBMCK_LAUNCH_MEDIA_SELECT;
    VK_keymap[VK_LAUNCH_APP1] = XBMCK_LAUNCH_APP1;
    VK_keymap[VK_LAUNCH_APP2] = XBMCK_LAUNCH_APP2;
  }
}

} // namespace WINDOWS
} // namespace WINDOWING
} // namespace KODI
