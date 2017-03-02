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

#include <algorithm>

#include <android/input.h>

#include "AndroidJoystickState.h"
#include "platform/android/jni/View.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;

static std::string PrintAxisIds(const std::vector<int>& axisIds)
{
  if (axisIds.empty())
    return "";

  if (axisIds.size() == 1)
    return StringUtils::Format("%d", axisIds.front());

  std::string strAxisIds;
  for (const auto& axisId : axisIds)
  {
    if (strAxisIds.empty())
      strAxisIds = "[";
    else
      strAxisIds += " | ";

    strAxisIds += StringUtils::Format("%d", axisId);
  }
  strAxisIds += "]";

  return strAxisIds;
}

static void MapAxisIds(int axisId, int primaryAxisId, int secondaryAxisId, std::vector<int>& axisIds)
{
  if (axisId != primaryAxisId && axisId != secondaryAxisId)
    return;

  if (axisIds.empty())
  {
    axisIds.push_back(primaryAxisId);
    axisIds.push_back(secondaryAxisId);
  }

  if (axisIds.size() > 1)
    return;

  if (axisId == primaryAxisId)
    axisIds.push_back(secondaryAxisId);
  else if (axisId == secondaryAxisId)
    axisIds.insert(axisIds.begin(), primaryAxisId);
}

CAndroidJoystickState::CAndroidJoystickState()
  : m_deviceId(-1)
{ }

CAndroidJoystickState::~CAndroidJoystickState()
{
  Deinitialize();
}

