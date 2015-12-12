/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AndroidJoyStick.h"
#include "AndroidExtra.h"
#include "XBMCApp.h"
#include "platform/android/jni/View.h"
#include "platform/android/activity/AndroidFeatures.h"
#include "utils/log.h"
#include "windowing/WinEvents.h"
#include "windowing/XBMC_events.h"
#include "utils/TimeUtils.h"

#include <android/input.h>


#include <math.h>

//#define DEBUG_VERBOSE

// mapping to axis IDs codes in keymaps.xmls
enum {
  AXIS_LEFT_STICK_L_R  = 1,
  AXIS_LEFT_STICK_U_D  = 2,
  AXIS_TRIGGER         = 3,
  AXIS_RIGHT_STICK_L_R = 4,
  AXIS_RIGHT_STICK_U_D = 5,
};

typedef struct {
  int32_t nativeKey;
  int16_t xbmcID;
} KeyMap;

// mapping to button codes in keymaps.xmls
static const KeyMap ButtonMap[] = {
  { AKEYCODE_BUTTON_A        , 1 },
  { AKEYCODE_BUTTON_B        , 2 },
  { AKEYCODE_BUTTON_X        , 3 },
  { AKEYCODE_BUTTON_Y        , 4 },
  { AKEYCODE_BUTTON_L1       , 5 },
  { AKEYCODE_BUTTON_R1       , 6 },
  { AKEYCODE_BUTTON_SELECT   , 7 },
  { AKEYCODE_BUTTON_START    , 8 },
  { AKEYCODE_BUTTON_THUMBL   , 9 },
  { AKEYCODE_BUTTON_THUMBR   , 10 },
  { AKEYCODE_DPAD_UP         , 11 },
  { AKEYCODE_DPAD_DOWN       , 12 },
  { AKEYCODE_DPAD_LEFT       , 13 },
  { AKEYCODE_DPAD_RIGHT      , 14 },
  { AKEYCODE_BUTTON_L2       , 16 },
  { AKEYCODE_BUTTON_R2       , 17 },
  { AKEYCODE_BUTTON_C        , 51 },
  { AKEYCODE_BUTTON_Z        , 52 },
};

/************************************************************************/
/************************************************************************/
static float AxisClampAsButton(const APP_InputDeviceAxis &axis, float value)
{
  // Clamp Axis so it acts like a D-Pad, return -1, 0 or +1
  if (fabs(value) < axis.buttonclamp)
    return 0.0;
  else
    return value < 0.0 ? -1.0:1.0;
}

static void LogAxisValues(int axis_id, const APP_InputDeviceAxis &axis)
{
  CLog::Log(LOGDEBUG, "LogAxisValues: "
    "axis(%d) Enabled(%d) Max(%f) Min(%f) Range(%f) Flat(%f) Fuzz(%f)",
    axis_id, axis.enabled, axis.max, axis.min, axis.range, axis.flat, axis.fuzz);
}

static void SetAxisFromValues(const float min, const float max,
  const float flat, const float fuzz, const float range, APP_InputDeviceAxis &axis)
{
  axis.min  = min;
  axis.max  = max;
  axis.flat = flat;
  axis.fuzz = fuzz;
  axis.range= range;
  // precalc some internals
  axis.deadzone= axis.flat + axis.fuzz;
  if (axis.deadzone < 0.1f)
    axis.deadzone = 0.1f;
  axis.buttonclamp = axis.range / 4.0f;
}

static void SetupAxis(const CJNIViewInputDevice &input_device, APP_InputDeviceAxis &axis, int axis_id, int source)
{
  CJNIViewInputDeviceMotionRange range = input_device.getMotionRange(axis_id, source);

  SetAxisFromValues(range.getMin(), range.getMax(), range.getFlat(), range.getFuzz(), range.getRange(), axis);
  axis.enabled = true;
}

