/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AxisConfiguration.h"
#include "PrimitiveDetector.h"
#include "input/joysticks/DriverPrimitive.h"

#include <chrono>

namespace KODI
{
namespace JOYSTICK
{
class CButtonMapping;

/*!
 * \ingroup joystick
 *
 * \brief Detects when an axis should be mapped
 */
class CAxisDetector : public CPrimitiveDetector
{
public:
  CAxisDetector(CButtonMapping* buttonMapping,
                unsigned int axisIndex,
                const AxisConfiguration& config);

  /*!
   * \brief Axis state has been updated
   *
   * \param position The new state
   *
   * \return Always true - axis motion events are always absorbed while button mapping
   */
  bool OnMotion(float position);

  /*!
   * \brief Called once per frame
   *
   * If an axis was activated, the button mapping command will be emitted
   * here.
   */
  void ProcessMotion();

  /*!
   * \brief Check if the axis was mapped and is still in motion
   *
   * \return True between when the axis is mapped and when it crosses zero
   */
  bool IsMapping() const { return m_state == AXIS_STATE::MAPPED; }

  /*!
   * \brief Set the state such that this axis has generated a mapping event
   *
   * If an axis is mapped to the Select action, it may be pressed when button
   * mapping begins. This function is used to indicate that the axis shouldn't
   * be mapped until after it crosses zero again.
   */
  void SetEmitted(const CDriverPrimitive& activePrimitive);

private:
  enum class AXIS_STATE
  {
    /*!
     * \brief Axis is inactive (position is less than threshold)
     */
    INACTIVE,

    /*!
     * \brief Axis is activated (position has exceeded threshold)
     */
    ACTIVATED,

    /*!
     * \brief Axis has generated a mapping event, but has not been centered yet
     */
    MAPPED,
  };

  enum class AXIS_TYPE
  {
    /*!
     * \brief Axis type is initially unknown
     */
    UNKNOWN,

    /*!
     * \brief Axis is centered about 0
     *
     *   - If the axis is an analog stick, it can travel to -1 or +1.
     *   - If the axis is a pressure-sensitive button or a normal trigger,
     *     it can travel to +1.
     *   - If the axis is a DirectInput trigger, then it is possible that two
     *     triggers can be on the same axis in opposite directions.
     *   - Normally, D-pads  appear as a hat or four buttons. However, some
     *     D-pads are reported as two axes that can have the discrete values
     *     -1, 0 or 1. This is called a "discrete D-pad".
     */
    NORMAL,

    /*!
     * \brief Axis is centered about -1 or 1
     *
     *   - On OSX, with the cocoa driver, triggers are centered about -1 and
     *     travel to +1. In this case, the range is 2 and the direction is
     *     positive.
     *   - The author of SDL has observed triggers centered at +1 and travel
     *     to 0. In this case, the range is 1 and the direction is negative.
     */
    OFFSET,
  };

  void DetectType(float position);

  // Construction parameters
  const unsigned int m_axisIndex;
  AxisConfiguration m_config; // mutable

  // State variables
  AXIS_STATE m_state = AXIS_STATE::INACTIVE;
  CDriverPrimitive m_activatedPrimitive;
  AXIS_TYPE m_type = AXIS_TYPE::UNKNOWN;
  bool m_initialPositionKnown = false; // set to true on first motion
  float m_initialPosition = 0.0f; // set to position of first motion
  bool m_initialPositionChanged =
      false; // set to true when position differs from the initial position
  std::chrono::time_point<std::chrono::steady_clock>
      m_activationTimeMs; // only used to delay anomalous trigger mapping to detect full range
};
} // namespace JOYSTICK
} // namespace KODI
