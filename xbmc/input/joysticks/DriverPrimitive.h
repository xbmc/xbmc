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

#include <stdint.h>

namespace KODI
{
namespace JOYSTICK
{
  /*!
   * \ingroup joystick
   * \brief Basic driver element associated with input events
   *
   * Driver input (bools, floats and enums) is split into primitives that better
   * map to the physical features on a joystick.
   *
   * A bool obviously only maps to a single feature, so it is a driver
   * primitive. Here, these are called "buttons".
   *
   * A hat enum encodes the state of the four hat directions. Each direction
   * can map to a different feature, so a hat enum consists of four driver
   * primitives called "hat directions".
   *
   * A float is a little trickier. Trivially, it can map to an analog stick or
   * trigger. However, DirectInput combines two triggers onto a single axis.
   * Therefore, the axis is split into two primitives called "semiaxes".
   *
   * The type determines the fields in use:
   *
   *    Button:
   *       - driver index
   *
   *    Hat direction:
   *       - driver index
   *       - hat direction (up/right/down/left)
   *
   *    Semiaxis:
   *       - driver index
   *       - center (-1, 0 or 1)
   *       - semiaxis direction (positive/negative)
   *       - range (1 or 2)
   *
   *    Motor:
   *       - driver index
   *
   * For more info, see "Chapter 2. Joystick drivers" in the documentation
   * thread: http://forum.kodi.tv/showthread.php?tid=257764
   */
  class CDriverPrimitive
  {
  public:
    /*!
     * \brief Construct an invalid driver primitive
     */
    CDriverPrimitive(void);

    /*!
     * \brief Construct a driver primitive representing a button or motor
     */
    CDriverPrimitive(PRIMITIVE_TYPE type, unsigned int index);

    /*!
     * \brief Construct a driver primitive representing one of the four
     *        direction arrows on a dpad
     */
    CDriverPrimitive(unsigned int hatIndex, HAT_DIRECTION direction);

    /*!
     * \brief Construct a driver primitive representing the positive or negative
     *        half of an axis
     */
    CDriverPrimitive(unsigned int axisIndex, int center, SEMIAXIS_DIRECTION direction, unsigned int range);

    bool operator==(const CDriverPrimitive& rhs) const;
    bool operator<(const CDriverPrimitive& rhs) const;

    bool operator!=(const CDriverPrimitive& rhs) const { return !operator==(rhs); }
    bool operator>(const CDriverPrimitive& rhs) const  { return !(operator<(rhs) || operator==(rhs)); }
    bool operator<=(const CDriverPrimitive& rhs) const { return   operator<(rhs) || operator==(rhs); }
    bool operator>=(const CDriverPrimitive& rhs) const { return  !operator<(rhs); }

    /*!
     * \brief The type of driver primitive
     */
    PRIMITIVE_TYPE Type(void) const { return m_type; }

    /*!
     * \brief The index used by the driver (valid for all types)
     */
    unsigned int Index(void) const { return m_driverIndex; }

    /*!
     * \brief The direction arrow (valid for hat directions)
     */
    HAT_DIRECTION HatDirection(void) const { return m_hatDirection; }

    /*!
     * \brief The location of the zero point of the semiaxis
     */
    int Center() const { return m_center; }

    /*!
     * \brief The semiaxis direction (valid for semiaxes)
     */
    SEMIAXIS_DIRECTION SemiAxisDirection(void) const { return m_semiAxisDirection; }

    /*!
     * \brief The distance between the center and the farthest valid value (valid for semiaxes)
     */
    unsigned int Range() const { return m_range; }

    /*!
     * \brief Test if an driver primitive is valid
     *
     * A driver primitive is valid if it has a known type and:
     *
     *   1) for hats, it is a cardinal direction
     *   2) for semi-axes, it is a positive or negative direction
     */
    bool IsValid(void) const;

  private:
    PRIMITIVE_TYPE     m_type;
    unsigned int       m_driverIndex;
    HAT_DIRECTION      m_hatDirection;
    int                m_center;
    SEMIAXIS_DIRECTION m_semiAxisDirection;
    unsigned int       m_range;
  };
}
}
