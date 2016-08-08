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

#include "input/joysticks/JoystickTypes.h"

#include <memory>

namespace JOYSTICK
{
  class CDriverPrimitive;
  class IInputHandler;
  class IButtonMap;

  class CJoystickFeature;
  typedef std::shared_ptr<CJoystickFeature> FeaturePtr;

  /*!
   * \brief Base class for joystick features
   *
   * See list of feature types in JoystickTypes.h.
   */
  class CJoystickFeature
  {
  public:
    CJoystickFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
    virtual ~CJoystickFeature(void) { }

    /*!
     * \brief A digital motion has occured
     *
     * \param source The source of the motion. Must be digital (button or hat)
     * \param bPressed True for press motion, false for release motion
     *
     * \return true if the motion was handled, false otherwise
     */
    virtual bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) = 0;

    /*!
     * \brief An analog motion has occured
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
     * \brief Process the motions that have occured since the last invocation
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
    const FeatureName    m_name;
    IInputHandler* const m_handler;
    IButtonMap* const    m_buttonMap;
    const bool           m_bEnabled;
  };

  class CScalarFeature : public CJoystickFeature
  {
  public:
    CScalarFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
    virtual ~CScalarFeature(void) { }

    // implementation of CJoystickFeature
    virtual bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) override;
    virtual bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
    virtual void ProcessMotions(void) override;

  private:
    void OnDigitalMotion(bool bPressed);
    void OnAnalogMotion(float magnitude);

    const INPUT_TYPE m_inputType;
    bool             m_bDigitalState;
    bool             m_bDigitalHandled;
    unsigned int     m_holdStartTimeMs;
    float            m_analogState;
  };

  /*!
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

  class CAnalogStick : public CJoystickFeature
  {
  public:
    CAnalogStick(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
    virtual ~CAnalogStick(void) { }

    // implementation of CJoystickFeature
    virtual bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) override;
    virtual bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
    virtual void ProcessMotions(void) override;

  protected:
    CFeatureAxis m_vertAxis;
    CFeatureAxis m_horizAxis;

    float m_vertState;
    float m_horizState;

    unsigned int m_motionStartTimeMs;
  };

  class CAccelerometer : public CJoystickFeature
  {
  public:
    CAccelerometer(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap);
    virtual ~CAccelerometer(void) { }

    // implementation of CJoystickFeature
    virtual bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed) override;
    virtual bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude) override;
    virtual void ProcessMotions(void) override;

  protected:
    CFeatureAxis m_xAxis;
    CFeatureAxis m_yAxis;
    CFeatureAxis m_zAxis;

    float m_xAxisState;
    float m_yAxisState;
    float m_zAxisState;
  };
}
