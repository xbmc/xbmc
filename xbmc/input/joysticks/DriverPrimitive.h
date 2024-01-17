/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JoystickTypes.h"
#include "input/keyboard/KeyboardTypes.h"
#include "input/mouse/MouseTypes.h"

#include <stdint.h>

namespace KODI
{
namespace JOYSTICK
{
/*!
 * \ingroup joystick
 *
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
 *    Key:
 *       - keycode
 *
 *    Mouse button:
 *       - driver index
 *
 *    Relative pointer:
 *       - pointer direction
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
  CDriverPrimitive(unsigned int axisIndex,
                   int center,
                   SEMIAXIS_DIRECTION direction,
                   unsigned int range);

  /*!
   * \brief Construct a driver primitive representing a key on a keyboard
   */
  CDriverPrimitive(KEYBOARD::XBMCKey keycode);

  /*!
   * \brief Construct a driver primitive representing a mouse button
   */
  CDriverPrimitive(MOUSE::BUTTON_ID index);

  /*!
   * \brief Construct a driver primitive representing a relative pointer
   */
  CDriverPrimitive(RELATIVE_POINTER_DIRECTION direction);

  bool operator==(const CDriverPrimitive& rhs) const;
  bool operator<(const CDriverPrimitive& rhs) const;

  bool operator!=(const CDriverPrimitive& rhs) const { return !operator==(rhs); }
  bool operator>(const CDriverPrimitive& rhs) const { return !(operator<(rhs) || operator==(rhs)); }
  bool operator<=(const CDriverPrimitive& rhs) const { return operator<(rhs) || operator==(rhs); }
  bool operator>=(const CDriverPrimitive& rhs) const { return !operator<(rhs); }

  /*!
   * \brief The type of driver primitive
   */
  PRIMITIVE_TYPE Type(void) const { return m_type; }

  /*!
   * \brief The index used by the joystick driver
   *
   * Valid for:
   *   - buttons
   *   - hats
   *   - semiaxes
   *   - motors
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
   * \brief The keyboard symbol (valid for keys)
   */
  KEYBOARD::XBMCKey Keycode() const { return m_keycode; }

  /*!
   * \brief The mouse button ID (valid for mouse buttons)
   */
  MOUSE::BUTTON_ID MouseButton() const { return static_cast<MOUSE::BUTTON_ID>(m_driverIndex); }

  /*!
   * \brief The relative pointer direction (valid for relative pointers)
   */
  RELATIVE_POINTER_DIRECTION PointerDirection() const { return m_pointerDirection; }

  /*!
   * \brief Test if an driver primitive is valid
   *
   * A driver primitive is valid if it has a known type and:
   *
   *   1) for hats, it is a cardinal direction
   *   2) for semi-axes, it is a positive or negative direction
   *   3) for keys, the keycode is non-empty
   */
  bool IsValid(void) const;

private:
  PRIMITIVE_TYPE m_type = PRIMITIVE_TYPE::UNKNOWN;
  unsigned int m_driverIndex = 0;
  HAT_DIRECTION m_hatDirection = HAT_DIRECTION::NONE;
  int m_center = 0;
  SEMIAXIS_DIRECTION m_semiAxisDirection = SEMIAXIS_DIRECTION::ZERO;
  unsigned int m_range = 1;
  KEYBOARD::XBMCKey m_keycode = KEYBOARD::XBMCKey::XBMCK_UNKNOWN;
  RELATIVE_POINTER_DIRECTION m_pointerDirection = RELATIVE_POINTER_DIRECTION::NONE;
};
} // namespace JOYSTICK
} // namespace KODI
