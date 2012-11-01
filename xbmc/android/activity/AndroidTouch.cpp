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

#include "AndroidTouch.h"
#include "XBMCApp.h"
#include "guilib/GUIWindowManager.h"
#include "windowing/WinEvents.h"
#include "ApplicationMessenger.h"

CAndroidTouch::CAndroidTouch() : m_dpi(160)
{
  CTouchInput::Get().RegisterHandler(this);
}

CAndroidTouch::~CAndroidTouch()
{
  CTouchInput::Get().UnregisterHandler();
}

bool CAndroidTouch::onTouchEvent(AInputEvent* event)
{
  CXBMCApp::android_printf("%s", __PRETTY_FUNCTION__);
  if (event == NULL)
    return false;

  size_t numPointers = AMotionEvent_getPointerCount(event);
  if (numPointers <= 0)
  {
    CXBMCApp::android_printf(" => aborting touch event because there are no active pointers");
    return false;
  }

  if (numPointers > TOUCH_MAX_POINTERS)
    numPointers = TOUCH_MAX_POINTERS;

  int32_t eventAction = AMotionEvent_getAction(event);
  int8_t touchAction = eventAction & AMOTION_EVENT_ACTION_MASK;
  size_t touchPointer = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
  
  CTouchInput::TouchEvent touchEvent = CTouchInput::TouchEventAbort;
  switch (touchAction)
  {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
      touchEvent = CTouchInput::TouchEventDown;
      break;

    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
      touchEvent = CTouchInput::TouchEventUp;
      break;

    case AMOTION_EVENT_ACTION_MOVE:
      touchEvent = CTouchInput::TouchEventMove;
      break;

    case AMOTION_EVENT_ACTION_OUTSIDE:
    case AMOTION_EVENT_ACTION_CANCEL:
    default:
      break;
  }

  float x = AMotionEvent_getX(event, touchPointer);
  float y = AMotionEvent_getY(event, touchPointer);
  float size = m_dpi / 16.0f;
  int64_t time = AMotionEvent_getEventTime(event);

  // first update all touch pointers
  for (unsigned int pointer = 0; pointer < numPointers; pointer++)
    CTouchInput::Get().Update(pointer, AMotionEvent_getX(event, pointer), AMotionEvent_getY(event, pointer),
    AMotionEvent_getEventTime(event), m_dpi / 16.0f);

  // now send the event
  return CTouchInput::Get().Handle(touchEvent, x, y, time, touchPointer, size);
}

bool CAndroidTouch::OnSingleTouchStart(float x, float y)
{
  // Send a mouse motion event for getting the current guiitem selected
  XBMC_Touch(XBMC_MOUSEMOTION, 0, (uint16_t)x, (uint16_t)y);

  return true;
}

bool CAndroidTouch::OnMultiTouchStart(float x, float y, int32_t pointers /* = 2 */)
{
  XBMC_TouchGesture(ACTION_GESTURE_BEGIN, x, y, 0.0f, 0.0f);

  return true;
}

bool CAndroidTouch::OnMultiTouchEnd(float x, float y, int32_t pointers /* = 2 */)
{
  XBMC_TouchGesture(ACTION_GESTURE_END, 0.0f, 0.0f, 0.0f, 0.0f);

  return true;
}

bool CAndroidTouch::OnTouchGesturePanStart(float x, float y)
{
  XBMC_TouchGesture(ACTION_GESTURE_BEGIN, x, y, 0.0f, 0.0f);

  return true;
}

bool CAndroidTouch::OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  XBMC_TouchGesture(ACTION_GESTURE_PAN, x, y, offsetX, offsetY);

  return true;
}

bool CAndroidTouch::OnTouchGesturePanEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  XBMC_TouchGesture(ACTION_GESTURE_END, velocityX, velocityY, x, y);

  // unfocus the focused GUI item
  g_windowManager.SendMessage(GUI_MSG_UNFOCUS_ALL, 0, 0, 0, 0);

  return true;
}

void CAndroidTouch::OnSingleTap(float x, float y)
{
  XBMC_Touch(XBMC_MOUSEBUTTONDOWN, XBMC_BUTTON_LEFT, (uint16_t)x, (uint16_t)y);
  XBMC_Touch(XBMC_MOUSEBUTTONUP, XBMC_BUTTON_LEFT, (uint16_t)x, (uint16_t)y);
}

void CAndroidTouch::OnSingleLongPress(float x, float y)
{
  // we send a right-click for this
  XBMC_Touch(XBMC_MOUSEBUTTONDOWN, XBMC_BUTTON_RIGHT, (uint16_t)x, (uint16_t)y);
  XBMC_Touch(XBMC_MOUSEBUTTONUP,   XBMC_BUTTON_RIGHT, (uint16_t)x, (uint16_t)y);
}

void CAndroidTouch::OnZoomPinch(float centerX, float centerY, float zoomFactor)
{
  XBMC_TouchGesture(ACTION_GESTURE_ZOOM, centerX, centerY, zoomFactor, 0);
}

void CAndroidTouch::OnRotate(float centerX, float centerY, float angle)
{
  XBMC_TouchGesture(ACTION_GESTURE_ROTATE, centerX, centerY, angle, 0);
}

void CAndroidTouch::setDPI(uint32_t dpi)
{
  if (dpi != 0)
    m_dpi = dpi;
}

void CAndroidTouch::XBMC_Touch(uint8_t type, uint8_t button, uint16_t x, uint16_t y)
{
  XBMC_Event newEvent;
  memset(&newEvent, 0, sizeof(newEvent));
  
  newEvent.type = type;
  newEvent.button.type = type;
  newEvent.button.button = button;
  newEvent.button.x = x;
  newEvent.button.y = y;
  
  CXBMCApp::android_printf("XBMC_Touch(%u, %u, %u, %u)", type, button, x, y);
  CWinEvents::MessagePush(&newEvent);
}

void CAndroidTouch::XBMC_TouchGesture(int32_t action, float posX, float posY, float offsetX, float offsetY)
{
  CXBMCApp::android_printf("XBMC_TouchGesture(%d, %f, %f, %f, %f)", action, posX, posY, offsetX, offsetY);
  if (action == ACTION_GESTURE_BEGIN)
    CApplicationMessenger::Get().SendAction(CAction(action, 0, posX, posY, 0, 0), WINDOW_INVALID, false);
  else if (action == ACTION_GESTURE_PAN)
    CApplicationMessenger::Get().SendAction(CAction(action, 0, posX, posY, offsetX, offsetY), WINDOW_INVALID, false);
  else if (action == ACTION_GESTURE_END)
    CApplicationMessenger::Get().SendAction(CAction(action, 0, posX, posY, offsetX, offsetY), WINDOW_INVALID, false);
  else if (action == ACTION_GESTURE_ZOOM || action == ACTION_GESTURE_ROTATE)
    CApplicationMessenger::Get().SendAction(CAction(action, 0, posX, posY, offsetX, 0), WINDOW_INVALID, false);
}
