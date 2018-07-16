/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
