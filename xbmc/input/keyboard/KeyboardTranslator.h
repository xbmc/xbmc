/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "KeyboardSymbols.h"
#include "KeyboardTypes.h"
#include "XBMC_keysym.h"

namespace KODI
{
namespace KEYBOARD
{

/*!
 * \brief Keyboard translation utilities
 */
class CKeyboardTranslator
{
public:
  /*!
   * \brief Translate a keyboard symbol to a Kodi key code
   *
   * Keyboard symbols are hardware-independent virtual key representations.
   * They are used to help facilitate keyboard mapping.
   *
   * \param symbol The key's symbol, defined in the Controller Topology Project:
   *   https://github.com/kodi-game/controller-topology-project/blob/master/Readme-Keyboard.md
   *
   * \return The layout-independent keycode associated with the key
   */
  static XBMCKey TranslateKeysym(const SymbolName& symbolName);

  /*!
   * \brief Translate a Kodi key code to a keyboard symbol
   *
   * \param keycode The Kodi key code
   *
   * \return The key's symbol, or an empty string if no symbol is defined for the keycode
   */
  static const char* TranslateKeycode(XBMCKey keycode);
};

} // namespace KEYBOARD
} // namespace KODI
