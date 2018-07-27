/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IInputHandler.h"

namespace KODI
{
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
    virtual bool AcceptsInput(const FeatureName& feature) const override;
    virtual bool OnButtonPress(const FeatureName& feature, bool bPressed) override;
    virtual void OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs) override { }
    virtual bool OnButtonMotion(const FeatureName& feature, float magnitude, unsigned int motionTimeMs) override;
    virtual bool OnAnalogStickMotion(const FeatureName& feature, float x, float y, unsigned int motionTimeMs) override;
    virtual bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z) override { return false; }
    virtual bool OnWheelMotion(const FeatureName& feature, float position, unsigned int motionTimeMs) override;
    virtual bool OnThrottleMotion(const FeatureName& feature, float position, unsigned int motionTimeMs) override;

  private:
    /*!
     * \brief  Reset screensaver and shutdown timers
     * \return True if the application was woken from screensaver
     */
    bool ResetTimers(void);
  };
}
}
