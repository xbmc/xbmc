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

#include <android/input.h>

#include "AndroidJoyStick.h"
#include "platform/android/activity/XBMCApp.h"

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
