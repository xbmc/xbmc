/*
 *      Copyright (C) 2005-2017 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "threads/Timer.h"
#include "windowing/XBMC_events.h"

#include <libinput.h>
#include <map>
#include <vector>
#include <xkbcommon/xkbcommon.h>

class CLibInputKeyboard
{
public:
  CLibInputKeyboard();
  ~CLibInputKeyboard();

  void ProcessKey(libinput_event_keyboard *e);
  void UpdateLeds(libinput_device *dev);
  void GetRepeat(libinput_device *dev);

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
