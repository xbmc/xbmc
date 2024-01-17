/*
 *  Copyright (C) 2012-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GenericTouchInputHandler.h"

#include "input/touch/generic/GenericTouchPinchDetector.h"
#include "input/touch/generic/GenericTouchRotateDetector.h"
#include "input/touch/generic/GenericTouchSwipeDetector.h"
#include "utils/log.h"

#include <algorithm>
#include <cmath>
#include <mutex>

using namespace std::chrono_literals;

namespace
{
constexpr auto TOUCH_HOLD_TIMEOUT = 500ms;
}

CGenericTouchInputHandler::CGenericTouchInputHandler() : m_holdTimer(new CTimer(this))
{
}

CGenericTouchInputHandler::~CGenericTouchInputHandler() = default;

CGenericTouchInputHandler& CGenericTouchInputHandler::GetInstance()
{
  static CGenericTouchInputHandler sTouchInput;
  return sTouchInput;
}

float CGenericTouchInputHandler::AdjustPointerSize(float size)
{
  if (size > 0.0f)
    return size;
  else
    // Set a default size if touch input layer does not have anything useful,
    // approx. 3.2mm
    return m_dpi / 8.0f;
}

bool CGenericTouchInputHandler::HandleTouchInput(TouchInput event,
                                                 float x,
                                                 float y,
                                                 int64_t time,
                                                 int32_t pointer /* = 0 */,
                                                 float size /* = 0.0f */)
{
  if (time < 0 || pointer < 0 || pointer >= MAX_POINTERS)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critical);

  bool result = true;

  m_pointers[pointer].current.x = x;
  m_pointers[pointer].current.y = y;
  m_pointers[pointer].current.time = time;

  switch (event)
  {
    case TouchInputAbort:
    {
      triggerDetectors(event, pointer);

      setGestureState(TouchGestureUnknown);
      for (auto& pointer : m_pointers)
        pointer.reset();

      OnTouchAbort();
      break;
    }

    case TouchInputDown:
    {
      m_pointers[pointer].down.x = x;
      m_pointers[pointer].down.y = y;
      m_pointers[pointer].down.time = time;
      m_pointers[pointer].moving = false;
      m_pointers[pointer].size = AdjustPointerSize(size);

      // If this is the down event of the primary pointer
      // we start by assuming that it's a single touch
      if (pointer == 0)
      {
        // create new gesture detectors
        m_detectors.emplace(new CGenericTouchSwipeDetector(this, m_dpi));
        m_detectors.emplace(new CGenericTouchPinchDetector(this, m_dpi));
        m_detectors.emplace(new CGenericTouchRotateDetector(this, m_dpi));
        triggerDetectors(event, pointer);

        setGestureState(TouchGestureSingleTouch);
        result = OnSingleTouchStart(x, y);

        m_holdTimer->Start(TOUCH_HOLD_TIMEOUT);
      }
      // Otherwise it's the down event of another pointer
      else
      {
        triggerDetectors(event, pointer);

        // If we so far assumed single touch or still have the primary
        // pointer of a previous multi touch pressed down, we can update to multi touch
        if (m_gestureState == TouchGestureSingleTouch ||
            m_gestureState == TouchGestureSingleTouchHold ||
            m_gestureState == TouchGestureMultiTouchDone)
        {
          result = OnMultiTouchDown(x, y, pointer);
          m_holdTimer->Stop(true);

          if (m_gestureState == TouchGestureSingleTouch ||
              m_gestureState == TouchGestureSingleTouchHold)
            m_holdTimer->Start(TOUCH_HOLD_TIMEOUT);

          setGestureState(TouchGestureMultiTouchStart);
        }
        // Otherwise we should ignore this pointer
        else
        {
          m_pointers[pointer].reset();
          break;
        }
      }
      return result;
    }

    case TouchInputUp:
    {
      // unexpected event => abort
      if (!m_pointers[pointer].valid() || m_gestureState == TouchGestureUnknown)
        break;

      triggerDetectors(event, pointer);

      m_holdTimer->Stop(false);

      // Just a single tap with a pointer
      if (m_gestureState == TouchGestureSingleTouch ||
          m_gestureState == TouchGestureSingleTouchHold)
      {
        result = OnSingleTouchEnd(x, y);

        if (m_gestureState == TouchGestureSingleTouch)
          OnTap(x, y, 1);
      }
      // A pan gesture started with a single pointer (ignoring any other pointers)
      else if (m_gestureState == TouchGesturePan)
      {
        float velocityX = 0.0f; // number of pixels per second
        float velocityY = 0.0f; // number of pixels per second
        m_pointers[pointer].velocity(velocityX, velocityY, false);

        result = OnTouchGestureEnd(x, y, x - m_pointers[pointer].down.x,
                                   y - m_pointers[pointer].down.y, velocityX, velocityY);
      }
      // we are in multi-touch
      else
        result = OnMultiTouchUp(x, y, pointer);

      // If we were in multi touch mode and lifted one pointer
      // we can go into the TouchGestureMultiTouchDone state which will allow
      // the user to go back into multi touch mode without lifting the primary pointer
      if (m_gestureState == TouchGestureMultiTouchStart ||
          m_gestureState == TouchGestureMultiTouchHold || m_gestureState == TouchGestureMultiTouch)
      {
        setGestureState(TouchGestureMultiTouchDone);

        // after lifting the primary pointer, the secondary pointer will
        // become the primary pointer in the next event
        if (pointer == 0)
        {
          m_pointers[0] = m_pointers[1];
          pointer = 1;
        }
      }
      // Otherwise abort
      else
      {
        if (m_gestureState == TouchGestureMultiTouchDone)
        {
          float velocityX = 0.0f; // number of pixels per second
          float velocityY = 0.0f; // number of pixels per second
          m_pointers[pointer].velocity(velocityX, velocityY, false);

          result = OnTouchGestureEnd(x, y, x - m_pointers[pointer].down.x,
                                     y - m_pointers[pointer].down.y, velocityX, velocityY);

          // if neither of the two pointers moved we have a single tap with multiple pointers
          if (m_gestureStateOld != TouchGestureMultiTouchHold &&
              m_gestureStateOld != TouchGestureMultiTouch)
            OnTap(std::abs((m_pointers[0].down.x + m_pointers[1].down.x) / 2),
                  std::abs((m_pointers[0].down.y + m_pointers[1].down.y) / 2), 2);
        }

        setGestureState(TouchGestureUnknown);
      }
      m_pointers[pointer].reset();

      return result;
    }

    case TouchInputMove:
    {
      // unexpected event => abort
      if (!m_pointers[pointer].valid() || m_gestureState == TouchGestureUnknown ||
          m_gestureState == TouchGestureMultiTouchDone)
        break;

      bool moving = std::any_of(m_pointers.cbegin(), m_pointers.cend(),
                                [](Pointer const& p) { return p.valid() && p.moving; });

      if (moving)
      {
        m_holdTimer->Stop();

        // the touch is moving so we start a gesture
        if (m_gestureState == TouchGestureSingleTouch ||
            m_gestureState == TouchGestureMultiTouchStart)
          result = OnTouchGestureStart(m_pointers[pointer].down.x, m_pointers[pointer].down.y);
      }

      triggerDetectors(event, pointer);

      // Check if the touch has moved far enough to count as movement
      if ((m_gestureState == TouchGestureSingleTouch ||
           m_gestureState == TouchGestureMultiTouchStart) &&
          !m_pointers[pointer].moving)
        break;

      if (m_gestureState == TouchGestureSingleTouch)
      {
        m_pointers[pointer].last.copy(m_pointers[pointer].down);
        setGestureState(TouchGesturePan);
      }
      else if (m_gestureState == TouchGestureMultiTouchStart)
      {
        setGestureState(TouchGestureMultiTouch);

        // set the starting point
        saveLastTouch();
      }

      float offsetX = x - m_pointers[pointer].last.x;
      float offsetY = y - m_pointers[pointer].last.y;
      float velocityX = 0.0f; // number of pixels per second
      float velocityY = 0.0f; // number of pixels per second
      m_pointers[pointer].velocity(velocityX, velocityY);

      if (m_pointers[pointer].moving &&
          (m_gestureState == TouchGestureSingleTouch ||
           m_gestureState == TouchGestureSingleTouchHold || m_gestureState == TouchGesturePan))
        result = OnSingleTouchMove(x, y, offsetX, offsetY, velocityX, velocityY);

      // Let's see if we have a pan gesture (i.e. the primary and only pointer moving)
      if (m_gestureState == TouchGesturePan)
      {
        result = OnTouchGesturePan(x, y, offsetX, offsetY, velocityX, velocityY);

        m_pointers[pointer].last.x = x;
        m_pointers[pointer].last.y = y;
      }
      else if (m_gestureState == TouchGestureMultiTouch)
      {
        if (moving)
          result = OnMultiTouchMove(x, y, offsetX, offsetY, velocityX, velocityY, pointer);
      }
      else
        break;

      return result;
    }

    default:
      CLog::Log(LOGDEBUG, "CGenericTouchInputHandler: unknown TouchInput");
      break;
  }

  return false;
}

