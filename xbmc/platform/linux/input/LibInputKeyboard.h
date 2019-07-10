/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Timer.h"
#include "windowing/XBMC_events.h"

#include <map>
#include <vector>

#include <libinput.h>
#include <xkbcommon/xkbcommon.h>

class CLibInputKeyboard
{
public:
  CLibInputKeyboard();
  ~CLibInputKeyboard();

  void ProcessKey(libinput_event_keyboard *e);
  void UpdateLeds(libinput_device *dev);
  void GetRepeat(libinput_device *dev);

  bool SetKeymap(const std::string& layout);

private:
  XBMCKey XBMCKeyForKeysym(xkb_keysym_t sym, uint32_t scancode);
  void KeyRepeatTimeout();

  xkb_context *m_ctx = nullptr;
  xkb_keymap *m_keymap = nullptr;
  xkb_state *m_state = nullptr;
  xkb_mod_index_t m_modindex[4];
  xkb_led_index_t m_ledindex[3];

  int m_leds;

  XBMC_Event m_repeatEvent;
  std::map<libinput_device*, std::vector<int>> m_repeatData;
  CTimer m_repeatTimer;
  int m_repeatRate;
};
