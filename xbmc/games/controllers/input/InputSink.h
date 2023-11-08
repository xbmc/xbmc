/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IInputHandler.h"

namespace KODI
{
namespace GAME
{
class CGameClient;

/*!
 * \ingroup games
 */
class CInputSink : public JOYSTICK::IInputHandler
{
public:
  explicit CInputSink(JOYSTICK::IInputHandler* gameInput);

  ~CInputSink() override = default;

  // Implementation of IInputHandler
  std::string ControllerID() const override;
  bool HasFeature(const std::string& feature) const override { return true; }
  bool AcceptsInput(const std::string& feature) const override;
  bool OnButtonPress(const std::string& feature, bool bPressed) override;
  void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override {}
  bool OnButtonMotion(const std::string& feature,
                      float magnitude,
                      unsigned int motionTimeMs) override;
  bool OnAnalogStickMotion(const std::string& feature,
                           float x,
                           float y,
                           unsigned int motionTimeMs) override;
  bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;
  bool OnWheelMotion(const std::string& feature,
                     float position,
                     unsigned int motionTimeMs) override;
  bool OnThrottleMotion(const std::string& feature,
                        float position,
                        unsigned int motionTimeMs) override;
  void OnInputFrame() override {}

private:
  // Construction parameters
  JOYSTICK::IInputHandler* m_gameInput;
};
} // namespace GAME
} // namespace KODI
