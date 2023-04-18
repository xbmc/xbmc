/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidMouse.h"

#include "ServiceBroker.h"
#include "XBMCApp.h"
#include "application/AppInboundProtocol.h"
#include "input/mouse/MouseStat.h"
#include "windowing/android/WinSystemAndroid.h"

//#define DEBUG_VERBOSE

bool CAndroidMouse::onMouseEvent(AInputEvent* event)
{
  if (event == NULL)
    return false;

  int32_t eventAction = AMotionEvent_getAction(event);
  int8_t mouseAction = eventAction & AMOTION_EVENT_ACTION_MASK;
  size_t mousePointerIdx = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

#ifdef DEBUG_VERBOSE
  int32_t mousePointerId = AMotionEvent_getPointerId(event, mousePointerIdx);
  CXBMCApp::android_printf("%s idx:%i, id:%i", __PRETTY_FUNCTION__, mousePointerIdx, mousePointerId);
#endif
  float x = AMotionEvent_getX(event, mousePointerIdx);
  float y = AMotionEvent_getY(event, mousePointerIdx);

  switch (mouseAction)
  {
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_DOWN:
      MouseButton(x,y,mouseAction,AMotionEvent_getButtonState(event));
      return true;
    case AMOTION_EVENT_ACTION_SCROLL:
      MouseWheel(x, y, AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_VSCROLL, mousePointerIdx));
      return true;
    default:
      MouseMove(x,y);
      return true;
  }
  return false;
}

void CAndroidMouse::MouseMove(float x, float y)
{
#ifdef DEBUG_VERBOSE
  CXBMCApp::android_printf("%s: x:%f, y:%f", __PRETTY_FUNCTION__, x, y);
#endif
  XBMC_Event newEvent = {};

  newEvent.type = XBMC_MOUSEMOTION;
  newEvent.motion.x = x;
  newEvent.motion.y = y;
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(newEvent);
}

void CAndroidMouse::MouseButton(float x, float y, int32_t action, int32_t buttons)
{
#ifdef DEBUG_VERBOSE
  CXBMCApp::android_printf("%s: x:%f, y:%f, action:%i, buttons:%i", __PRETTY_FUNCTION__, x, y, action, buttons);
#endif
  XBMC_Event newEvent = {};

  int32_t checkButtons = buttons;
  if (action ==  AMOTION_EVENT_ACTION_UP)
    checkButtons = m_lastButtonState;

  newEvent.type = (action ==  AMOTION_EVENT_ACTION_DOWN) ? XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  newEvent.button.x = x;
  newEvent.button.y = y;
  if (checkButtons & AMOTION_EVENT_BUTTON_PRIMARY)
    newEvent.button.button = XBMC_BUTTON_LEFT;
  else if (checkButtons & AMOTION_EVENT_BUTTON_SECONDARY)
    newEvent.button.button = XBMC_BUTTON_RIGHT;
  else if (checkButtons & AMOTION_EVENT_BUTTON_TERTIARY)
    newEvent.button.button = XBMC_BUTTON_MIDDLE;

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(newEvent);

  m_lastButtonState = buttons;
}

void CAndroidMouse::MouseWheel(float x, float y, float value)
{
#ifdef DEBUG_VERBOSE
  CXBMCApp::android_printf("%s: val:%f", __PRETTY_FUNCTION__, value);
#endif
  XBMC_Event newEvent = {};

  if (value > 0.0f)
  {
    newEvent.type = XBMC_MOUSEBUTTONDOWN;
    newEvent.button.button = XBMC_BUTTON_WHEELUP;
  }
  else if (value < 0.0f)
  {
    newEvent.type = XBMC_MOUSEBUTTONDOWN;
    newEvent.button.button = XBMC_BUTTON_WHEELDOWN;
  }
  else
    return;

  newEvent.button.x = x;
  newEvent.button.y = y;

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(newEvent);

  newEvent.type = XBMC_MOUSEBUTTONUP;

  dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem())->MessagePush(&newEvent);
}
