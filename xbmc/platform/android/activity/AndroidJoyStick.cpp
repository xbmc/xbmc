/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidJoyStick.h"

#include "platform/android/activity/XBMCApp.h"

#include <android/input.h>

bool CAndroidJoyStick::onJoyStickEvent(AInputEvent* event)
{
  int32_t source = AInputEvent_getSource(event);

  // only handle input events from a gamepad or joystick
  if ((source & (AINPUT_SOURCE_GAMEPAD | AINPUT_SOURCE_JOYSTICK)) != 0)
    return CXBMCApp::onInputDeviceEvent(event);

  CXBMCApp::android_printf("CAndroidJoyStick::onJoyStickEvent(type = %d, keycode = %d, source = %d): ignoring non-joystick input event",
                           AInputEvent_getType(event), AKeyEvent_getKeyCode(event), source);
  return false;
}
