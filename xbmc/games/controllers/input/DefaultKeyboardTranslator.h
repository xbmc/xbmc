/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/KeyboardTypes.h"

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 */
class CDefaultKeyboardTranslator
{
public:
  /*!
   * \brief Translate a Kodi key code to a keyboard key name in a controller profile
   *
   * \param keycode The Kodi key code
   *
   * \return The key's name, or an empty string if no symbol is defined for the keycode
   */
  static const char* TranslateKeycode(KEYBOARD::XBMCKey keycode);
};

} // namespace GAME
} // namespace KODI
