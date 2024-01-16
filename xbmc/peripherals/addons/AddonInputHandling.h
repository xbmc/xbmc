/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IDriverHandler.h"
#include "input/joysticks/interfaces/IInputReceiver.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"

#include <memory>

namespace KODI
{
namespace JOYSTICK
{
class IButtonMap;
class IDriverReceiver;
class IInputHandler;
} // namespace JOYSTICK

namespace KEYBOARD
{
class IKeyboardInputHandler;
}

namespace MOUSE
{
class IMouseInputHandler;
}
} // namespace KODI

namespace PERIPHERALS
{
class CPeripheral;
class CPeripheralAddon;

/*!
* \ingroup peripherals
*/
class CAddonInputHandling : public KODI::JOYSTICK::IDriverHandler,
                            public KODI::JOYSTICK::IInputReceiver,
                            public KODI::KEYBOARD::IKeyboardDriverHandler,
                            public KODI::MOUSE::IMouseDriverHandler
{
public:
  CAddonInputHandling(CPeripheral* peripheral,
                      std::shared_ptr<CPeripheralAddon> addon,
                      KODI::JOYSTICK::IInputHandler* handler,
                      KODI::JOYSTICK::IDriverReceiver* receiver);

  CAddonInputHandling(CPeripheral* peripheral,
                      std::shared_ptr<CPeripheralAddon> addon,
                      KODI::KEYBOARD::IKeyboardInputHandler* handler);

  CAddonInputHandling(CPeripheral* peripheral,
                      std::shared_ptr<CPeripheralAddon> addon,
                      KODI::MOUSE::IMouseInputHandler* handler);

  ~CAddonInputHandling(void) override;

  bool Load();

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

  // implementation of IInputReceiver
  bool SetRumbleState(const KODI::JOYSTICK::FeatureName& feature, float magnitude) override;

private:
  // Construction parameters
  CPeripheral* const m_peripheral;
  const std::shared_ptr<CPeripheralAddon> m_addon;
  KODI::JOYSTICK::IInputHandler* const m_joystickInputHandler{nullptr};
  KODI::JOYSTICK::IDriverReceiver* const m_joystickDriverReceiver{nullptr};
  KODI::KEYBOARD::IKeyboardInputHandler* m_keyboardInputHandler{nullptr};
  KODI::MOUSE::IMouseInputHandler* const m_mouseInputHandler{nullptr};

  // Input parameters
  std::unique_ptr<KODI::JOYSTICK::IDriverHandler> m_joystickDriverHandler;
  std::unique_ptr<KODI::JOYSTICK::IInputReceiver> m_joystickInputReceiver;
  std::unique_ptr<KODI::KEYBOARD::IKeyboardDriverHandler> m_keyboardDriverHandler;
  std::unique_ptr<KODI::MOUSE::IMouseDriverHandler> m_mouseDriverHandler;
  std::unique_ptr<KODI::JOYSTICK::IButtonMap> m_buttonMap;
};
} // namespace PERIPHERALS
