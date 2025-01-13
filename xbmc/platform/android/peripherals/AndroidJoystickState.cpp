/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidJoystickState.h"

#include "AndroidJoystickTranslator.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/DefaultController.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>
#include <utility>

#include <android/input.h>
#include <androidjni/View.h>

using namespace KODI;
using namespace PERIPHERALS;

namespace
{
// clang-format off
static const std::vector<int> ButtonKeycodes{
    // add the usual suspects
    AKEYCODE_BUTTON_A,
    AKEYCODE_BUTTON_B,
    AKEYCODE_BUTTON_C,
    AKEYCODE_BUTTON_X,
    AKEYCODE_BUTTON_Y,
    AKEYCODE_BUTTON_Z,
    AKEYCODE_BACK,
    AKEYCODE_MENU,
    AKEYCODE_HOME,
    AKEYCODE_BUTTON_SELECT,
    AKEYCODE_BUTTON_MODE,
    AKEYCODE_BUTTON_START,
    AKEYCODE_BUTTON_L1,
    AKEYCODE_BUTTON_R1,
    AKEYCODE_BUTTON_L2,
    AKEYCODE_BUTTON_R2,
    AKEYCODE_BUTTON_THUMBL,
    AKEYCODE_BUTTON_THUMBR,
    AKEYCODE_DPAD_UP,
    AKEYCODE_DPAD_RIGHT,
    AKEYCODE_DPAD_DOWN,
    AKEYCODE_DPAD_LEFT,
    AKEYCODE_DPAD_CENTER,
    // add generic gamepad buttons for controllers that Android doesn't know
    // how to map
    AKEYCODE_BUTTON_1,
    AKEYCODE_BUTTON_2,
    AKEYCODE_BUTTON_3,
    AKEYCODE_BUTTON_4,
    AKEYCODE_BUTTON_5,
    AKEYCODE_BUTTON_6,
    AKEYCODE_BUTTON_7,
    AKEYCODE_BUTTON_8,
    AKEYCODE_BUTTON_9,
    AKEYCODE_BUTTON_10,
    AKEYCODE_BUTTON_11,
    AKEYCODE_BUTTON_12,
    AKEYCODE_BUTTON_13,
    AKEYCODE_BUTTON_14,
    AKEYCODE_BUTTON_15,
    AKEYCODE_BUTTON_16,
    // only add additional buttons at the end of the list
};
// clang-format on

// clang-format off
static const std::vector<int> AxisIDs{
    AMOTION_EVENT_AXIS_HAT_X,
    AMOTION_EVENT_AXIS_HAT_Y,
    AMOTION_EVENT_AXIS_X,
    AMOTION_EVENT_AXIS_Y,
    AMOTION_EVENT_AXIS_Z,
    AMOTION_EVENT_AXIS_RX,
    AMOTION_EVENT_AXIS_RY,
    AMOTION_EVENT_AXIS_RZ,
    AMOTION_EVENT_AXIS_LTRIGGER,
    AMOTION_EVENT_AXIS_RTRIGGER,
    AMOTION_EVENT_AXIS_GAS,
    AMOTION_EVENT_AXIS_BRAKE,
    AMOTION_EVENT_AXIS_THROTTLE,
    AMOTION_EVENT_AXIS_RUDDER,
    AMOTION_EVENT_AXIS_WHEEL,
    AMOTION_EVENT_AXIS_GENERIC_1,
    AMOTION_EVENT_AXIS_GENERIC_2,
    AMOTION_EVENT_AXIS_GENERIC_3,
    AMOTION_EVENT_AXIS_GENERIC_4,
    AMOTION_EVENT_AXIS_GENERIC_5,
    AMOTION_EVENT_AXIS_GENERIC_6,
    AMOTION_EVENT_AXIS_GENERIC_7,
    AMOTION_EVENT_AXIS_GENERIC_8,
    AMOTION_EVENT_AXIS_GENERIC_9,
    AMOTION_EVENT_AXIS_GENERIC_10,
    AMOTION_EVENT_AXIS_GENERIC_11,
    AMOTION_EVENT_AXIS_GENERIC_12,
    AMOTION_EVENT_AXIS_GENERIC_13,
    AMOTION_EVENT_AXIS_GENERIC_14,
    AMOTION_EVENT_AXIS_GENERIC_15,
    AMOTION_EVENT_AXIS_GENERIC_16,
};
// clang-format on

static void MapAxisIds(int axisId,
                       int primaryAxisId,
                       int secondaryAxisId,
                       std::vector<int>& axisIds)
{
  if (axisId != primaryAxisId && axisId != secondaryAxisId)
    return;

  if (axisIds.empty())
  {
    axisIds.emplace_back(primaryAxisId);
    axisIds.emplace_back(secondaryAxisId);
  }

  if (axisIds.size() > 1)
    return;

  if (axisId == primaryAxisId)
    axisIds.emplace_back(secondaryAxisId);
  else if (axisId == secondaryAxisId)
    axisIds.insert(axisIds.begin(), primaryAxisId);
}
} // namespace