static void SetupJoySticks(APP_InputDeviceAxes *axes, int device)
{
  axes->id = device;
  memset(&axes->x_hat,  0x00, sizeof(APP_InputDeviceAxis));
  memset(&axes->y_hat,  0x00, sizeof(APP_InputDeviceAxis));
  memset(&axes->x_axis, 0x00, sizeof(APP_InputDeviceAxis));
  memset(&axes->y_axis, 0x00, sizeof(APP_InputDeviceAxis));
  memset(&axes->z_axis, 0x00, sizeof(APP_InputDeviceAxis));
  memset(&axes->rz_axis,0x00, sizeof(APP_InputDeviceAxis));

  CJNIViewInputDevice  input_device = CJNIViewInputDevice::getDevice(axes->id);
  int device_sources = input_device.getSources();
  std::string device_name = input_device.getName();

  CLog::Log(LOGDEBUG, "SetupJoySticks:caching  id(%d), sources(%d), device(%s)",
    axes->id, device_sources, device_name.c_str());

  CJNIList<CJNIViewInputDeviceMotionRange> device_ranges = input_device.getMotionRanges();
  for (int i = 0; i < device_ranges.size(); i++)
  {
    int axis = device_ranges.get(i).getAxis();
    int source = device_ranges.get(i).getSource();
#ifdef DEBUG_VERBOSE
    CLog::Log(LOGDEBUG, "SetupJoySticks:range(%d), axis(%d), source(%d)", i, axis, source);
#endif

    // ignore anything we do not understand
    if (source != AINPUT_SOURCE_JOYSTICK)
      continue;

    // match axis/source to our handlers
    // anything that is not present, will be disabled
    switch(axis)
    {
      // Left joystick
      case AMOTION_EVENT_AXIS_X:
        SetupAxis(input_device, axes->x_axis,  axis, source);
        break;
      break;
      case AMOTION_EVENT_AXIS_Y:
        SetupAxis(input_device, axes->y_axis,  axis, source);
        break;

      // Right joystick
      case AMOTION_EVENT_AXIS_Z:
        SetupAxis(input_device, axes->z_axis,  axis, source);
        break;
      case AMOTION_EVENT_AXIS_RZ:
        SetupAxis(input_device, axes->rz_axis, axis, source);
        break;

      // D-Pad
      case AMOTION_EVENT_AXIS_HAT_X:
        SetupAxis(input_device, axes->x_hat,   axis, source);
        break;
      case AMOTION_EVENT_AXIS_HAT_Y:
        SetupAxis(input_device, axes->y_hat,   axis, source);
      break;
    }
  }

  if (device_name.find("GameStick Controller") != std::string::npos)
  {
    // Right joystick seems to have a range of -0.5 to 0.5, fix the range
    // Production GameStick Controllers should not have this problem
    // and this quirk can vanish once verified.
    SetAxisFromValues(-0.5f, 0.5f, 0.1f, 0.0f, 1.0f, axes->z_axis);
    SetAxisFromValues(-0.5f, 0.5f, 0.1f, 0.0f, 1.0f, axes->rz_axis);
  }

#ifdef DEBUG_VERBOSE
  LogAxisValues(AMOTION_EVENT_AXIS_X,     axes->x_axis);
  LogAxisValues(AMOTION_EVENT_AXIS_Y,     axes->y_axis);
  LogAxisValues(AMOTION_EVENT_AXIS_Z,     axes->z_axis);
  LogAxisValues(AMOTION_EVENT_AXIS_RZ,    axes->rz_axis);
  LogAxisValues(AMOTION_EVENT_AXIS_HAT_X, axes->x_hat);
  LogAxisValues(AMOTION_EVENT_AXIS_HAT_Y, axes->y_hat);
#endif
}

/************************************************************************/
/************************************************************************/
CAndroidJoyStick::CAndroidJoyStick()
  : m_prev_device(0)
  , m_prev_button(0)
  , m_prev_holdtime(0)
{
}

CAndroidJoyStick::~CAndroidJoyStick()
{
  while (!m_input_devices.empty())
  {
    APP_InputDeviceAxes *device_axes = m_input_devices.back();
    delete device_axes;
    m_input_devices.pop_back();
  }
}

