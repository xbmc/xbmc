/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Seat.h"
#include "XkbcommonKeymap.h"
#include "input/keyboard/XBMC_keysym.h"
#include "threads/Timer.h"
#include "windowing/XBMC_events.h"

#include <atomic>
#include <cstdint>
#include <memory>

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

class CInputProcessorKeyboard final : public IRawInputHandlerKeyboard
{
public:
  CInputProcessorKeyboard(IInputHandlerKeyboard& handler);

  void OnKeyboardKeymap(CSeat* seat, wayland::keyboard_keymap_format format, std::string const& keymap) override;
  void OnKeyboardEnter(CSeat* seat,
                       std::uint32_t serial,
                       const wayland::surface_t& surface,
                       const wayland::array_t& keys) override;
  void OnKeyboardLeave(CSeat* seat,
                       std::uint32_t serial,
                       const wayland::surface_t& surface) override;
  void OnKeyboardKey(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::uint32_t key, wayland::keyboard_key_state state) override;
  void OnKeyboardModifiers(CSeat* seat, std::uint32_t serial, std::uint32_t modsDepressed, std::uint32_t modsLatched, std::uint32_t modsLocked, std::uint32_t group) override;
  void OnKeyboardRepeatInfo(CSeat* seat, std::int32_t rate, std::int32_t delay) override;

private:
  CInputProcessorKeyboard(CInputProcessorKeyboard const& other) = delete;
  CInputProcessorKeyboard& operator=(CInputProcessorKeyboard const& other) = delete;

  void ConvertAndSendKey(std::uint32_t scancode, bool pressed);
  XBMC_Event SendKey(unsigned char scancode, XBMCKey key, std::uint16_t unicodeCodepoint, bool pressed);
  /**
   * Notify the outside world about key composing events
   *
   * \param eventType - the key composition event type
   * \param unicodeCodepoint - unicode codepoint of the pressed dead key
   */
  void NotifyKeyComposingEvent(uint8_t eventType, std::uint16_t unicodeCodepoint);
  void KeyRepeatTimeout();

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