CAndroidJoystickState::CAndroidJoystickState(CAndroidJoystickState&& other) noexcept
  : m_deviceId(other.m_deviceId),
    m_buttons(std::move(other.m_buttons)),
    m_axes(std::move(other.m_axes)),
    m_analogState(std::move(other.m_analogState)),
    m_digitalEvents(std::move(other.m_digitalEvents))
{
}

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
      CLog::Log(LOGDEBUG,
                "CAndroidJoystickState: axis {} has unexpected source {} for input device \"{}\" "
                "with ID {}",
                motionRange.getAxis(), motionRange.getSource(), deviceName, m_deviceId);
    }

    int axisId = motionRange.getAxis();
    JoystickAxis axis{{axisId},
                      motionRange.getFlat(),
                      motionRange.getFuzz(),
                      motionRange.getMin(),
                      motionRange.getMax(),
                      motionRange.getRange(),
                      motionRange.getResolution()};

    // check if the axis ID belongs to a D-pad, analogue stick, trigger or
    // generic axis
    if (std::find(AxisIDs.begin(), AxisIDs.end(), axisId) != AxisIDs.end())
    {
      CLog::Log(LOGDEBUG, "CAndroidJoystickState:     axis found: {} ({})",
                CAndroidJoystickTranslator::TranslateAxis(axisId), axisId);

      // check if this axis is already known
      if (ContainsAxis(axisId, m_axes))
        continue;

      // map AMOTION_EVENT_AXIS_GAS to AMOTION_EVENT_AXIS_RTRIGGER and
      // AMOTION_EVENT_AXIS_BRAKE to AMOTION_EVENT_AXIS_LTRIGGER
      // to avoid duplicate events on controllers sending both events
      MapAxisIds(axisId, AMOTION_EVENT_AXIS_LTRIGGER, AMOTION_EVENT_AXIS_BRAKE, axis.ids);
      MapAxisIds(axisId, AMOTION_EVENT_AXIS_RTRIGGER, AMOTION_EVENT_AXIS_GAS, axis.ids);

      m_axes.emplace_back(std::move(axis));
    }
    else
      CLog::Log(LOGWARNING,
                "CAndroidJoystickState: ignoring unknown axis {} on input device \"{}\" with ID {}",
                axisId, deviceName, m_deviceId);
  }

  // check for presence of buttons
  auto results = inputDevice.hasKeys(ButtonKeycodes);

  if (results.size() != ButtonKeycodes.size())
  {
    CLog::Log(LOGERROR, "CAndroidJoystickState: failed to get key status for {} buttons",
              ButtonKeycodes.size());
    return false;
  }

  // log positive results and assign results to buttons
  for (unsigned int i = 0; i < ButtonKeycodes.size(); ++i)
  {
    if (results[i])
    {
      const int buttonKeycode = ButtonKeycodes[i];
      CLog::Log(LOGDEBUG, "CAndroidJoystickState:     button found: {} ({})",
                CAndroidJoystickTranslator::TranslateKeyCode(buttonKeycode), buttonKeycode);
      m_buttons.emplace_back(JoystickAxis{{buttonKeycode}});
    }
  }

  // check if there are no buttons or axes at all
  if (GetButtonCount() == 0 && GetAxisCount() == 0)
  {
    CLog::Log(LOGWARNING,
              "CAndroidJoystickState: no buttons, hats or axes detected for input device \"{}\" "
              "with ID {}",
              deviceName, m_deviceId);
    return false;
  }

  m_analogState.assign(GetAxisCount(), 0.0f);

  return true;
}

