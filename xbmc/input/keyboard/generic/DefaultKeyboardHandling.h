/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"

#include <memory>

namespace PERIPHERALS
{
class CPeripheral;
} // namespace PERIPHERALS

namespace KODI
{
namespace JOYSTICK
{
class IButtonMap;
} // namespace JOYSTICK

namespace KEYBOARD
{
class IKeyboardInputHandler;

class CDefaultKeyboardHandling : public IKeyboardDriverHandler
{
public:
  CDefaultKeyboardHandling(PERIPHERALS::CPeripheral* peripheral, IKeyboardInputHandler* handler);

  ~CDefaultKeyboardHandling() override;

  bool Load();

  // Implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override;

private:
  // Construction parameters
  PERIPHERALS::CPeripheral* const m_peripheral;
  IKeyboardInputHandler* const m_inputHandler;

  // Input parameters
  std::unique_ptr<IKeyboardDriverHandler> m_driverHandler;
  std::unique_ptr<JOYSTICK::IButtonMap> m_buttonMap;
};
} // namespace KEYBOARD
} // namespace KODI
