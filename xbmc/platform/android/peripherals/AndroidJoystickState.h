/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/addon-instance/peripheral/PeripheralUtils.h"
#include "threads/CriticalSection.h"

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
    CAndroidJoystickState() = default;
    CAndroidJoystickState(CAndroidJoystickState &&other);
    virtual ~CAndroidJoystickState();

    int GetDeviceId() const { return m_deviceId; }

    unsigned int GetButtonCount() const { return static_cast<unsigned int>(m_buttons.size()); }
    unsigned int GetAxisCount() const { return static_cast<unsigned int>(m_axes.size()); }

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
    void GetEvents(std::vector<kodi::addon::PeripheralEvent>& events);

  private:
    bool SetButtonValue(int axisId, JOYSTICK_STATE_BUTTON buttonValue);
    bool SetAxisValue(const std::vector<int>& axisIds, JOYSTICK_STATE_AXIS axisValue);

    void GetButtonEvents(std::vector<kodi::addon::PeripheralEvent>& events);
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

    int m_deviceId = -1;

    JoystickAxes m_buttons;
    JoystickAxes m_axes;

    std::vector<JOYSTICK_STATE_AXIS> m_analogState;

    CCriticalSection m_eventMutex;
    std::vector<kodi::addon::PeripheralEvent> m_digitalEvents;
  };
}