void CAndroidJoystickState::Deinitialize(void)
{
  m_buttons.clear();
  m_axes.clear();

  m_analogState.clear();
  m_digitalEvents.clear();
}

bool CAndroidJoystickState::InitializeButtonMap(JOYSTICK::IButtonMap& buttonMap) const
{
  // We only map the default controller
  if (buttonMap.ControllerID() != GAME::DEFAULT_CONTROLLER_ID)
    return false;

  bool success = false;

  // Map buttons
  for (int buttonKeycode : ButtonKeycodes)
    success |= MapButton(buttonMap, buttonKeycode);

  // Map D-pad
  success |= MapDpad(buttonMap, AMOTION_EVENT_AXIS_HAT_X, AMOTION_EVENT_AXIS_HAT_Y);

  // Map triggers
  // Note: This should come after buttons, because the PS4 controller uses
  // both a digital button and an analog axis for the triggers, and we want
  // the analog axis to override the button for full range of motion.
  success |= MapTrigger(buttonMap, AMOTION_EVENT_AXIS_LTRIGGER,
                        GAME::CDefaultController::FEATURE_LEFT_TRIGGER);
  success |= MapTrigger(buttonMap, AMOTION_EVENT_AXIS_RTRIGGER,
                        GAME::CDefaultController::FEATURE_RIGHT_TRIGGER);

  // Map analog sticks
  success |= MapAnalogStick(buttonMap, AMOTION_EVENT_AXIS_X, AMOTION_EVENT_AXIS_Y,
                            GAME::CDefaultController::FEATURE_LEFT_STICK);
  success |= MapAnalogStick(buttonMap, AMOTION_EVENT_AXIS_Z, AMOTION_EVENT_AXIS_RZ,
                            GAME::CDefaultController::FEATURE_RIGHT_STICK);

  const std::string controllerId = GetAppearance();

  // Handle PS controller triggers
  if (controllerId == GAME::CONTROLLER_ID_PLAYSTATION)
  {
    size_t indexL2 = 0;
    size_t indexR2 = 0;
    if (GetAxesIndex({AKEYCODE_BUTTON_L2}, m_buttons, indexL2) &&
        GetAxesIndex({AKEYCODE_BUTTON_R2}, m_buttons, indexR2))
    {
      CLog::Log(LOGDEBUG, "Detected dual-input triggers, ignoring digital buttons");
      std::vector<JOYSTICK::CDriverPrimitive> ignoredPrimitives{
          {JOYSTICK::PRIMITIVE_TYPE::BUTTON, static_cast<unsigned int>(indexL2)},
          {JOYSTICK::PRIMITIVE_TYPE::BUTTON, static_cast<unsigned int>(indexR2)},
      };
      buttonMap.SetIgnoredPrimitives(ignoredPrimitives);
      success = true;
    }
  }

  if (!controllerId.empty())
  {
    CLog::Log(LOGDEBUG, "Setting appearance to {}", controllerId);
    success |= buttonMap.SetAppearance(controllerId);
  }

  if (success)
  {
    // Save the buttonmap
    buttonMap.SaveButtonMap();
  }

  return success;
}

