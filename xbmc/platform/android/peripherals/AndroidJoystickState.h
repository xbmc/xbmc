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

namespace KODI
{
namespace JOYSTICK
{
class IButtonMap;
} // namespace JOYSTICK
} // namespace KODI

namespace PERIPHERALS
{
class CAndroidJoystickState
{
public:
  CAndroidJoystickState() = default;
  CAndroidJoystickState(CAndroidJoystickState&& other) noexcept;
  virtual ~CAndroidJoystickState();

  int GetDeviceId() const { return m_deviceId; }

  unsigned int GetButtonCount() const { return static_cast<unsigned int>(m_buttons.size()); }
  unsigned int GetAxisCount() const { return static_cast<unsigned int>(m_axes.size()); }

  /*!
   * \brief Initialize the joystick object
   *
   * Joystick will be initialized before the first call to GetEvents().
   */
  bool Initialize(const CJNIViewInputDevice& inputDevice);

  /*!
   * \brief Initialize a joystick buttonmap, if possible
   *
   * Android has a large database of buttonmaps, which it uses to provide
   * mapped button keycodes such as AKEYCODE_BUTTON_A. We can take advantage of
   * this to initialize a default buttonmap based on these mappings.
   *
   * If Android can't map the buttons, it will use generic button keycodes such
   * as AKEYCODE_BUTTON_1, in which case we can't initialize the buttonmap.
   */
  bool InitializeButtonMap(KODI::JOYSTICK::IButtonMap& buttonMap) const;

  /*!
   * \brief Get the joystick appearance, if known
   */
  std::string GetAppearance() const;

  /*!
   * \brief Deinitialize the joystick object
   *
   * GetEvents() will not be called after deinitialization.
   */
  void Deinitialize();

  /*!
   * \brief Processes the given input event.
   */
  bool ProcessEvent(const AInputEvent* event);

  /*!
   * \brief Get events that have occurred since the last call to GetEvents()
   */
  void GetEvents(std::vector<kodi::addon::PeripheralEvent>& events);

private:
  bool SetButtonValue(int axisId, JOYSTICK_STATE_BUTTON buttonValue);
  bool SetAxisValue(const std::vector<int>& axisIds, JOYSTICK_STATE_AXIS axisValue);

  void GetButtonEvents(std::vector<kodi::addon::PeripheralEvent>& events);
  void GetAxisEvents(std::vector<kodi::addon::PeripheralEvent>& events) const;

  bool MapButton(KODI::JOYSTICK::IButtonMap& buttonMap, int buttonKeycode) const;
  bool MapTrigger(KODI::JOYSTICK::IButtonMap& buttonMap,
                  int axisId,
                  const std::string& triggerName) const;
  bool MapDpad(KODI::JOYSTICK::IButtonMap& buttonMap, int horizAxisId, int vertAxisId) const;
  bool MapAnalogStick(KODI::JOYSTICK::IButtonMap& buttonMap,
                      int horizAxisId,
                      int vertAxisId,
                      const std::string& analogStickName) const;

  static float Contain(float value, float min, float max);
  static float Scale(float value, float max, float scaledMax);
  static float Deadzone(float value, float deadzone);

  struct JoystickAxis
  {
    std::vector<int> ids;
    float flat = 0.0f;
    float fuzz = 0.0f;
    float min = 0.0f;
    float max = 0.0f;
    float range = 0.0f;
    float resolution = 0.0f;
  };

  using JoystickAxes = std::vector<JoystickAxis>;

  static JoystickAxes::const_iterator GetAxis(const std::vector<int>& axisIds,
                                              const JoystickAxes& axes);
  static bool ContainsAxis(int axisId, const JoystickAxes& axes);
  static bool GetAxesIndex(const std::vector<int>& axisIds,
                           const JoystickAxes& axes,
                           size_t& axesIndex);

  int m_deviceId = -1;

  JoystickAxes m_buttons;
  JoystickAxes m_axes;

  std::vector<JOYSTICK_STATE_AXIS> m_analogState;

  CCriticalSection m_eventMutex;
  std::vector<kodi::addon::PeripheralEvent> m_digitalEvents;
};
} // namespace PERIPHERALS
