/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "input/joysticks/IDriverHandler.h"
#include "input/joysticks/IInputReceiver.h"

#include <memory>

namespace JOYSTICK
{
  class IButtonMap;
  class IDriverReceiver;
  class IInputHandler;
}

namespace PERIPHERALS
{
  class CPeripheral;

  class CAddonInputHandling : public JOYSTICK::IDriverHandler,
                              public JOYSTICK::IInputReceiver
  {
  public:
    CAddonInputHandling(CPeripheral* peripheral, JOYSTICK::IInputHandler* handler, JOYSTICK::IDriverReceiver* receiver);

    virtual ~CAddonInputHandling(void);

    // implementation of IDriverHandler
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
    virtual bool OnHatMotion(unsigned int hatIndex, JOYSTICK::HAT_STATE state) override;
    virtual bool OnAxisMotion(unsigned int axisIndex, float position) override;
    virtual void ProcessAxisMotions(void) override;

    // implementation of IInputReceiver
    virtual bool SetRumbleState(const JOYSTICK::FeatureName& feature, float magnitude) override;

  private:
    std::unique_ptr<JOYSTICK::IDriverHandler> m_driverHandler;
    std::unique_ptr<JOYSTICK::IInputReceiver> m_inputReceiver;
    std::unique_ptr<JOYSTICK::IButtonMap>     m_buttonMap;
  };
}