std::string CAndroidJoystickState::GetAppearance() const
{
  // If the controller has both L2/R2 buttons and LTRIGGER/RTRIGGER axes, it's
  // probably a PS controller
  size_t indexL2 = 0;
  size_t indexR2 = 0;
  size_t indexLTrigger = 0;
  size_t indexRTrigger = 0;
  if (GetAxesIndex({AKEYCODE_BUTTON_L2}, m_buttons, indexL2) &&
      GetAxesIndex({AKEYCODE_BUTTON_R2}, m_buttons, indexR2) &&
      GetAxesIndex({AMOTION_EVENT_AXIS_LTRIGGER}, m_axes, indexLTrigger) &&
      GetAxesIndex({AMOTION_EVENT_AXIS_RTRIGGER}, m_axes, indexRTrigger))
  {
    return GAME::CONTROLLER_ID_PLAYSTATION;
  }

  return "";
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

      CLog::Log(LOGDEBUG, "Android Key {} ({}) {}",
                CAndroidJoystickTranslator::TranslateKeyCode(keycode), keycode,
                (buttonState == JOYSTICK_STATE_BUTTON_UNPRESSED ? "released" : "pressed"));

      bool result = SetButtonValue(keycode, buttonState);

      return result;
    }

    case AINPUT_EVENT_TYPE_MOTION:
    {
      size_t count = AMotionEvent_getPointerCount(event);

      bool success = false;
      for (size_t pointer = 0; pointer < count; ++pointer)
      {
        // process all axes
        for (const auto& axis : m_axes)
        {
          // get all potential values
          std::vector<float> values;
          values.reserve(axis.ids.size());
          for (const auto& axisId : axis.ids)
            values.emplace_back(AMotionEvent_getAxisValue(event, axisId, pointer));

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
      CLog::Log(LOGWARNING,
                "CAndroidJoystickState: unknown input event type {} from input device with ID {}",
                type, m_deviceId);
      break;
  }

  return false;
}

void CAndroidJoystickState::GetEvents(std::vector<kodi::addon::PeripheralEvent>& events)
{
  GetButtonEvents(events);
  GetAxisEvents(events);
}

void CAndroidJoystickState::GetButtonEvents(std::vector<kodi::addon::PeripheralEvent>& events)
{
  std::unique_lock<CCriticalSection> lock(m_eventMutex);

  // Only report a single event per button (avoids dropping rapid presses)
  std::vector<kodi::addon::PeripheralEvent> repeatButtons;

  for (const auto& digitalEvent : m_digitalEvents)
  {
    auto HasButton = [&digitalEvent](const kodi::addon::PeripheralEvent& event)
    {
      if (event.Type() == PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON)
        return event.DriverIndex() == digitalEvent.DriverIndex();
      return false;
    };

    if (std::find_if(events.begin(), events.end(), HasButton) == events.end())
      events.emplace_back(digitalEvent);
    else
      repeatButtons.emplace_back(digitalEvent);
  }

  m_digitalEvents.swap(repeatButtons);
}

void CAndroidJoystickState::GetAxisEvents(std::vector<kodi::addon::PeripheralEvent>& events) const
{
  for (unsigned int i = 0; i < m_analogState.size(); i++)
    events.emplace_back(m_deviceId, i, m_analogState[i]);
}

bool CAndroidJoystickState::SetButtonValue(int axisId, JOYSTICK_STATE_BUTTON buttonValue)
{
  size_t buttonIndex = 0;
  if (!GetAxesIndex({axisId}, m_buttons, buttonIndex) || buttonIndex >= GetButtonCount())
    return false;

  std::unique_lock<CCriticalSection> lock(m_eventMutex);

  m_digitalEvents.emplace_back(m_deviceId, buttonIndex, buttonValue);

  return true;
}

bool CAndroidJoystickState::SetAxisValue(const std::vector<int>& axisIds,
                                         JOYSTICK_STATE_AXIS axisValue)
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

  m_analogState[axisIndex] = axisValue;
  return true;
}

bool CAndroidJoystickState::MapButton(JOYSTICK::IButtonMap& buttonMap, int buttonKeycode) const
{
  size_t buttonIndex = 0;
  std::string featureName;

  if (!GetAxesIndex({buttonKeycode}, m_buttons, buttonIndex))
    return false;

  // Check if button is already mapped
  JOYSTICK::CDriverPrimitive buttonPrimitive{JOYSTICK::PRIMITIVE_TYPE::BUTTON,
                                             static_cast<unsigned int>(buttonIndex)};
  if (buttonMap.GetFeature(buttonPrimitive, featureName))
    return false;

  // Translate the button
  std::string controllerButton = CAndroidJoystickTranslator::TranslateJoystickButton(buttonKeycode);
  if (controllerButton.empty())
    return false;

  // Check if feature is already mapped
  if (buttonMap.GetFeatureType(controllerButton) != JOYSTICK::FEATURE_TYPE::UNKNOWN)
    return false;

  // Map the button
  CLog::Log(LOGDEBUG, "Automatically mapping {} to {}", controllerButton,
            buttonPrimitive.ToString());
  buttonMap.AddScalar(controllerButton, buttonPrimitive);

  return true;
}

