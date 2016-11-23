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

#include "IActionMap.h"
#include "IInputHandler.h"
#include "JoystickTypes.h"
#include "RumbleGenerator.h"

#include <map>
#include <vector>

#define DEFAULT_CONTROLLER_ID    "game.controller.default"

// Analog sticks on the default controller
#define DEFAULT_LEFT_STICK_NAME   "leftstick"
#define DEFAULT_RIGHT_STICK_NAME  "rightstick"

namespace JOYSTICK
{
  class IKeymapHandler;

  /*!
   * \ingroup joystick
   * \brief Implementation of IInputHandler for Kodi input
   *
   * \sa IInputHandler
   */
  class CDefaultJoystick : public IInputHandler,
                           public IActionMap
  {
  public:
    CDefaultJoystick(void);

    virtual ~CDefaultJoystick(void);

    // implementation of IInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool HasFeature(const FeatureName& feature) const override;
    virtual bool AcceptsInput(void) override;
    virtual INPUT_TYPE GetInputType(const FeatureName& feature) const override;
    virtual bool OnButtonPress(const FeatureName& feature, bool bPressed) override;
    virtual void OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs) override;
    virtual bool OnButtonMotion(const FeatureName& feature, float magnitude) override;
    virtual bool OnAnalogStickMotion(const FeatureName& feature, float x, float y, unsigned int motionTimeMs = 0) override;
    virtual bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z) override;

    // implementation of IActionMap
    virtual int GetActionID(const FeatureName& feature) override;

    // Forward rumble commands to rumble generator
    void NotifyUser(void) { m_rumbleGenerator.NotifyUser(InputReceiver()); }
    bool TestRumble(void) { return m_rumbleGenerator.DoTest(InputReceiver()); }
    void AbortRumble() { return m_rumbleGenerator.AbortRumble(); }

  private:
    bool ActivateDirection(const FeatureName& feature, float magnitude, ANALOG_STICK_DIRECTION dir, unsigned int motionTimeMs);
    void DeactivateDirection(const FeatureName& feature, ANALOG_STICK_DIRECTION dir);

    /*!
     * \brief Get the keymap key, as defined in Key.h, for the specified
     *        joystick feature/direction
     *
     * \param           feature The name of the feature on the default controller
     * \param[optional] dir     The direction (used for analog sticks)
     *
     * \return The key ID, or 0 if unknown
     */
    static unsigned int GetKeyID(const FeatureName& feature, ANALOG_STICK_DIRECTION dir = ANALOG_STICK_DIRECTION::UNKNOWN);

    /*!
     * \brief Return a vector of the four cardinal directions
     */
    static const std::vector<ANALOG_STICK_DIRECTION>& GetDirections(void);

    // Handler to process joystick input to Kodi actions
    IKeymapHandler* const m_handler;

    // State variables used to process joystick input
    std::map<unsigned int, unsigned int> m_holdStartTimes; // Key ID -> hold start time (ms)
    std::map<FeatureName, ANALOG_STICK_DIRECTION> m_currentDirections; // Analog stick name -> direction

    // Rumble functionality
    CRumbleGenerator m_rumbleGenerator;
  };
}
