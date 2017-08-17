/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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

#include <atomic>
#include <cstdint>
#include <memory>

#include <wayland-client-protocol.hpp>

#include "input/XBMC_keysym.h"
#include "threads/Timer.h"
#include "windowing/XBMC_events.h"
#include "XkbcommonKeymap.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class IInputHandlerKeyboard
{
public:
  virtual void OnKeyboardEnter() {}
  virtual void OnKeyboardLeave() {}
  virtual void OnKeyboardEvent(XBMC_Event& event) = 0;
  virtual ~IInputHandlerKeyboard() = default;
};

class CInputProcessorKeyboard
{
public:
  CInputProcessorKeyboard(wayland::keyboard_t const& keyboard, IInputHandlerKeyboard& handler);

private:
  CInputProcessorKeyboard(CInputProcessorKeyboard const& other) = delete;
  CInputProcessorKeyboard& operator=(CInputProcessorKeyboard const& other) = delete;

  void ConvertAndSendKey(std::uint32_t scancode, bool pressed);
  XBMC_Event SendKey(unsigned char scancode, XBMCKey key, std::uint16_t unicodeCodepoint, bool pressed);
  void KeyRepeatTimeout();

  wayland::keyboard_t m_keyboard;
  IInputHandlerKeyboard& m_handler;

  std::unique_ptr<CXkbcommonContext> m_xkbContext;
  std::unique_ptr<CXkbcommonKeymap> m_keymap;
  // Default values are used if compositor does not send any
  std::atomic<int> m_keyRepeatDelay{1000};
  std::atomic<int> m_keyRepeatInterval{50};
  // Save complete XBMC_Event so no keymap lookups which might not be thread-safe
  // are needed in the repeat callback
  XBMC_Event m_keyToRepeat;

  CTimer m_keyRepeatTimer;
};

}
}
}