bool CAndroidJoystickState::MapTrigger(JOYSTICK::IButtonMap& buttonMap,
                                       int axisId,
                                       const std::string& triggerName) const
{
  size_t axisIndex = 0;
  std::string featureName;

  if (!GetAxesIndex({axisId}, m_axes, axisIndex))
    return false;

  const JOYSTICK::CDriverPrimitive semiaxis{static_cast<unsigned int>(axisIndex), 0,
                                            JOYSTICK::SEMIAXIS_DIRECTION::POSITIVE, 1};
  if (buttonMap.GetFeature(semiaxis, featureName))
    return false;

  CLog::Log(LOGDEBUG, "Automatically mapping {} to {}", triggerName, semiaxis.ToString());
  buttonMap.AddScalar(triggerName, semiaxis);

  return true;
}

bool CAndroidJoystickState::MapDpad(JOYSTICK::IButtonMap& buttonMap,
                                    int horizAxisId,
                                    int vertAxisId) const
{
  bool success = false;

  size_t axisIndex = 0;
  std::string featureName;

  // Map horizontal axis
  if (GetAxesIndex({horizAxisId}, m_axes, axisIndex))
  {
    const JOYSTICK::CDriverPrimitive positiveSemiaxis{static_cast<unsigned int>(axisIndex), 0,
                                                      JOYSTICK::SEMIAXIS_DIRECTION::POSITIVE, 1};
    const JOYSTICK::CDriverPrimitive negativeSemiaxis{static_cast<unsigned int>(axisIndex), 0,
                                                      JOYSTICK::SEMIAXIS_DIRECTION::NEGATIVE, 1};
    if (!buttonMap.GetFeature(positiveSemiaxis, featureName) &&
        !buttonMap.GetFeature(negativeSemiaxis, featureName))
    {
      CLog::Log(LOGDEBUG, "Automatically mapping {} to {}", GAME::CDefaultController::FEATURE_LEFT,
                negativeSemiaxis.ToString());
      CLog::Log(LOGDEBUG, "Automatically mapping {} to {}", GAME::CDefaultController::FEATURE_RIGHT,
                positiveSemiaxis.ToString());
      buttonMap.AddScalar(GAME::CDefaultController::FEATURE_LEFT, negativeSemiaxis);
      buttonMap.AddScalar(GAME::CDefaultController::FEATURE_RIGHT, positiveSemiaxis);
      success |= true;
    }
  }

  // Map vertical axis
  if (GetAxesIndex({vertAxisId}, m_axes, axisIndex))
  {
    const JOYSTICK::CDriverPrimitive positiveSemiaxis{static_cast<unsigned int>(axisIndex), 0,
                                                      JOYSTICK::SEMIAXIS_DIRECTION::POSITIVE, 1};
    const JOYSTICK::CDriverPrimitive negativeSemiaxis{static_cast<unsigned int>(axisIndex), 0,
                                                      JOYSTICK::SEMIAXIS_DIRECTION::NEGATIVE, 1};
    if (!buttonMap.GetFeature(positiveSemiaxis, featureName) &&
        !buttonMap.GetFeature(negativeSemiaxis, featureName))
    {
      CLog::Log(LOGDEBUG, "Automatically mapping {} to {}", GAME::CDefaultController::FEATURE_UP,
                negativeSemiaxis.ToString());
      CLog::Log(LOGDEBUG, "Automatically mapping {} to {}", GAME::CDefaultController::FEATURE_DOWN,
                positiveSemiaxis.ToString());
      buttonMap.AddScalar(GAME::CDefaultController::FEATURE_DOWN, positiveSemiaxis);
      buttonMap.AddScalar(GAME::CDefaultController::FEATURE_UP, negativeSemiaxis);
      success |= true;
    }
  }

  return success;
}

