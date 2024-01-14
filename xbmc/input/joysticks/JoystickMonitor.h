/*
 *  Copyright (C) 2015-2024 Team Kodi
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
 *
 * \brief Monitors joystick input and resets screensaver/shutdown timers
 *        whenever motion occurs.
 */
class CJoystickMonitor : public IInputHandler
{
public:
  // implementation of IInputHandler
  std::string ControllerID() const override;
  bool HasFeature(const FeatureName& feature) const override { return true; }
  bool AcceptsInput(const FeatureName& feature) const override;
  bool OnButtonPress(const FeatureName& feature, bool bPressed) override;
  void OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs) override {}
  bool OnButtonMotion(const FeatureName& feature,
                      float magnitude,
                      unsigned int motionTimeMs) override;
  bool OnAnalogStickMotion(const FeatureName& feature,
                           float x,
                           float y,
                           unsigned int motionTimeMs) override;
  bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z) override
  {
    return false;
  }
  bool OnWheelMotion(const FeatureName& feature,
                     float position,
                     unsigned int motionTimeMs) override;
  bool OnThrottleMotion(const FeatureName& feature,
                        float position,
                        unsigned int motionTimeMs) override;
  void OnInputFrame() override {}

private:
  /*!
   * \brief  Reset screensaver and shutdown timers
   * \return True if the application was woken from screensaver
   */
  bool ResetTimers(void);
};
} // namespace JOYSTICK
} // namespace KODI
