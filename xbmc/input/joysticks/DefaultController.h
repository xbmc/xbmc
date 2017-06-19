/*
 *      Copyright (C) 2017 Team Kodi
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

#include "DefaultJoystick.h"
#include "RumbleGenerator.h"

namespace KODI
{
namespace JOYSTICK
{
  class CDefaultController : public CDefaultJoystick
  {
  public:
    CDefaultController(void);

    virtual ~CDefaultController(void) = default;

    // Forward rumble commands to rumble generator
    void NotifyUser(void) { m_rumbleGenerator.NotifyUser(InputReceiver()); }
    bool TestRumble(void) { return m_rumbleGenerator.DoTest(InputReceiver()); }
    void AbortRumble() { m_rumbleGenerator.AbortRumble(); }

  protected:
    // implementation of CDefaultJoystick
    virtual std::string GetControllerID() const;
    virtual unsigned int GetKeyID(const FeatureName& feature, ANALOG_STICK_DIRECTION dir = ANALOG_STICK_DIRECTION::UNKNOWN) const override;

  private:
    // Rumble functionality
    CRumbleGenerator m_rumbleGenerator;
  };
}
}
