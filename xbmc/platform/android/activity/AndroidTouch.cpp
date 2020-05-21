/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidTouch.h"

#include "input/touch/generic/GenericTouchActionHandler.h"
#include "input/touch/generic/GenericTouchInputHandler.h"

#include "platform/android/activity/XBMCApp.h"

CAndroidTouch::CAndroidTouch()
{
  CGenericTouchInputHandler::GetInstance().RegisterHandler(&CGenericTouchActionHandler::GetInstance());
}

CAndroidTouch::~CAndroidTouch()
{
  CGenericTouchInputHandler::GetInstance().UnregisterHandler();
}

bool CAndroidTouch::onTouchEvent(AInputEvent* event)
{
  if (event == NULL)
    return false;

  size_t numPointers = AMotionEvent_getPointerCount(event);
  if (numPointers <= 0)
  {
    CXBMCApp::android_printf(" => aborting touch event because there are no active pointers");
    return false;
  }

  if (numPointers > CGenericTouchInputHandler::MAX_POINTERS)
    numPointers = CGenericTouchInputHandler::MAX_POINTERS;

  int32_t eventAction = AMotionEvent_getAction(event);
  int8_t touchAction = eventAction & AMOTION_EVENT_ACTION_MASK;
  size_t touchPointer = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

  TouchInput touchEvent = TouchInputAbort;
  switch (touchAction)
  {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
      touchEvent = TouchInputDown;
      break;

    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
      touchEvent = TouchInputUp;
      break;

    case AMOTION_EVENT_ACTION_MOVE:
      touchEvent = TouchInputMove;
      break;

    case AMOTION_EVENT_ACTION_OUTSIDE:
    case AMOTION_EVENT_ACTION_CANCEL:
    default:
      break;
  }

  float x = AMotionEvent_getX(event, touchPointer);
  float y = AMotionEvent_getY(event, touchPointer);
  int64_t time = AMotionEvent_getEventTime(event);

  // first update all touch pointers
  for (unsigned int pointer = 0; pointer < numPointers; pointer++)
    CGenericTouchInputHandler::GetInstance().UpdateTouchPointer(pointer, AMotionEvent_getX(event, pointer), AMotionEvent_getY(event, pointer),
    AMotionEvent_getEventTime(event));

  // let system know that we are starting a guesture
  if (touchEvent == TouchInputDown)
    CGenericTouchActionHandler::GetInstance().QuerySupportedGestures(x, y);

  // now send the event
  return CGenericTouchInputHandler::GetInstance().HandleTouchInput(touchEvent, x, y, time, touchPointer);
}

void CAndroidTouch::setDPI(uint32_t dpi)
{
  if (dpi != 0)
  {
    m_dpi = dpi;

    CGenericTouchInputHandler::GetInstance().SetScreenDPI(m_dpi);
  }
}