bool CGenericTouchInputHandler::UpdateTouchPointer(
    int32_t pointer, float x, float y, int64_t time, float size /* = 0.0f */)
{
  if (pointer < 0 || pointer >= MAX_POINTERS)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critical);

  m_pointers[pointer].last.copy(m_pointers[pointer].current);

  m_pointers[pointer].current.x = x;
  m_pointers[pointer].current.y = y;
  m_pointers[pointer].current.time = time;
  m_pointers[pointer].size = AdjustPointerSize(size);

  // calculate whether the pointer has moved at all
  if (!m_pointers[pointer].moving)
  {
    CVector down = m_pointers[pointer].down;
    CVector current = m_pointers[pointer].current;
    CVector distance = down - current;

    if (distance.length() > m_pointers[pointer].size)
      m_pointers[pointer].moving = true;
  }

  for (auto const& detector : m_detectors)
    detector->OnTouchUpdate(pointer, m_pointers[pointer]);

  return true;
}

void CGenericTouchInputHandler::saveLastTouch()
{
  for (auto& pointer : m_pointers)
    pointer.last.copy(pointer.current);
}

void CGenericTouchInputHandler::OnTimeout()
{
  std::unique_lock<CCriticalSection> lock(m_critical);

  switch (m_gestureState)
  {
    case TouchGestureSingleTouch:
      setGestureState(TouchGestureSingleTouchHold);

      OnSingleTouchHold(m_pointers[0].down.x, m_pointers[0].down.y);
      OnLongPress(m_pointers[0].down.x, m_pointers[0].down.y, 1);
      break;

    case TouchGestureMultiTouchStart:
      if (!m_pointers[0].moving && !m_pointers[1].moving)
      {
        setGestureState(TouchGestureMultiTouchHold);

        OnMultiTouchHold(m_pointers[0].down.x, m_pointers[0].down.y);
        OnLongPress(std::abs((m_pointers[0].down.x + m_pointers[1].down.x) / 2),
                    std::abs((m_pointers[0].down.y + m_pointers[1].down.y) / 2), 2);
      }
      break;

    default:
      break;
  }
}

void CGenericTouchInputHandler::triggerDetectors(TouchInput event, int32_t pointer)
{
  switch (event)
  {
    case TouchInputAbort:
    {
      m_detectors.clear();
      break;
    }

    case TouchInputDown:
    {
      for (auto const& detector : m_detectors)
        detector->OnTouchDown(pointer, m_pointers[pointer]);
      break;
    }

    case TouchInputUp:
    {
      for (auto const& detector : m_detectors)
        detector->OnTouchUp(pointer, m_pointers[pointer]);
      break;
    }

    case TouchInputMove:
    {
      for (auto const& detector : m_detectors)
        detector->OnTouchMove(pointer, m_pointers[pointer]);
      break;
    }

    default:
      return;
  }

  for (auto it = m_detectors.begin(); it != m_detectors.end();)
  {
    if ((*it)->IsDone())
      it = m_detectors.erase(it);
    else
      it++;
  }
}