bool CAndroidJoystickState::MapAnalogStick(JOYSTICK::IButtonMap& buttonMap,
                                           int horizAxisId,
                                           int vertAxisId,
                                           const std::string& analogStickName) const
{
  size_t axisIndex1 = 0;
  size_t axisIndex2 = 0;
  std::string featureName;

  if (!GetAxesIndex({horizAxisId}, m_axes, axisIndex1) ||
      !GetAxesIndex({vertAxisId}, m_axes, axisIndex2))
    return false;

  const JOYSTICK::CDriverPrimitive upSemiaxis{static_cast<unsigned int>(axisIndex2), 0,
                                              JOYSTICK::SEMIAXIS_DIRECTION::NEGATIVE, 1};
  const JOYSTICK::CDriverPrimitive downSemiaxis{static_cast<unsigned int>(axisIndex2), 0,
                                                JOYSTICK::SEMIAXIS_DIRECTION::POSITIVE, 1};
  const JOYSTICK::CDriverPrimitive leftSemiaxis{static_cast<unsigned int>(axisIndex1), 0,
                                                JOYSTICK::SEMIAXIS_DIRECTION::NEGATIVE, 1};
  const JOYSTICK::CDriverPrimitive rightSemiaxis{static_cast<unsigned int>(axisIndex1), 0,
                                                 JOYSTICK::SEMIAXIS_DIRECTION::POSITIVE, 1};
  if (buttonMap.GetFeature(upSemiaxis, featureName) ||
      buttonMap.GetFeature(downSemiaxis, featureName) ||
      buttonMap.GetFeature(leftSemiaxis, featureName) ||
      buttonMap.GetFeature(rightSemiaxis, featureName))
    return false;

  CLog::Log(LOGDEBUG, "Automatically mapping {} to [{}, {}, {}, {}]", analogStickName,
            upSemiaxis.ToString(), downSemiaxis.ToString(), leftSemiaxis.ToString(),
            rightSemiaxis.ToString());
  buttonMap.AddAnalogStick(analogStickName, JOYSTICK::ANALOG_STICK_DIRECTION::UP, upSemiaxis);
  buttonMap.AddAnalogStick(analogStickName, JOYSTICK::ANALOG_STICK_DIRECTION::DOWN, downSemiaxis);
  buttonMap.AddAnalogStick(analogStickName, JOYSTICK::ANALOG_STICK_DIRECTION::LEFT, leftSemiaxis);
  buttonMap.AddAnalogStick(analogStickName, JOYSTICK::ANALOG_STICK_DIRECTION::RIGHT, rightSemiaxis);

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
  if ((value > 0.0f && value < deadzone) || (value < 0.0f && value > -deadzone))
    return 0.0f;

  return value;
}

CAndroidJoystickState::JoystickAxes::const_iterator CAndroidJoystickState::GetAxis(
    const std::vector<int>& axisIds, const JoystickAxes& axes)
{
  return std::find_if(axes.cbegin(), axes.cend(),
                      [&axisIds](const JoystickAxis& axis)
                      {
                        std::vector<int> matches(std::max(axisIds.size(), axis.ids.size()));
                        const auto& matchesEnd =
                            std::set_intersection(axisIds.begin(), axisIds.end(), axis.ids.begin(),
                                                  axis.ids.end(), matches.begin());
                        matches.resize(matchesEnd - matches.begin());
                        return !matches.empty();
                      });
}

bool CAndroidJoystickState::ContainsAxis(int axisId, const JoystickAxes& axes)
{
  return GetAxis({axisId}, axes) != axes.cend();
}

bool CAndroidJoystickState::GetAxesIndex(const std::vector<int>& axisIds,
                                         const JoystickAxes& axes,
                                         size_t& axesIndex)
{
  auto axesIt = GetAxis(axisIds, axes);
  if (axesIt == axes.end())
    return false;

  axesIndex = std::distance(axes.begin(), axesIt);
  return true;
}
