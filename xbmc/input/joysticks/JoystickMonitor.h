/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "IInputHandler.h"

namespace JOYSTICK
{
  /*!
   * \ingroup joystick
   * \brief Monitors joystick input and resets screensaver/shutdown timers
   *        whenever motion occurs.
   */
  class CJoystickMonitor : public IInputHandler
  {
  public:
    // implementation of IInputHandler
    virtual std::string ControllerID() const override;
    virtual bool HasFeature(const FeatureName& feature) const override { return true; }
    virtual bool AcceptsInput(void) override;
    virtual INPUT_TYPE GetInputType(const FeatureName& feature) const override { return INPUT_TYPE::ANALOG; }
    virtual bool OnButtonPress(const FeatureName& feature, bool bPressed) override;
    virtual void OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs) override { }
    virtual bool OnButtonMotion(const FeatureName& feature, float magnitude) override;
    virtual bool OnAnalogStickMotion(const FeatureName& feature, float x, float y, unsigned int motionTimeMs) override;
    virtual bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z) override { return false; }

  private:
    /*!
     * \brief  Reset screensaver and shutdown timers
     * \return True if the application was woken from screensaver
     */
    bool ResetTimers(void);
  };
}
