/*
 *  Copyright (C) 2007-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//
// C++ Interface: CKeyboard
//
// Description: Adds features like international keyboard layout mapping on top of the
// platform specific low level keyboard classes.
// Here it must be done only once. Within the other mentioned classes it would have to be done
// several times.
//
// Keyboards always deliver printable characters, logical keys for functional behaviour, modifiers
// ... alongside Based on the same hardware with the same scancodes (also alongside) but delivered
// with different labels to customers the software must solve the mapping to the real labels. This
// is done here. The mapping must be specified by an xml configuration that should be able to access
// everything available, but this allows for double/redundant or ambiguous mapping definition, e.g.
// ASCII/unicode could be derived from scancodes, virtual keys, modifiers and/or other
// ASCII/unicode.

#include "input/keyboard/Key.h"
#include "input/keyboard/XBMC_keyboard.h"

#include <chrono>
#include <string>

namespace KODI
{
namespace KEYBOARD
{
/*!
 * \ingroup keyboard
 */
class CKeyboardStat
{
public:
  CKeyboardStat();
  ~CKeyboardStat() = default;

  void Initialize();

  CKey TranslateKey(XBMC_keysym& keysym) const;

  void ProcessKeyDown(XBMC_keysym& keysym);
  void ProcessKeyUp(void);

  std::string GetKeyName(int KeyID);

private:
  static bool LookupSymAndUnicodePeripherals(XBMC_keysym& keysym, uint8_t* key, char* unicode);

  XBMC_keysym m_lastKeysym;
  std::chrono::time_point<std::chrono::steady_clock> m_lastKeyTime;
};
} // namespace KEYBOARD
} // namespace KODI