bool CAndroidJoystickState::Initialize(const CJNIViewInputDevice& inputDevice)
{
  if (!inputDevice)
    return false;

  const std::string deviceName = inputDevice.getName();

  // get the device ID
  m_deviceId = inputDevice.getId();

  // get all motion ranges to be able to count the number of available buttons, hats and axis'
  const CJNIList<CJNIViewInputDeviceMotionRange> motionRanges = inputDevice.getMotionRanges();
  for (int index = 0; index < motionRanges.size(); ++index)
  {
    const CJNIViewInputDeviceMotionRange motionRange = motionRanges.get(index);
    if (!motionRange.isFromSource(CJNIViewInputDevice::SOURCE_JOYSTICK) &&
        !motionRange.isFromSource(CJNIViewInputDevice::SOURCE_GAMEPAD))
    {
      CLog::Log(LOGDEBUG, "CAndroidJoystickState: ignoring axis %d from source %d for input device \"%s\" with ID %d", motionRange.getAxis(), motionRange.getSource(), deviceName.c_str(), m_deviceId);
      continue;
    }

    int axisId = motionRange.getAxis();
    JoystickAxis axis {
      { axisId },
      motionRange.getFlat(),
      motionRange.getFuzz(),
      motionRange.getMin(),
      motionRange.getMax(),
      motionRange.getRange(),
      motionRange.getResolution()
    };

    // check if the axis ID belongs to a hat/D-PAD
    if (axisId == AMOTION_EVENT_AXIS_HAT_X || axisId == AMOTION_EVENT_AXIS_HAT_Y)
    {
      // check if this hat is already known
      if (ContainsAxis(axisId, m_hats))
      {
        CLog::Log(LOGWARNING, "CAndroidJoystickState: duplicate hat %d on input device \"%s\" with ID %d", axisId, deviceName.c_str(), m_deviceId);
        continue;
      }

      // handle the two hat axes together as they belong to the same hat
      MapAxisIds(axisId, AMOTION_EVENT_AXIS_HAT_X, AMOTION_EVENT_AXIS_HAT_Y, axis.ids);

      m_hats.push_back(axis);
      CLog::Log(LOGDEBUG, "CAndroidJoystickState: hat %s on input device \"%s\" with ID %d detected", PrintAxisIds(axis.ids).c_str(), deviceName.c_str(), m_deviceId);
    }
    // check if the axis ID belongs to an analogue stick or trigger
    else if (axisId == AMOTION_EVENT_AXIS_X || axisId == AMOTION_EVENT_AXIS_Y ||
             axisId == AMOTION_EVENT_AXIS_Z || axisId == AMOTION_EVENT_AXIS_RZ ||
             axisId == AMOTION_EVENT_AXIS_LTRIGGER || axisId == AMOTION_EVENT_AXIS_RTRIGGER ||
             axisId == AMOTION_EVENT_AXIS_GAS || axisId == AMOTION_EVENT_AXIS_BRAKE ||
             axisId == AMOTION_EVENT_AXIS_THROTTLE || axisId == AMOTION_EVENT_AXIS_RUDDER || axisId == AMOTION_EVENT_AXIS_WHEEL)
    {
       // check if this hat is already known
      if (ContainsAxis(axisId, m_axes))
      {
        CLog::Log(LOGWARNING, "CAndroidJoystickState: duplicate axis %s on input device \"%s\" with ID %d", PrintAxisIds(axis.ids).c_str(), deviceName.c_str(), m_deviceId);
        continue;
      }

      // map AMOTION_EVENT_AXIS_GAS to AMOTION_EVENT_AXIS_RTRIGGER and
      // AMOTION_EVENT_AXIS_BRAKE to AMOTION_EVENT_AXIS_LTRIGGER
      // to avoid duplicate events on controllers sending both events
      MapAxisIds(axisId, AMOTION_EVENT_AXIS_LTRIGGER, AMOTION_EVENT_AXIS_BRAKE, axis.ids);
      MapAxisIds(axisId, AMOTION_EVENT_AXIS_RTRIGGER, AMOTION_EVENT_AXIS_GAS, axis.ids);

      m_axes.push_back(axis);
      CLog::Log(LOGDEBUG, "CAndroidJoystickState: axis %s on input device \"%s\" with ID %d detected", PrintAxisIds(axis.ids).c_str(), deviceName.c_str(), m_deviceId);
    }
    // check if the axis ID belongs to a known button
    else if (axisId == AKEYCODE_HOME || axisId == AKEYCODE_BACK || axisId == AKEYCODE_MENU ||
             axisId == AKEYCODE_BUTTON_A || axisId == AKEYCODE_BUTTON_B || axisId == AKEYCODE_BUTTON_C ||
             axisId == AKEYCODE_BUTTON_X || axisId == AKEYCODE_BUTTON_Y || axisId == AKEYCODE_BUTTON_Z ||
             axisId == AKEYCODE_BUTTON_L1 || axisId == AKEYCODE_BUTTON_R1 || axisId == AKEYCODE_BUTTON_L2 || axisId == AKEYCODE_BUTTON_R2 ||
             axisId == AKEYCODE_BUTTON_THUMBL || axisId == AKEYCODE_BUTTON_THUMBR ||
             axisId == AKEYCODE_BUTTON_START || axisId == AKEYCODE_BUTTON_SELECT || axisId == AKEYCODE_BUTTON_MODE)
    {
       // check if this hat is already known
      if (ContainsAxis(axisId, m_buttons))
      {
        CLog::Log(LOGWARNING, "CAndroidJoystickState: duplicate button %d on input device \"%s\" with ID %d", axisId, deviceName.c_str(), m_deviceId);
        continue;
      }

      // map AKEYCODE_BUTTON_SELECT to AKEYCODE_BACK and
      // AKEYCODE_BUTTON_MODE to AKEYCODE_MENU and
      // AKEYCODE_BUTTON_START to AKEYCODE_HOME to avoid
      // duplicate events on controllers sending both events
      MapAxisIds(axisId, AKEYCODE_BACK, AKEYCODE_BUTTON_SELECT, axis.ids);
      MapAxisIds(axisId, AKEYCODE_MENU, AKEYCODE_BUTTON_MODE, axis.ids);
      MapAxisIds(axisId, AKEYCODE_HOME, AKEYCODE_BUTTON_START, axis.ids);

      m_buttons.push_back(axis);
      CLog::Log(LOGDEBUG, "CAndroidJoystickState: button %s on input device \"%s\" with ID %d detected", PrintAxisIds(axis.ids).c_str(), deviceName.c_str(), m_deviceId);
    }
    else
      CLog::Log(LOGWARNING, "CAndroidJoystickState: ignoring unknown axis %d on input device \"%s\" with ID %d", axisId, deviceName.c_str(), m_deviceId);
  }

  // if there are no buttons add the usual suspects
  if (GetButtonCount() <= 0)
  {
    m_buttons.push_back({ { AKEYCODE_BUTTON_A } });
    m_buttons.push_back({ { AKEYCODE_BUTTON_B } });
    m_buttons.push_back({ { AKEYCODE_BUTTON_X } });
    m_buttons.push_back({ { AKEYCODE_BUTTON_Y } });
    m_buttons.push_back({ { AKEYCODE_BACK, AKEYCODE_BUTTON_SELECT } });
    m_buttons.push_back({ { AKEYCODE_MENU, AKEYCODE_BUTTON_MODE } });
    m_buttons.push_back({ { AKEYCODE_HOME, AKEYCODE_BUTTON_START } });
    m_buttons.push_back({ { AKEYCODE_BUTTON_L1 } });
    m_buttons.push_back({ { AKEYCODE_BUTTON_R1 } });
    m_buttons.push_back({ { AKEYCODE_BUTTON_THUMBL } });
    m_buttons.push_back({ { AKEYCODE_BUTTON_THUMBR } });
  }

  // check if there are no buttons, hats or axes at all
  if (GetButtonCount() == 0 && GetHatCount() == 0 && GetAxisCount() == 0)
  {
    CLog::Log(LOGWARNING, "CAndroidJoystickState: no buttons, hats or axes detected for input device \"%s\" with ID %d", deviceName.c_str(), m_deviceId);
    return false;
  }

  m_state.buttons.assign(GetButtonCount(), JOYSTICK_STATE_BUTTON_UNPRESSED);
  m_state.hats.assign(GetHatCount(), JOYSTICK_STATE_HAT_UNPRESSED);
  m_state.axes.assign(GetAxisCount(), 0.0f);

  m_stateBuffer.buttons.assign(GetButtonCount(), JOYSTICK_STATE_BUTTON_UNPRESSED);
  m_stateBuffer.hats.assign(GetHatCount(), JOYSTICK_STATE_HAT_UNPRESSED);
  m_stateBuffer.axes.assign(GetAxisCount(), 0.0f);

  return true;
}

