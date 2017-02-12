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

#include "input/joysticks/IButtonMapCallback.h"
#include "input/joysticks/IDriverHandler.h"

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

  class CAddonButtonMapping : public KODI::JOYSTICK::IDriverHandler,
                              public KODI::JOYSTICK::IButtonMapCallback
  {
  public:
    CAddonButtonMapping(CPeripheral* peripheral, KODI::JOYSTICK::IButtonMapper* mapper);

    virtual ~CAddonButtonMapping(void);

    // implementation of IDriverHandler
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
    virtual bool OnHatMotion(unsigned int hatIndex, KODI::JOYSTICK::HAT_STATE state) override;
    virtual bool OnAxisMotion(unsigned int axisIndex, float position, int center, unsigned int range) override;
    virtual void ProcessAxisMotions(void) override;

    // implementation of IButtonMapCallback
    virtual void SaveButtonMap() override;
    virtual void ResetIgnoredPrimitives() override;
    virtual void RevertButtonMap() override;

  private:
    std::unique_ptr<KODI::JOYSTICK::CButtonMapping> m_buttonMapping;
    std::unique_ptr<KODI::JOYSTICK::IButtonMap>     m_buttonMap;
  };
}
