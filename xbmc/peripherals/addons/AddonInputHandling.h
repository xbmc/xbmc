/*
 *      Copyright (C) 2014-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
}

namespace KEYBOARD
{
  class IKeyboardInputHandler;
}

namespace MOUSE
{
  class IMouseInputHandler;
}
}

namespace PERIPHERALS
{
  class CPeripheral;
  class CPeripherals;

  class CAddonInputHandling : public KODI::JOYSTICK::IDriverHandler,
                              public KODI::JOYSTICK::IInputReceiver,
                              public KODI::KEYBOARD::IKeyboardDriverHandler,
                              public KODI::MOUSE::IMouseDriverHandler
  {
  public:
    CAddonInputHandling(CPeripherals& manager,
                        CPeripheral* peripheral,
                        KODI::JOYSTICK::IInputHandler* handler,
                        KODI::JOYSTICK::IDriverReceiver* receiver);

    CAddonInputHandling(CPeripherals& manager,
                        CPeripheral* peripheral,
                        KODI::KEYBOARD::IKeyboardInputHandler* handler);

    CAddonInputHandling(CPeripherals& manager,
                        CPeripheral* peripheral,
                        KODI::MOUSE::IMouseInputHandler* handler);

    ~CAddonInputHandling(void) override;

    // implementation of IDriverHandler
    bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
    bool OnHatMotion(unsigned int hatIndex, KODI::JOYSTICK::HAT_STATE state) override;
    bool OnAxisMotion(unsigned int axisIndex, float position, int center, unsigned int range) override;
    void ProcessAxisMotions(void) override;

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
    std::unique_ptr<KODI::JOYSTICK::IDriverHandler> m_driverHandler;
    std::unique_ptr<KODI::JOYSTICK::IInputReceiver> m_inputReceiver;
    std::unique_ptr<KODI::KEYBOARD::IKeyboardDriverHandler> m_keyboardHandler;
    std::unique_ptr<KODI::MOUSE::IMouseDriverHandler> m_mouseHandler;
    std::unique_ptr<KODI::JOYSTICK::IButtonMap>     m_buttonMap;
  };
}
