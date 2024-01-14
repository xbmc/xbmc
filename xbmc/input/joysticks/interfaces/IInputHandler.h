/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"

#include <string>

namespace KODI
{
namespace JOYSTICK
{
class IInputReceiver;

/*!
 * \ingroup joystick
 *
 * \brief Interface for handling input events for game controllers
 */
class IInputHandler
{
public:
  virtual ~IInputHandler() = default;

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
   * \brief Return true if the input handler is currently accepting input for the
   *        given feature
   *
   * \param feature A feature belonging to the controller specified by ControllerID()
   *
   * \return True if the feature is currently accepting input, false otherwise
   *
   * This does not prevent the input events from being called, but can return
   * false to indicate that input wasn't handled for the specified feature.
   */
  virtual bool AcceptsInput(const FeatureName& feature) const = 0;

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
   * \param motionTimeMs The time elapsed since the magnitude was 0
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnButtonMotion(const FeatureName& feature,
                              float magnitude,
                              unsigned int motionTimeMs) = 0;

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
  virtual bool OnAnalogStickMotion(const FeatureName& feature,
                                   float x,
                                   float y,
                                   unsigned int motionTimeMs) = 0;

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
  virtual bool OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z)
  {
    return false;
  }

  /*!
   * \brief A wheel has changed state
   *
   * Left is negative position, right is positive position
   *
   * \param feature      The wheel changing state
   * \param position     The position in the closed interval [-1, 1]
   * \param motionTimeMs The time elapsed since the position was 0
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnWheelMotion(const FeatureName& feature,
                             float position,
                             unsigned int motionTimeMs) = 0;

  /*!
   * \brief A throttle has changed state
   *
   * Up is positive position, down is negative position.
   *
   * \param feature      The wheel changing state
   * \param position     The position in the closed interval [-1, 1]
   * \param motionTimeMs The time elapsed since the position was 0
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnThrottleMotion(const FeatureName& feature,
                                float position,
                                unsigned int motionTimeMs) = 0;

  /*!
   * \brief Called at the end of the frame that provided input
   */
  virtual void OnInputFrame() = 0;

  // Input receiver interface
  void SetInputReceiver(IInputReceiver* receiver) { m_receiver = receiver; }
  void ResetInputReceiver(void) { m_receiver = nullptr; }
  IInputReceiver* InputReceiver(void) { return m_receiver; }

private:
  IInputReceiver* m_receiver = nullptr;
};
} // namespace JOYSTICK
} // namespace KODI
