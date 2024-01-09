/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/mouse/interfaces/IMouseDriverHandler.h"

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

namespace MOUSE
{
class IMouseInputHandler;

class CDefaultMouseHandling : public IMouseDriverHandler
{
public:
  CDefaultMouseHandling(PERIPHERALS::CPeripheral* peripheral, IMouseInputHandler* handler);

  ~CDefaultMouseHandling() override;

  bool Load();

  // Implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(BUTTON_ID button) override;
  void OnButtonRelease(BUTTON_ID button) override;

private:
  // Construction parameters
  PERIPHERALS::CPeripheral* const m_peripheral;
  IMouseInputHandler* const m_inputHandler;

  // Input parameters
  std::unique_ptr<IMouseDriverHandler> m_driverHandler;
  std::unique_ptr<JOYSTICK::IButtonMap> m_buttonMap;
};
} // namespace MOUSE
} // namespace KODI