void CAndroidJoystickState::Deinitialize(void)
{
  m_buttons.clear();
  m_hats.clear();
  m_axes.clear();

  m_state.buttons.clear();
  m_state.hats.clear();
  m_state.axes.clear();

  m_stateBuffer.buttons.clear();
  m_stateBuffer.hats.clear();
  m_stateBuffer.axes.clear();
}

bool CAndroidJoystickState::ProcessEvent(const AInputEvent* event)
{
  int32_t type = AInputEvent_getType(event);

  switch (type)
  {
    case AINPUT_EVENT_TYPE_KEY:
    {
      int32_t keycode = AKeyEvent_getKeyCode(event);
      int32_t action = AKeyEvent_getAction(event);

      JOYSTICK_STATE_BUTTON buttonState = JOYSTICK_STATE_BUTTON_UNPRESSED;
      if (action == AKEY_EVENT_ACTION_DOWN)
        buttonState = JOYSTICK_STATE_BUTTON_PRESSED;
      CLog::Log(LOGDEBUG, "CAndroidJoystickState::ProcessEvent(type = key, keycode = %d, action = %d): %s",
                keycode, action, (buttonState == JOYSTICK_STATE_BUTTON_UNPRESSED ? "unpressed" : "pressed"));

      bool result = SetButtonValue(keycode, buttonState);

      // check if the key event belongs to the D-Pad which needs to be ignored
      // if we handle the D-Pad as a hat
      if (!result &&
         (keycode == AKEYCODE_DPAD_UP || keycode == AKEYCODE_DPAD_DOWN || keycode == AKEYCODE_DPAD_LEFT || keycode == AKEYCODE_DPAD_RIGHT) &&
         !m_hats.empty())
        return true;

      return result;
    }

    case AINPUT_EVENT_TYPE_MOTION:
    {
      size_t count = AMotionEvent_getPointerCount(event);

      bool success = false;
      for (size_t pointer = 0; pointer < count; ++pointer)
      {
        // process the hats
        for (const auto& hat : m_hats)
        {
          float valueX = AMotionEvent_getAxisValue(event, hat.ids[0], pointer);
          float valueY = AMotionEvent_getAxisValue(event, hat.ids[1], pointer);

          int hatValue = JOYSTICK_STATE_HAT_UNPRESSED;
          if (valueX < -hat.flat)
            hatValue |= JOYSTICK_STATE_HAT_LEFT;
          else if (valueX > hat.flat)
            hatValue |= JOYSTICK_STATE_HAT_RIGHT;
          if (valueY < -hat.flat)
            hatValue |= JOYSTICK_STATE_HAT_UP;
          else if (valueY > hat.flat)
            hatValue |= JOYSTICK_STATE_HAT_DOWN;

          success |= SetHatValue(hat.ids, static_cast<JOYSTICK_STATE_HAT>(hatValue));
        }

        // process all axes
        for (const auto& axis : m_axes)
        {
          // get all potential values
          std::vector<float> values;
          for (const auto& axisId : axis.ids)
            values.push_back(AMotionEvent_getAxisValue(event, axisId, pointer));

          // remove all zero values
          values.erase(std::remove(values.begin(), values.end(), 0.0f), values.end());

          float value = 0.0f;
          // pick the first non-zero value
          if (!values.empty())
            value = values.front();

          success |= SetAxisValue(axis.ids, value);
        }
      }
      return success;
    }

    default:
      CLog::Log(LOGWARNING, "CAndroidJoystickState: unknown input event type %d from input device with ID %d", type, m_deviceId);
      break;
  }

  return false;
}

void CAndroidJoystickState::GetEvents(std::vector<ADDON::PeripheralEvent>& events) const
{
  GetButtonEvents(events);
  GetHatEvents(events);
  GetAxisEvents(events);
}

