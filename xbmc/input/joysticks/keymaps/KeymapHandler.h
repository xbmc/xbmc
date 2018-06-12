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

#include "input/joysticks/interfaces/IButtonSequence.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "input/joysticks/interfaces/IKeymapHandler.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>
#include <memory>
#include <string>

class IActionListener;
class IKeymap;

namespace KODI
{
namespace JOYSTICK
{
  class IKeyHandler;

  /*!
   * \ingroup joystick
   * \brief
   */
  class CKeymapHandler : public IKeymapHandler,
                         public IInputHandler
  {
  public:
    CKeymapHandler(IActionListener *actionHandler, const IKeymap *keymap);

    virtual ~CKeymapHandler() = default;

    // implementation of IKeymapHandler
    virtual bool HotkeysPressed(const std::set<std::string>& keyNames) const override;
    virtual std::string GetLastPressed() const override { return m_lastPressed; }
    virtual void OnPress(const std::string& keyName) override { m_lastPressed = keyName; }

    // implementation of IInputHandler
    virtual std::string ControllerID() const override;
    virtual bool HasFeature(const FeatureName& feature) const override { return true; }
    virtual bool AcceptsInput(const FeatureName& feature) const override;
    virtual bool OnButtonPress(const FeatureName& feature, bool bPressed) override;
    virtual void OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs) override;
    virtual bool OnButtonMotion(const FeatureName& feature, float magnitude, unsigned int motionTimeMs) override;
    virtual bool OnAnalogStickMotion(const FeatureName& feature, float x, float y, unsigned int motionTimeMs) override;
    virtual bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z) override;
    virtual bool OnWheelMotion(const FeatureName& feature, float position, unsigned int motionTimeMs) override;
    virtual bool OnThrottleMotion(const FeatureName& feature, float position, unsigned int motionTimeMs) override;

  protected:
    // Keep track of cheat code presses
    std::unique_ptr<IButtonSequence> m_easterEgg;

  private:
    // Analog stick helper functions
    bool ActivateDirection(const FeatureName& feature, float magnitude, ANALOG_STICK_DIRECTION dir, unsigned int motionTimeMs);
    void DeactivateDirection(const FeatureName& feature, ANALOG_STICK_DIRECTION dir);

    // Wheel helper functions
    bool ActivateDirection(const FeatureName& feature, float magnitude, WHEEL_DIRECTION dir, unsigned int motionTimeMs);
    void DeactivateDirection(const FeatureName& feature, WHEEL_DIRECTION dir);

    // Throttle helper functions
    bool ActivateDirection(const FeatureName& feature, float magnitude, THROTTLE_DIRECTION dir, unsigned int motionTimeMs);
    void DeactivateDirection(const FeatureName& feature, THROTTLE_DIRECTION dir);

    // Helper functions
    IKeyHandler *GetKeyHandler(const std::string &keyName);
    bool HasAction(const std::string &keyName) const;

    // Construction parameters
    IActionListener *const m_actionHandler;
    const IKeymap *const m_keymap;

    // Handlers for individual keys
    std::map<std::string, std::unique_ptr<IKeyHandler>> m_keyHandlers; // Key name -> handler

    // Last pressed key
    std::string m_lastPressed;
  };
}
}