bool CAndroidJoyStick::onJoyStickKeyEvent(AInputEvent *event)
{
  if (event == NULL)
    return false;

  int32_t keycode = AKeyEvent_getKeyCode(event);
  // watch this check, others might be different.
  // AML IR Controller is       AINPUT_SOURCE_GAMEPAD | AINPUT_SOURCE_KEYBOARD | AINPUT_SOURCE_DPAD
  // Gamestick Controller    == AINPUT_SOURCE_GAMEPAD | AINPUT_SOURCE_KEYBOARD
  // NVidiaShield Controller == AINPUT_SOURCE_GAMEPAD | AINPUT_SOURCE_KEYBOARD
  // we want to reject AML IR Controller.
  if (AInputEvent_getSource(event) == (AINPUT_SOURCE_GAMEPAD | AINPUT_SOURCE_KEYBOARD))
  {
    // GamePad events are AINPUT_EVENT_TYPE_KEY events,
    // trap them here and revector valid ones as JoyButtons
    // so we get keymap handling.
    for (size_t i = 0; i < sizeof(ButtonMap) / sizeof(KeyMap); i++)
    {
      if (keycode == ButtonMap[i].nativeKey)
      {
        uint32_t holdtime = 0;
        uint8_t  button = ButtonMap[i].xbmcID;
        int32_t  action = AKeyEvent_getAction(event);
        int32_t  device = AInputEvent_getDeviceId(event);

        if ((action == AKEY_EVENT_ACTION_UP))
        {
          // ProcessJoystickEvent does not understand up, ignore it.
          m_prev_holdtime = m_prev_device = m_prev_button = 0;
          return false;
        }
        else
        {
          if (m_prev_holdtime && device == m_prev_device && button == m_prev_button)
          {
            holdtime = CTimeUtils::GetFrameTime() - m_prev_holdtime;
          }
          else
          {
            m_prev_holdtime = CTimeUtils::GetFrameTime();
            m_prev_device = device;
            m_prev_button = button;
          }
        }

        XBMC_JoyButton(device, button, holdtime, action == AKEY_EVENT_ACTION_UP);
        return true;
      }
    }
  }

  return false;
}

bool CAndroidJoyStick::onJoyStickMotionEvent(AInputEvent *event)
{
  if (event == NULL)
    return false;

  // match this device to a created device struct,
  // create it if we do not find it.
  APP_InputDeviceAxes *device_axes = NULL;
  int32_t device = AInputEvent_getDeviceId(event);
  // look for device name in our inputdevice cache.
  for (size_t i = 0; i < m_input_devices.size(); i++)
  {
    if (m_input_devices[i]->id == device)
      device_axes = m_input_devices[i];
  }
  if (!device_axes)
  {
    // as we see each axis, create a device axes and cache it.
    device_axes = new APP_InputDeviceAxes;
    SetupJoySticks(device_axes, device);
    m_input_devices.push_back(device_axes);
  }

  // handle queued motion events, we
  // ingnore history as it only relates to touch.
  for (size_t p = 0; p < AMotionEvent_getPointerCount(event); p++)
    ProcessMotionEvents(event, p, device, device_axes);

  return true;
}

void CAndroidJoyStick::ProcessMotionEvents(AInputEvent *event,
  size_t pointer_index, int32_t device, APP_InputDeviceAxes *axes)
{
  // Left joystick
  if (axes->y_axis.enabled)
    ProcessAxis(event, pointer_index, axes->y_axis, device, AXIS_LEFT_STICK_L_R, AMOTION_EVENT_AXIS_Y);
  if (axes->x_axis.enabled)
    ProcessAxis(event, pointer_index, axes->x_axis, device, AXIS_LEFT_STICK_U_D, AMOTION_EVENT_AXIS_X);

  // Right joystick
  if (axes->z_axis.enabled)
    ProcessAxis(event, pointer_index, axes->z_axis, device, AXIS_RIGHT_STICK_L_R, AMOTION_EVENT_AXIS_Z);
  if (axes->rz_axis.enabled)
    ProcessAxis(event, pointer_index, axes->rz_axis,device, AXIS_RIGHT_STICK_U_D, AMOTION_EVENT_AXIS_RZ);

  // Dpad
  if (axes->y_hat.enabled)
    ProcessHat(event, pointer_index,  axes->y_hat,  device, AMOTION_EVENT_AXIS_HAT_Y);
  if (axes->x_hat.enabled)
    ProcessHat(event, pointer_index,  axes->x_hat,  device, AMOTION_EVENT_AXIS_HAT_X);

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "joystick event. x(%f),  y(%f)", axes->x_axis.value, axes->y_axis.value);
  CLog::Log(LOGDEBUG, "joystick event. z(%f), rz(%f)", axes->z_axis.value, axes->rz_axis.value);
  CLog::Log(LOGDEBUG, "joystick event. xhat(%f), yhat(%f)", axes->x_hat.value, axes->y_hat.value);
