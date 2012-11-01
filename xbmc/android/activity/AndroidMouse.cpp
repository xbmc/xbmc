/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include "AndroidMouse.h"
#include "XBMCApp.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "windowing/WinEvents.h"

CAndroidMouse::CAndroidMouse()
{
}

CAndroidMouse::~CAndroidMouse()
{
}

bool CAndroidMouse::onMouseEvent(AInputEvent* event)
{
  if (event == NULL)
    return false;

  int32_t eventAction = AMotionEvent_getAction(event);
  int8_t mouseAction = eventAction & AMOTION_EVENT_ACTION_MASK;
  size_t mousePointer = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

  CXBMCApp::android_printf("%s pointer:%i", __PRETTY_FUNCTION__, mousePointer);
  float x = AMotionEvent_getX(event, mousePointer);
  float y = AMotionEvent_getY(event, mousePointer);

  switch (mouseAction)
  {
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_DOWN:
      MouseButton(x,y,mouseAction);
      return true;
    default:
      MouseMove(x,y);
      return true;
  }
  return false;
}

void CAndroidMouse::MouseMove(float x, float y)
{
  CXBMCApp::android_printf("%s: x:%f, y:%f", __PRETTY_FUNCTION__, x, y);
  XBMC_Event newEvent;

  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.type = XBMC_MOUSEMOTION;
  newEvent.motion.type = XBMC_MOUSEMOTION;
  newEvent.motion.which = 0;
  newEvent.motion.state = 0;
  newEvent.motion.x = x;
  newEvent.motion.y = y;
  newEvent.motion.xrel = 0;
  newEvent.motion.yrel = 0;
  CWinEventsAndroid::MessagePush(&newEvent);
}

void CAndroidMouse::MouseButton(float x, float y, int32_t action)
{
  CXBMCApp::android_printf("%s: x:%f, y:%f, action:%i", __PRETTY_FUNCTION__, x, y, action);
  XBMC_Event newEvent;

  memset(&newEvent, 0, sizeof(newEvent));

  newEvent.type = (action ==  AMOTION_EVENT_ACTION_DOWN) ? XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  newEvent.button.state = (action ==  AMOTION_EVENT_ACTION_DOWN) ? XBMC_PRESSED : XBMC_RELEASED;
  newEvent.button.type = newEvent.type;
  newEvent.button.x = x;
  newEvent.button.y = y;
  newEvent.button.button = XBMC_BUTTON_LEFT;
  CWinEventsAndroid::MessagePush(&newEvent);
}
