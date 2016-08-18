/*
 *      Copyright (C) 2016 Team Kodi
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

namespace PERIPHERALS
{
  class CPeripheral;
}

namespace JOYSTICK
{
  class IButtonMap;

  /*!
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
   *   - Positive in the interval (deadzone, 1.0)
   */
  class CDeadzoneFilter
  {
  public:
    CDeadzoneFilter(IButtonMap* buttonMap, PERIPHERALS::CPeripheral* peripheral);

    /*!
     * \brief Apply deadzone filtering to an axis
     * \param axisIndex The axis index
     * \param axisValud The axis value\
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
    bool GetDeadzone(unsigned int axisIndex, float& result, const char* featureName, const char* settingName);

    /*!
     * \brief Utility function to calculate the deadzone
     * \param value The value
     * \param deadzone The deadzone
     * \return The scaled deadzone
     */
    static float ApplyDeadzone(float value, float deadzone);

    // Construction parameters
    IButtonMap* const               m_buttonMap;
    PERIPHERALS::CPeripheral* const m_peripheral;
  };
}
