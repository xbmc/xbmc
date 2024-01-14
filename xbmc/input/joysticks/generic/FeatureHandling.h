/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"

#include <chrono>
#include <memory>

namespace KODI
{
namespace JOYSTICK
{
class CDriverPrimitive;
class IInputHandler;
class IButtonMap;

class CJoystickFeature;
using FeaturePtr = std::shared_ptr<CJoystickFeature>;

/*!
 * \ingroup joystick
 *
 * \brief Base class for joystick features
 *
 * See list of feature types in JoystickTypes.h.
 */
class CJoystickFeature
{
public:
  CJoystickFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
  virtual ~CJoystickFeature() = default;

  /*!
   * \brief A digital motion has occurred
   *
   * \param source The source of the motion. Must be digital (button or hat)
   * \param bPressed True for press motion, false for release motion
   *
   * \return true if the motion was handled, false otherwise
   */
  virtual bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) = 0;

  /*!
   * \brief An analog motion has occurred
   *
   * \param source The source of the motion. Must be a semiaxis
   * \param magnitude The magnitude of the press or motion in the interval [0.0, 1.0]
   *
   * For semiaxes, the magnitude is the force or travel distance in the
   * direction of the semiaxis. If the value is in the opposite direction,
   * the magnitude is 0.0.
   *
   * For example, if the analog stick goes left, the negative semiaxis will
   * have a value of 1.0 and the positive semiaxis will have a value of 0.0.
   */
  virtual bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) = 0;

  /*!
   * \brief Process the motions that have occurred since the last invocation
   *
   * This allows features with motion on multiple driver primitives to call
   * their handler once all driver primitives are accounted for.
   */
  virtual void ProcessMotions(void) = 0;

  /*!
   * \brief Check if the input handler is accepting input
   *
   * \param bActivation True if the motion is activating (true or positive),
   *                    false if the motion is deactivating (false or zero)
   *
   * \return True if input should be sent to the input handler, false otherwise
   */
  bool AcceptsInput(bool bActivation);

protected:
  /*!
   * \brief Reset motion timer
   */
  void ResetMotion();

  /*!
   * \brief Start the motion timer
   */
  void StartMotion();

  /*!
   * \brief Check if the feature is in motion
   */
  bool InMotion() const;

  /*!
   * \brief Get the time for which the feature has been in motion
   */
  unsigned int MotionTimeMs() const;

  const FeatureName m_name;
  IInputHandler* const m_handler;
  IButtonMap* const m_buttonMap;
  const bool m_bEnabled;

private:
  std::chrono::time_point<std::chrono::steady_clock> m_motionStartTimeMs;
};

/*!
 * \ingroup joystick
 */
class CScalarFeature : public CJoystickFeature
{
public:
  CScalarFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
  ~CScalarFeature() override = default;

  // implementation of CJoystickFeature
  bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) override;
  bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
  void ProcessMotions() override;

private:
  bool OnDigitalMotion(bool bPressed);
  bool OnAnalogMotion(float magnitude);

  void ProcessDigitalMotion();
  void ProcessAnalogMotion();

  // State variables
  INPUT_TYPE m_inputType = INPUT_TYPE::UNKNOWN;
  bool m_bDigitalState = false;
  bool m_bInitialPressHandled = false;

  // Analog state variables
  float m_analogState = 0.0f; // The current magnitude
  float m_bActivated = false; // Set to true when first activated (magnitude > 0.0)
  bool m_bDiscrete = true; // Set to false when a non-discrete axis is detected
};

/*!
 * \ingroup joystick
 *
 * \brief Axis of a feature (analog stick, accelerometer, etc)
 *
 * Axes are composed of two driver primitives, one for the positive semiaxis
 * and one for the negative semiaxis.
 *
 * This effectively means that an axis is two-dimensional, with each dimension
 * either:
 *
 *    - a digital value (0.0 or 1.0)
 *    - an analog value (continuous in the interval [0.0, 1.0])
 */
class CFeatureAxis
{
public:
  CFeatureAxis(void) { Reset(); }

  /*!
   * \brief Set value of positive axis
   */
  void SetPositiveDistance(float distance) { m_positiveDistance = distance; }

  /*!
   * \brief Set value of negative axis
   */
  void SetNegativeDistance(float distance) { m_negativeDistance = distance; }

  /*!
   * \brief Get the final value of this axis.
   *
   * This axis is two-dimensional, so we need to compress these into a single
   * dimension. This is done by subtracting the negative from the positive.
   * Some examples:
   *
   *      Positive axis:  1.0 (User presses right or analog stick moves right)
   *      Negative axis:  0.0
   *      -------------------
   *      Pos - Neg:      1.0 (Emulated analog stick moves right)
   *
   *
   *      Positive axis:  0.0
   *      Negative axis:  1.0 (User presses left or analog stick moves left)
   *      -------------------
   *      Pos - Neg:     -1.0 (Emulated analog stick moves left)
   *
   *
   *      Positive axis:  1.0 (User presses both buttons)
   *      Negative axis:  1.0
   *      -------------------
   *      Pos - Neg:      0.0 (Emulated analog stick is centered)
   *
   */
  float GetPosition(void) const { return m_positiveDistance - m_negativeDistance; }

  /*!
   * \brief Reset both positive and negative values to zero
   */
  void Reset(void) { m_positiveDistance = m_negativeDistance = 0.0f; }

protected:
  float m_positiveDistance;
  float m_negativeDistance;
};

/*!
 * \ingroup joystick
 */
class CAxisFeature : public CJoystickFeature
{
public:
  CAxisFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
  ~CAxisFeature() override = default;

  // partial implementation of CJoystickFeature
  bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) override;
  void ProcessMotions() override;

protected:
  CFeatureAxis m_axis;

  float m_state = 0.0f;
};

/*!
 * \ingroup joystick
 */
class CWheel : public CAxisFeature
{
public:
  CWheel(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
  ~CWheel() override = default;

  // partial implementation of CJoystickFeature
  bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
};

/*!
 * \ingroup joystick
 */
class CThrottle : public CAxisFeature
{
public:
  CThrottle(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
  ~CThrottle() override = default;

  // partial implementation of CJoystickFeature
  bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
};

/*!
 * \ingroup joystick
 */
class CAnalogStick : public CJoystickFeature
{
public:
  CAnalogStick(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
  ~CAnalogStick() override = default;

  // implementation of CJoystickFeature
  bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) override;
  bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
  void ProcessMotions() override;

protected:
  CFeatureAxis m_vertAxis;
  CFeatureAxis m_horizAxis;

  float m_vertState = 0.0f;
  float m_horizState = 0.0f;
};

/*!
 * \ingroup joystick
 */
class CAccelerometer : public CJoystickFeature
{
public:
  CAccelerometer(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
  ~CAccelerometer() override = default;

  // implementation of CJoystickFeature
  bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) override;
  bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
  void ProcessMotions() override;

protected:
  CFeatureAxis m_xAxis;
  CFeatureAxis m_yAxis;
  CFeatureAxis m_zAxis;

  float m_xAxisState = 0.0f;
  float m_yAxisState = 0.0f;
  float m_zAxisState = 0.0f;
};
} // namespace JOYSTICK
} // namespace KODI
