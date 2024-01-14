/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"

namespace KODI
{
namespace JOYSTICK
{
class IButtonMap;
}

namespace KEYBOARD
{
class IKeyboardInputHandler;

/*!
 * \ingroup keyboard
 *
 * \brief Class to translate input from Kodi keycodes to key names defined
 *        by the keyboard's controller profile
 */
class CKeyboardInputHandling : public IKeyboardDriverHandler
{
public:
  CKeyboardInputHandling(IKeyboardInputHandler* handler, JOYSTICK::IButtonMap* buttonMap);

  ~CKeyboardInputHandling(void) override = default;

  // implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override;

private:
  // Construction parameters
  IKeyboardInputHandler* const m_handler;
  JOYSTICK::IButtonMap* const m_buttonMap;
};
} // namespace KEYBOARD
} // namespace KODI
