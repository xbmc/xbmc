#pragma once
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

#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/PeripheralUtils.h"

#include <string>
#include <utility>
#include <vector>

struct AInputEvent;
class CJNIViewInputDevice;

namespace PERIPHERALS
{
  class CAndroidJoystickState
  {
  public:
    CAndroidJoystickState();
    virtual ~CAndroidJoystickState();

    int GetDeviceId() const { return m_deviceId; }

    unsigned int GetButtonCount() const { return m_buttons.size(); }
    unsigned int GetHatCount() const { return m_hats.size(); }
    unsigned int GetAxisCount() const { return m_axes.size(); }

    /*!
     * Initialize the joystick object. Joystick will be initialized before the
     * first call to GetEvents().
     */
    bool Initialize(const CJNIViewInputDevice& inputDevice);

    /*!
     * Deinitialize the joystick object. GetEvents() will not be called after
     * deinitialization.
     */
    void Deinitialize();

    /*!
     * Processes the given input event.
     */
    bool ProcessEvent(const AInputEvent* event);

    /*!
     * Get events that have occurred since the last call to GetEvents()
     */
    void GetEvents(std::vector<kodi::addon::PeripheralEvent>& events) const;

  private:
    bool SetButtonValue(int axisId, JOYSTICK_STATE_BUTTON buttonValue);
    bool SetHatValue(const std::vector<int>& axisIds, JOYSTICK_STATE_HAT hatValue);
    bool SetAxisValue(const std::vector<int>& axisIds, JOYSTICK_STATE_AXIS axisValue);

    void GetButtonEvents(std::vector<kodi::addon::PeripheralEvent>& events) const;
    void GetHatEvents(std::vector<kodi::addon::PeripheralEvent>& events) const;
    void GetAxisEvents(std::vector<kodi::addon::PeripheralEvent>& events) const;

    static float Contain(float value, float min, float max);
    static float Scale(float value, float max, float scaledMax);
    static float Deadzone(float value, float deadzone);

    struct JoystickAxis
    {
      std::vector<int> ids;
      float flat;
      float fuzz;
      float min;
      float max;
      float range;
      float resolution;
    };

    using JoystickAxes = std::vector<JoystickAxis>;

    static JoystickAxes::const_iterator GetAxis(const std::vector<int>& axisIds, const JoystickAxes& axes);
    static bool ContainsAxis(int axisId, const JoystickAxes& axes);
    static bool GetAxesIndex(const std::vector<int>& axisIds, const JoystickAxes& axes, size_t& axesIndex);

    struct JoystickState
    {
      std::vector<JOYSTICK_STATE_BUTTON> buttons;
      std::vector<JOYSTICK_STATE_HAT> hats;
      std::vector<JOYSTICK_STATE_AXIS> axes;
    };

    int m_deviceId;

    JoystickAxes m_buttons;
    JoystickAxes m_hats;
    JoystickAxes m_axes;

    mutable JoystickState m_state;
    JoystickState m_stateBuffer;
  };
}
