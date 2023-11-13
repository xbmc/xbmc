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
#include <memory>
#include <vector>

#include <libinput.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>

class CLibInputKeyboard
{
public:
  CLibInputKeyboard();
  ~CLibInputKeyboard() = default;

  void ProcessKey(libinput_event_keyboard *e);
  void UpdateLeds(libinput_device *dev);
  void GetRepeat(libinput_device *dev);

  bool SetKeymap(const std::string& layout);

private:
  XBMCKey XBMCKeyForKeysym(xkb_keysym_t sym, uint32_t scancode);
  void KeyRepeatTimeout();
  /**
   * Check if the system supports key composition
   * \return true if composition is supported, false otherwise
   */
  bool SupportsKeyComposition() const;
  /**
   * Notify the outside world about key composing events
   *
   * \param eventType - the key composition event type
   * \param unicodeCodepoint - unicode codepoint of the pressed dead key
   */
  void NotifyKeyComposingEvent(uint8_t eventType, std::uint16_t unicodeCodepoint);
  /**
   * Get Unicode codepoint/UTF32 code for provided keycode
   */
  std::uint32_t UnicodeCodepointForKeycode(xkb_keycode_t code) const;
  struct XkbContextDeleter
  {
    void operator()(xkb_context* ctx) const;
  };
  std::unique_ptr<xkb_context, XkbContextDeleter> m_ctx;

  struct XkbKeymapDeleter
  {
    void operator()(xkb_keymap* keymap) const;
  };
  std::unique_ptr<xkb_keymap, XkbKeymapDeleter> m_keymap;

  struct XkbStateDeleter
  {
    void operator()(xkb_state* state) const;
  };
  std::unique_ptr<xkb_state, XkbStateDeleter> m_state;

  struct XkbComposeTableDeleter
  {
    void operator()(xkb_compose_table* composeTable) const;
  };
  std::unique_ptr<xkb_compose_table, XkbComposeTableDeleter> m_composeTable;

  struct XkbComposeStateDeleter
  {
    void operator()(xkb_compose_state* state) const;
  };
  std::unique_ptr<xkb_compose_state, XkbComposeStateDeleter> m_composedState;

  xkb_mod_index_t m_modindex[4];
  xkb_led_index_t m_ledindex[3];

  int m_leds;

  XBMC_Event m_repeatEvent;
  std::map<libinput_device*, std::vector<int>> m_repeatData;
  CTimer m_repeatTimer;
  int m_repeatRate;
};
