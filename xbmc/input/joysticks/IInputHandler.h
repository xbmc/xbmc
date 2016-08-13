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

#include "JoystickTypes.h"

#include <string>

namespace JOYSTICK
{
  class IInputReceiver;

  /*!
   * \brief Interface for handling input events for game controllers
   */
  class IInputHandler
  {
  public:
    IInputHandler(void) : m_receiver(nullptr) { }

    virtual ~IInputHandler(void) { }

    /*!
     * \brief The add-on ID of the game controller associated with this input handler
     *
     * \return The ID of the add-on extending kodi.game.controller
     */
    virtual std::string ControllerID(void) const = 0;

    /*!
     * \brief Return true if the input handler accepts the given feature
     *
     * \param feature A feature belonging to the controller specified by ControllerID()
     *
     * \return True if the feature is used for input, false otherwise
     */
    virtual bool HasFeature(const FeatureName& feature) const = 0;

    /*!
     * \brief Return true if the input handler is currently accepting input
     */
    virtual bool AcceptsInput(void) = 0;

    /*!
     * \brief Get the type of input handled by the specified feature
     *
     * \return INPUT_TYPE::DIGITAL for digital buttons, INPUT::ANALOG for analog
     *         buttons, or INPUT::UNKNOWN otherwise
     */
    virtual INPUT_TYPE GetInputType(const FeatureName& feature) const = 0;

    /*!
     * \brief A digital button has been pressed or released
     *
     * \param feature      The feature being pressed
     * \param bPressed     True if pressed, false if released
     *
     * \return True if the event was handled otherwise false
     */
    virtual bool OnButtonPress(const FeatureName& feature, bool bPressed) = 0;

    /*!
    * \brief A digital button has been pressed for more than one event frame
    *
    * \param feature      The feature being held
    * \param holdTimeMs   The time elapsed since the initial press (ms)
    *
    * If OnButtonPress() returns true for the initial press, then this callback
    * is invoked on subsequent frames until the button is released.
    */
    virtual void OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs) = 0;

    /*!
     * \brief An analog button (trigger or a pressure-sensitive button) has changed state
     *
     * \param feature      The feature changing state
     * \param magnitude    The button pressure or trigger travel distance in the
     *                     closed interval [0, 1]
     *
     * \return True if the event was handled otherwise false
     */
    virtual bool OnButtonMotion(const FeatureName& feature, float magnitude) = 0;

    /*!
     * \brief An analog stick has moved
     *
     * \param feature      The analog stick being moved
     * \param x            The x coordinate in the closed interval [-1, 1]
     * \param y            The y coordinate in the closed interval [-1, 1]
     * \param motionTimeMs The time elapsed since this analog stick was centered,
     *                     or 0 if the analog stick is centered
     *
     * \return True if the event was handled otherwise false
     */
    virtual bool OnAnalogStickMotion(const FeatureName& feature, float x, float y, unsigned int motionTimeMs = 0) = 0;

    /*!
     * \brief An accelerometer's state has changed
     *
     * \param feature      The accelerometer being accelerated
     * \param x            The x coordinate in the closed interval [-1, 1]
     * \param y            The y coordinate in the closed interval [-1, 1]
     * \param z            The z coordinate in the closed interval [-1, 1]
     *
     * \return True if the event was handled otherwise false
     */
    virtual bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z) { return false; }

    // Input receiver interface
    void SetInputReceiver(IInputReceiver* receiver) { m_receiver = receiver; }
    void ResetInputReceiver(void) { m_receiver = nullptr; }
    IInputReceiver* InputReceiver(void) { return m_receiver; }

  private:
    IInputReceiver* m_receiver;
  };
}
