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
#include "input/KeymapEnvironment.h"

#include <memory>

namespace KODI
{
namespace JOYSTICK
{
  class CKeymapHandling;
  class IInputProvider;
}

namespace GAME
{
  class CPort : public JOYSTICK::IInputHandler,
                public IKeymapEnvironment
  {
  public:
    CPort(JOYSTICK::IInputHandler* gameInput);
    ~CPort() override;

    void RegisterInput(JOYSTICK::IInputProvider *provider);
    void UnregisterInput(JOYSTICK::IInputProvider *provider);

    JOYSTICK::IInputHandler *InputHandler() { return m_gameInput; }

    // Implementation of IInputHandler
    virtual std::string ControllerID() const override;
    virtual bool HasFeature(const std::string& feature) const override { return true; }
    virtual bool AcceptsInput(const std::string& feature) const override;
    virtual bool OnButtonPress(const std::string& feature, bool bPressed) override;
    virtual void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override;
    virtual bool OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs) override;
    virtual bool OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs) override;
    virtual bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;
    virtual bool OnWheelMotion(const std::string& feature, float position, unsigned int motionTimeMs) override;
    virtual bool OnThrottleMotion(const std::string& feature, float position, unsigned int motionTimeMs) override;

    // Implementation of IKeymapEnvironment
    virtual int GetWindowID() const override;
    virtual void SetWindowID(int windowId) override { }
    virtual int GetFallthrough(int windowId) const override { return -1; }
    virtual bool UseGlobalFallthrough() const override { return false; }
    virtual bool UseEasterEgg() const override { return false; }

  private:
    // Construction parameters
    JOYSTICK::IInputHandler* const m_gameInput;

    // Handles input to Kodi
    std::unique_ptr<JOYSTICK::CKeymapHandling> m_appInput;

    // Prevents input falling through to Kodi when not handled by the game
    std::unique_ptr<JOYSTICK::IInputHandler> m_inputSink;
  };
}
}
