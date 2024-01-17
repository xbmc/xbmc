/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IButtonMapCallback.h"
#include "input/joysticks/interfaces/IDriverHandler.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"

#include <memory>

namespace KODI
{
namespace JOYSTICK
{
class CButtonMapping;
class IButtonMap;
class IButtonMapper;
} // namespace JOYSTICK
} // namespace KODI

namespace PERIPHERALS
{
class CPeripheral;
class CPeripherals;

/*!
* \ingroup peripherals
*/
class CAddonButtonMapping : public KODI::JOYSTICK::IDriverHandler,
                            public KODI::KEYBOARD::IKeyboardDriverHandler,
                            public KODI::MOUSE::IMouseDriverHandler,
                            public KODI::JOYSTICK::IButtonMapCallback
{
public:
  CAddonButtonMapping(CPeripherals& manager,
                      CPeripheral* peripheral,
                      KODI::JOYSTICK::IButtonMapper* mapper);

  ~CAddonButtonMapping(void) override;

  // implementation of IDriverHandler
  bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
  bool OnHatMotion(unsigned int hatIndex, KODI::JOYSTICK::HAT_STATE state) override;
  bool OnAxisMotion(unsigned int axisIndex,
                    float position,
                    int center,
                    unsigned int range) override;
  void OnInputFrame(void) override;

  // implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override;

  // implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(KODI::MOUSE::BUTTON_ID button) override;
  void OnButtonRelease(KODI::MOUSE::BUTTON_ID button) override;

  // implementation of IButtonMapCallback
  void SaveButtonMap() override;
  void ResetIgnoredPrimitives() override;
  void RevertButtonMap() override;

private:
  std::unique_ptr<KODI::JOYSTICK::CButtonMapping> m_buttonMapping;
  std::unique_ptr<KODI::JOYSTICK::IButtonMap> m_buttonMap;
};
} // namespace PERIPHERALS
