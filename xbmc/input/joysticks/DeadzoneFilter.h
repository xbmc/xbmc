/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace PERIPHERALS
{
class CPeripheral;
}

namespace KODI
{
namespace JOYSTICK
{
class IButtonMap;

/*!
 * \ingroup joystick
 *
 * \brief Analog axis deadzone filtering
 *
 * Axis is scaled appropriately, so position is continuous
 * from -1.0 to 1.0:
 *
 *            |    / 1.0
 *            |   /
 *          __|__/
 *         /  |
 *        /   |--| Deadzone
 *  -1.0 /    |
 *
 * After deadzone filtering, the value will be:
 *
 *   - Negative in the interval [-1.0, -deadzone)
 *   - Zero in the interval [-deadzone, deadzone]
 *   - Positive in the interval (deadzone, 1.0]
 */
class CDeadzoneFilter
{
public:
  CDeadzoneFilter(IButtonMap* buttonMap, PERIPHERALS::CPeripheral* peripheral);

  /*!
   * \brief Apply deadzone filtering to an axis
   * \param axisIndex The axis index
   * \param axisValue The axis value
   * \return The value after applying deadzone filtering
   */
  float FilterAxis(unsigned int axisIndex, float axisValue);

private:
  /*!
   * \brief Get the deadzone value from the peripheral's settings
   * \param axisIndex The axis index
   * \param[out] result The deadzone value
   * \param featureName The feature that axisIndex is mapped to
   * \param settingName The setting corresponding to the given feature
   * \return True if the feature is an analog stick and the peripheral has the setting
   */
  bool GetDeadzone(unsigned int axisIndex,
                   float& result,
                   const char* featureName,
                   const char* settingName);

  /*!
   * \brief Utility function to calculate the deadzone
   * \param value The value
   * \param deadzone The deadzone
   * \return The scaled deadzone
   */
  static float ApplyDeadzone(float value, float deadzone);

  // Construction parameters
  IButtonMap* const m_buttonMap;
  PERIPHERALS::CPeripheral* const m_peripheral;
};
} // namespace JOYSTICK
} // namespace KODI