#endif
}

bool CAndroidJoyStick::ProcessHat(AInputEvent *event, size_t pointer_index,
  APP_InputDeviceAxis &hat, int device, int android_axis)
{
  bool rtn = false;
  // Dpad (quantized to -1.0, 0.0 and 1.0)
  float value = AMotionEvent_getAxisValue(event, android_axis, pointer_index);
  if (value != hat.value)
  {
    u_int8_t hatvalue = XBMC_HAT_CENTERED;
    if (value != 0)
      switch (android_axis)
      {
        case AMOTION_EVENT_AXIS_HAT_X:
          if (value < 0)
            hatvalue |= XBMC_HAT_LEFT;
          else
            hatvalue |= XBMC_HAT_RIGHT;
          break;

        case AMOTION_EVENT_AXIS_HAT_Y:
          if (value < 0)
            hatvalue |= XBMC_HAT_UP;
          else
            hatvalue |= XBMC_HAT_DOWN;
          break;
      }

    XBMC_JoyHat(device, hatvalue);
    rtn = true;
  }
  hat.value = value;

  return rtn;
}

bool CAndroidJoyStick::ProcessAxis(AInputEvent *event, size_t pointer_index,
  APP_InputDeviceAxis &axis, int device, int keymap_axis, int android_axis)
{
  bool rtn = false;

  float value = AMotionEvent_getAxisValue(event, android_axis, pointer_index);
  //CLog::Log(LOGDEBUG, "ProcessAxis: keymap_axis(%d), value(%f)", keymap_axis, value);

  value = AxisClampAsButton(axis, value);
  if (value != axis.value)
  {
    XBMC_JoyAxis(device, keymap_axis, value);
    rtn = true;
  }
  axis.value = value;

  return rtn;
}

void CAndroidJoyStick::XBMC_JoyAxis(uint8_t device, uint8_t axis, float value)
{
  XBMC_Event newEvent = {};

  newEvent.type       = XBMC_JOYAXISMOTION;
  newEvent.jaxis.type = XBMC_JOYAXISMOTION;
  newEvent.jaxis.which  = device;
  newEvent.jaxis.axis   = axis;
  newEvent.jaxis.fvalue = value;

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "XBMC_Axis(%u, %u, %u, %f)", newEvent.type, device, axis, value);
#endif
  CWinEvents::MessagePush(&newEvent);
}

void CAndroidJoyStick::XBMC_JoyHat(uint8_t device, uint8_t value)
{
  XBMC_Event newEvent = {};

  newEvent.type       = XBMC_JOYHATMOTION;
  newEvent.jhat.type = XBMC_JOYHATMOTION;
  newEvent.jhat.which  = device;
  newEvent.jhat.hat   = 1;
  newEvent.jhat.value = value;

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "XBMC_Hat(%u, %u, %u)", newEvent.type, device, value);
#endif
  CWinEvents::MessagePush(&newEvent);
}

void CAndroidJoyStick::XBMC_JoyButton(uint8_t device, uint8_t button, uint32_t holdtime, bool up)
{
  XBMC_Event newEvent = {};

  unsigned char type = up ? XBMC_JOYBUTTONUP : XBMC_JOYBUTTONDOWN;
  newEvent.type = type;
  newEvent.jbutton.type = type;
  newEvent.jbutton.which  = device;
  newEvent.jbutton.button = button;
  newEvent.jbutton.holdTime = holdtime;

#ifdef DEBUG_VERBOSE
  CXBMCApp::android_printf("CAndroidJoyStick::XBMC_JoyButton(%u, %u, %u, %d)",
    newEvent.jbutton.type, newEvent.jbutton.which, newEvent.jbutton.button, newEvent.jbutton.holdTime);
#endif

  CWinEvents::MessagePush(&newEvent);
}
