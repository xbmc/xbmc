/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "guilib/GUIKeyboard.h"

#include <cstdint>
#include <string>

namespace KODI::WINDOWING::WASM
{
// Text entry through a DOM <input> on the browser main thread, which is what
// brings up the browser's or the TV's own on-screen keyboard.
class CWasmKeyboard : public CGUIKeyboard
{
public:
  CWasmKeyboard() = default;
  ~CWasmKeyboard() override;

  bool ShowAndGetInput(char_callback_t pCallback,
                       const std::string& initialString,
                       std::string& typedString,
                       const std::string& heading,
                       bool bHiddenInput) override;
  void Cancel() override;
  bool SetTextToKeyboard(const std::string& text, bool closeKeyboard) override;

  // True while a native keyboard owns key events; CWinEventsWasm drops them then.
  static bool IsActive();

  // Written by the main thread with Atomics; `signal` is bumped and notified
  // on every change so ShowAndGetInput can futex-wait on it.
  struct Shared
  {
    int32_t signal;
    int32_t state;
    int32_t textLength;
    char text[4096];
  };

private:
  enum State : int32_t
  {
    IDLE = 0,
    SHOWN = 1,
    CONFIRMED = 2,
    CANCELLED = 3,
  };

  static void InstallJs();
  void Hide();

  Shared m_shared{};
  bool m_shown{false};
};
} // namespace KODI::WINDOWING::WASM
