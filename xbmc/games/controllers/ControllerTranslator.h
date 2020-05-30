/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"
#include "input/keyboard/KeyboardTypes.h"

#include <string>

namespace KODI
{
namespace GAME
{

class CControllerTranslator
{
public:
  static const char* TranslateFeatureType(JOYSTICK::FEATURE_TYPE type);
  static JOYSTICK::FEATURE_TYPE TranslateFeatureType(const std::string& strType);

  static const char* TranslateFeatureCategory(JOYSTICK::FEATURE_CATEGORY category);
  static JOYSTICK::FEATURE_CATEGORY TranslateFeatureCategory(const std::string& strCategory);

  static const char* TranslateInputType(JOYSTICK::INPUT_TYPE type);
  static JOYSTICK::INPUT_TYPE TranslateInputType(const std::string& strType);

  /*!
   * \brief Translate a keyboard symbol to a Kodi key code
   *
   * \param symbol The key's symbol, defined in the kodi-game-controllers project
   *
   * \return The layout-independent keycode associated with the key
   */
  static KEYBOARD::KeySymbol TranslateKeysym(const std::string& symbol);

  /*!
   * \brief Translate a Kodi key code to a keyboard symbol
   *
   * \param keycode The Kodi key code
   *
   * \return The key's symbol, or an empty string if no symbol is defined for the keycode
   */
  static const char* TranslateKeycode(KEYBOARD::KeySymbol keycode);
};

} // namespace GAME
} // namespace KODI