void CAndroidJoystickState::GetButtonEvents(std::vector<ADDON::PeripheralEvent>& events) const
{
  const std::vector<JOYSTICK_STATE_BUTTON>& buttons = m_stateBuffer.buttons;

  for (unsigned int i = 0; i < buttons.size(); i++)
  {
    if (buttons[i] != m_state.buttons[i])
      events.push_back(ADDON::PeripheralEvent(m_deviceId, i, buttons[i]));
  }

  m_state.buttons.assign(buttons.begin(), buttons.end());
}

void CAndroidJoystickState::GetHatEvents(std::vector<ADDON::PeripheralEvent>& events) const
{
  const std::vector<JOYSTICK_STATE_HAT>& hats = m_stateBuffer.hats;

  for (unsigned int i = 0; i < hats.size(); i++)
  {
    if (hats[i] != m_state.hats[i])
      events.push_back(ADDON::PeripheralEvent(m_deviceId, i, hats[i]));
  }

  m_state.hats.assign(hats.begin(), hats.end());
}

void CAndroidJoystickState::GetAxisEvents(std::vector<ADDON::PeripheralEvent>& events) const
{
  const std::vector<JOYSTICK_STATE_AXIS>& axes = m_stateBuffer.axes;

  for (unsigned int i = 0; i < axes.size(); i++)
  {
    if (axes[i] != 0.0f || m_state.axes[i] != 0.0f)
      events.push_back(ADDON::PeripheralEvent(m_deviceId, i, axes[i]));
  }

  m_state.axes.assign(axes.begin(), axes.end());
}

bool CAndroidJoystickState::SetButtonValue(int axisId, JOYSTICK_STATE_BUTTON buttonValue)
{
  size_t buttonIndex = 0;
  if (!GetAxesIndex({ axisId }, m_buttons, buttonIndex) || buttonIndex >= GetButtonCount())
    return false;

  CLog::Log(LOGDEBUG, "CAndroidJoystickState: setting value for button %s to %d", PrintAxisIds(m_buttons[buttonIndex].ids).c_str(), buttonValue);
  m_stateBuffer.buttons[buttonIndex] = buttonValue;
  return true;
}

bool CAndroidJoystickState::SetHatValue(const std::vector<int>& axisIds, JOYSTICK_STATE_HAT hatValue)
{
  size_t hatIndex = 25;
  if (!GetAxesIndex(axisIds, m_hats, hatIndex) || hatIndex >= GetHatCount())
    return false;

  m_stateBuffer.hats[hatIndex] = hatValue;
  return true;
}

bool CAndroidJoystickState::SetAxisValue(const std::vector<int>& axisIds, JOYSTICK_STATE_AXIS axisValue)
{
  size_t axisIndex = 0;
  if (!GetAxesIndex(axisIds, m_axes, axisIndex) || axisIndex >= GetAxisCount())
    return false;

  const JoystickAxis& axis = m_axes[axisIndex];

  // make sure that the axis value is in the valid range
  axisValue = Contain(axisValue, axis.min, axis.max);
  // apply deadzoning
  axisValue = Deadzone(axisValue, axis.flat);
  // scale the axis value down to a value between -1.0f and 1.0f
  axisValue = Scale(axisValue, axis.max, 1.0f);

  m_stateBuffer.axes[axisIndex] = axisValue;
  return true;
}

float CAndroidJoystickState::Contain(float value, float min, float max)
{
  if (value < min)
    return min;
  if (value > max)
    return max;

  return value;
}

float CAndroidJoystickState::Scale(float value, float max, float scaledMax)
{
  return value * (scaledMax / max);
}

float CAndroidJoystickState::Deadzone(float value, float deadzone)
{
  if ((value > 0.0f && value < deadzone) ||
      (value < 0.0f && value > -deadzone))
    return 0.0f;

  return value;
}

CAndroidJoystickState::JoystickAxes::const_iterator CAndroidJoystickState::GetAxis(const std::vector<int>& axisIds, const JoystickAxes& axes)
{
  return std::find_if(axes.cbegin(), axes.cend(),
                     [&axisIds](const JoystickAxis& axis)
                     {
                       std::vector<int> matches(std::max(axisIds.size(), axis.ids.size()));
                       const auto& matchesEnd = std::set_intersection(axisIds.begin(), axisIds.end(),
                                                                      axis.ids.begin(), axis.ids.end(),
                                                                      matches.begin());
                       matches.resize(matchesEnd - matches.begin());
                       return !matches.empty();
                     });
}

bool CAndroidJoystickState::ContainsAxis(int axisId, const JoystickAxes& axes)
{
  return GetAxis({ axisId }, axes) != axes.cend();
}

bool CAndroidJoystickState::GetAxesIndex(const std::vector<int>& axisIds, const JoystickAxes& axes, size_t& axesIndex)
{
  auto axesIt = GetAxis(axisIds, axes);
  if (axesIt == axes.end())
    return false;

  axesIndex = std::distance(axes.begin(), axesIt);
  return true;
}
