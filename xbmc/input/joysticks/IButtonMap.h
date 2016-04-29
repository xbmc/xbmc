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

#include "DriverPrimitive.h"
#include "JoystickTypes.h"

#include <string>

namespace JOYSTICK
{
  /*!
   * \brief Button map interface to translate between the driver's raw
   *        button/hat/axis elements and physical joystick features.
   *
   * \sa IButtonMapper
   */
  class IButtonMap
  {
  public:
    virtual ~IButtonMap(void) { }

    /*!
     * \brief The add-on ID of the game controller associated with this button map
     *
     * The controller ID provided by the implementation serves as the context
     * for the feature names below.
     *
     * \return The ID of this button map's game controller add-on
     */
    virtual std::string ControllerID(void) const = 0;

    /*!
     * \brief Load the button map into memory
     *
     * \return True if button map is ready to start translating buttons, false otherwise
     */
    virtual bool Load(void) = 0;

    /*!
     * \brief Reset the button map to its defaults, or clear button map if no defaults
     */
    virtual void Reset(void) = 0;

    /*!
     * \brief Get the feature associated with a driver primitive
     *
     * Multiple primitives can be mapped to the same feature. For example,
     * analog sticks use one primitive for each direction.
     *
     * \param primitive    The driver primitive (a button, hat direction or semi-axis)
     * \param feature      The name of the resolved joystick feature, or
     *                     invalid if false is returned
     *
     * \return True if the driver primitive is associated with a feature, false otherwise
     */
    virtual bool GetFeature(
      const CDriverPrimitive& primitive,
      FeatureName& feature
    ) = 0;

    /*!
     * \brief Get the type of the feature for the given name
     *
     * \param feature      The feature to look up
     *
     * \return The feature's type
     */
    virtual FEATURE_TYPE GetFeatureType(const FeatureName& feature) = 0;

    /*!
     * \brief Get the driver primitive for a scalar feature
     *
     * When a feature can be represented by a single driver primitive, it is
     * called a scalar feature.
     *
     *   - This includes buttons and triggers, because they can be mapped to a
     *     single button/hat/semiaxis
     *
     *   - This does not include analog sticks, because they require two axes
     *     and four driver primitives (one for each semiaxis)
     *
     * \param feature        Must be a scalar feature (a feature that only
     *                       requires a single driver primitive)
     * \param primitive      The resolved driver primitive
     *
     * \return True if the feature resolved to a driver primitive, false if the
     *         feature didn't resolve or isn't a scalar feature
     */
    virtual bool GetScalar(
      const FeatureName& feature,
      CDriverPrimitive& primitive
    ) = 0;

    /*!
     * \brief Add or update a scalar feature
     *
     * \param feature        Must be a scalar feature
     * \param primitive      The feature's driver primitive
     *
     * \return True if the feature was updated, false if the feature is
     *         unchanged or failure occurs
     */
    virtual bool AddScalar(
      const FeatureName& feature,
      const CDriverPrimitive& primitive
    ) = 0;

    /*!
     * \brief Get an analog stick from the button map
     *
     * \param feature  Must be an analog stick or this will return false
     * \param up       The primitive mapped to the up direction (possibly unknown)
     * \param down     The primitive mapped to the down direction (possibly unknown)
     * \param right    The primitive mapped to the right direction (possibly unknown)
     * \param left     The primitive mapped to the left direction (possibly unknown)
     *
     * \return True if the feature resolved to an analog stick with at least 1 known direction
     */
    virtual bool GetAnalogStick(
      const FeatureName& feature,
      CDriverPrimitive& up,
      CDriverPrimitive& down,
      CDriverPrimitive& right,
      CDriverPrimitive& left
    ) = 0;

    /*!
     * \brief Add or update an analog stick
     *
     * \param feature  Must be an analog stick or this will return false
     * \param up       The driver primitive for the up direction
     * \param down     The driver primitive for the down direction
     * \param right    The driver primitive for the right direction
     * \param left     The driver primitive for the left direction
     *
     * It is not required that these primitives be axes. If a primitive is a
     * semiaxis, its opposite should point to the same axis index but with
     * opposite direction.
     *
     * \return True if the analog stick was updated, false otherwise
     */
    virtual bool AddAnalogStick(
      const FeatureName& feature,
      const CDriverPrimitive& up,
      const CDriverPrimitive& down,
      const CDriverPrimitive& right,
      const CDriverPrimitive& left
    ) = 0;

    /*!
     * \brief Get an accelerometer from the button map
     *
     * \param feature       Must be an accelerometer or this will return false
     * \param positiveX     The semiaxis mapped to the positive X direction (possibly unknown)
     * \param positiveY     The semiaxis mapped to the positive Y direction (possibly unknown)
     * \param positiveZ     The semiaxis mapped to the positive Z direction (possibly unknown)
     *
     * \return True if the feature resolved to an accelerometer with at least 1 known axis
     */
    virtual bool GetAccelerometer(
      const FeatureName& feature,
      CDriverPrimitive& positiveX,
      CDriverPrimitive& positiveY,
      CDriverPrimitive& positiveZ
    ) = 0;

    /*!
     * \brief Get or update an accelerometer
     *
     * \param feature       Must be an accelerometer or this will return false
     * \param positiveX     The semiaxis corresponding to the positive X direction
     * \param positiveY     The semiaxis corresponding to the positive Y direction
     * \param positiveZ     The semiaxis corresponding to the positive Z direction
     *
     * The driver primitives must be mapped to a semiaxis or this function will fail.
     *
     * \return True if the accelerometer was updated, false if unchanged or failure occurred
     */
    virtual bool AddAccelerometer(
      const FeatureName& feature,
      const CDriverPrimitive& positiveX,
      const CDriverPrimitive& positiveY,
      const CDriverPrimitive& positiveZ
    ) = 0;
  };
}
