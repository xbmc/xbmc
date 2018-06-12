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
}
}

namespace PERIPHERALS
{
  class CPeripheral;
  class CPeripherals;

  class CAddonButtonMapping : public KODI::JOYSTICK::IDriverHandler,
                              public KODI::KEYBOARD::IKeyboardDriverHandler,
                              public KODI::MOUSE::IMouseDriverHandler,
                              public KODI::JOYSTICK::IButtonMapCallback
  {
  public:
    CAddonButtonMapping(CPeripherals& manager, CPeripheral* peripheral, KODI::JOYSTICK::IButtonMapper* mapper);

    ~CAddonButtonMapping(void) override;

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

    // implementation of IButtonMapCallback
    void SaveButtonMap() override;
    void ResetIgnoredPrimitives() override;
    void RevertButtonMap() override;

  private:
    std::unique_ptr<KODI::JOYSTICK::CButtonMapping> m_buttonMapping;
    std::unique_ptr<KODI::JOYSTICK::IButtonMap>     m_buttonMap;
  };
}
