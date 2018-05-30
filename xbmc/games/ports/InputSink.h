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

#include "input/joysticks/interfaces/IInputHandler.h"

namespace KODI
{
namespace GAME
{
  class CGameClient;

  class CInputSink : public JOYSTICK::IInputHandler
  {
  public:
    explicit CInputSink(JOYSTICK::IInputHandler* gameInput);

    virtual ~CInputSink() = default;

    // Implementation of IInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool HasFeature(const std::string& feature) const override { return true; }
    virtual bool AcceptsInput(const std::string& feature) const override;
    virtual bool OnButtonPress(const std::string& feature, bool bPressed) override;
    virtual void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override { }
    virtual bool OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs) override;
    virtual bool OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs) override;
    virtual bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;
    virtual bool OnWheelMotion(const std::string& feature, float position, unsigned int motionTimeMs) override;
    virtual bool OnThrottleMotion(const std::string& feature, float position, unsigned int motionTimeMs) override;

  private:
    // Construction parameters
    JOYSTICK::IInputHandler* m_gameInput;
  };
}
}
