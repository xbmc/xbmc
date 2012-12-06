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

#include <math.h>

#include "TouchInput.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028842
#endif

CTouchInput::CTouchInput()
     : m_holdTimeout(1000),
       m_handler(NULL),
       m_fRotateAngle(0.0f),
       m_gestureState(TouchGestureUnknown),
       m_gestureStateOld(TouchGestureUnknown)
{
  m_holdTimer = new CTimer(this);
}

CTouchInput::~CTouchInput()
{
  delete m_holdTimer;
}

CTouchInput &CTouchInput::Get()
{
  static CTouchInput sTouchInput;
  return sTouchInput;
}

void CTouchInput::RegisterHandler(ITouchHandler *touchHandler)
{
  m_handler = touchHandler;
}

void CTouchInput::UnregisterHandler()
{
  m_handler = NULL;
}

void CTouchInput::SetTouchHoldTimeout(int32_t timeout)
{
  if (timeout <= 0)
    return;

  m_holdTimeout = timeout;
}

bool CTouchInput::Handle(TouchEvent event, float x, float y, int64_t time, int32_t pointer /* = 0 */, float size /* = 0.0f */)
{
  if (time < 0 || pointer < 0 || pointer >= TOUCH_MAX_POINTERS)
    return false;

  CSingleLock lock(m_critical);

  bool result = true;

  m_pointers[pointer].current.x = x;
  m_pointers[pointer].current.y = y;
  m_pointers[pointer].current.time = time;

  switch (event)
  {
    case TouchEventAbort:
    {
      CLog::Log(LOGDEBUG, "CTouchInput: TouchEventAbort");
      setGestureState(TouchGestureUnknown);
      for (unsigned int pIndex = 0; pIndex < TOUCH_MAX_POINTERS; pIndex++)
        m_pointers[pIndex].reset();

      OnTouchAbort();
      break;
    }

    case TouchEventDown:
    {
      CLog::Log(LOGDEBUG, "CTouchInput: TouchEventDown");
      m_pointers[pointer].down.x = x;
      m_pointers[pointer].down.y = y;
      m_pointers[pointer].down.time = time;
      m_pointers[pointer].moving = false;
      m_pointers[pointer].size = size;

      // If this is the down event of the primary pointer
      // we start by assuming that it's a single touch
      if (pointer == 0)
      {
        setGestureState(TouchGestureSingleTouch);
        result = OnSingleTouchStart(x, y);

        m_holdTimer->Start(m_holdTimeout);
      }
      // Otherwise it's the down event of another pointer
      else
      {
        // If we so far assumed single touch or still have the primary
        // pointer of a previous multi touch pressed down, we can update to multi touch
        if (m_gestureState == TouchGestureSingleTouch || m_gestureState == TouchGestureSingleTouchHold ||
            m_gestureState == TouchGestureMultiTouchDone)
        {
          if (m_gestureState == TouchGestureSingleTouch || m_gestureState == TouchGestureSingleTouchHold)
          {
            result = OnMultiTouchStart(x, y);

            m_holdTimer->Stop(true);
            m_holdTimer->Start(m_holdTimeout);
          }
          else
          {
            result = OnMultiTouchDown(x, y, pointer);

            m_holdTimer->Stop(false);
          }

          setGestureState(TouchGestureMultiTouchStart);
          m_fRotateAngle = 0.0f;
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

    case TouchEventUp:
    {
      CLog::Log(LOGDEBUG, "CTouchInput: TouchEventUp");
      // unexpected event => abort
      if (!m_pointers[pointer].valid() ||
          m_gestureState == TouchGestureUnknown)
        break;

      m_holdTimer->Stop(false);

      // Just a single tap with a pointer
      if (m_gestureState == TouchGestureSingleTouch || m_gestureState == TouchGestureSingleTouchHold)
      {
        result = OnSingleTouchEnd(x, y);

        if (m_gestureState == TouchGestureSingleTouch)
          OnSingleTap(x, y);
      }
      // A pan gesture started with a single pointer (ignoring any other pointers)
      else if (m_gestureState == TouchGesturePan)
      {
        float velocityX = 0.0f; // number of pixels per second
        float velocityY = 0.0f; // number of pixels per second
        m_pointers[pointer].velocity(velocityX, velocityY, false);

        result = OnTouchGesturePanEnd(x, y,
                                      x - m_pointers[pointer].down.x, y - m_pointers[pointer].down.y,
                                      velocityX, velocityY);
      }
      // we are in multi-touch
      else
        result = OnMultiTouchUp(x, y, pointer);

      // If we were in multi touch mode and lifted one pointer
      // we can go into the TouchGestureMultiTouchDone state which will allow
      // the user to go back into multi touch mode without lifting the primary pointer
      if (m_gestureState == TouchGestureMultiTouchStart || m_gestureState == TouchGestureMultiTouchHold || m_gestureState == TouchGestureMultiTouch)
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
          result = OnMultiTouchEnd(m_pointers[0].down.x, m_pointers[0].down.y);

          // if neither of the two pointers moved we have a double tap
          if (m_gestureStateOld != TouchGestureMultiTouchHold && m_gestureStateOld != TouchGestureMultiTouch)
            OnDoubleTap(m_pointers[0].down.x, m_pointers[0].down.y,
                        m_pointers[1].down.x, m_pointers[1].down.y);
        }

        setGestureState(TouchGestureUnknown);
      }

      m_pointers[pointer].reset();
      return result;
    }

    case TouchEventMove:
    {
      CLog::Log(LOGDEBUG, "CTouchInput: TouchEventMove");
      // unexpected event => abort
      if (!m_pointers[pointer].valid() ||
          m_gestureState == TouchGestureUnknown ||
          m_gestureState == TouchGestureMultiTouchDone)
        break;

      if (m_pointers[pointer].moving)
        m_holdTimer->Stop();

      if (m_gestureState == TouchGestureSingleTouch)
      {
        // Check if the touch has moved far enough to count as movement
        if (!m_pointers[pointer].moving)
          break;

        result = OnTouchGesturePanStart(m_pointers[pointer].down.x, m_pointers[pointer].down.y);

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
         (m_gestureState == TouchGestureSingleTouch || m_gestureState == TouchGestureSingleTouchHold || m_gestureState == TouchGesturePan))
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
        if (m_pointers[pointer].moving)
          result = OnMultiTouchMove(x, y,offsetX, offsetY, velocityX, velocityY, pointer);

        handleMultiTouchGesture();
      }
      else
        break;

      return result;
    }

    default:
      CLog::Log(LOGDEBUG, "CTouchInput: unknown TouchEvent");
      break;
  }

  return false;
}

bool CTouchInput::Update(int32_t pointer, float x, float y, int64_t time, float size /* = 0.0f */)
{
  if (pointer < 0 || pointer >= TOUCH_MAX_POINTERS)
    return false;

  CSingleLock lock(m_critical);

  m_pointers[pointer].last.copy(m_pointers[pointer].current);

  m_pointers[pointer].current.x = x;
  m_pointers[pointer].current.y = y;
  m_pointers[pointer].current.time = time;
  if (size > 0.0f)
    m_pointers[pointer].size = size;

  // calculate whether the pointer has moved at all
  if (!m_pointers[pointer].moving)
  {
    CVector down = m_pointers[pointer].down;
    CVector current = m_pointers[pointer].current;
    CVector distance = down - current;

    if (distance.length() > m_pointers[pointer].size)
      m_pointers[pointer].moving = true;
  }

  return true;
}

void CTouchInput::saveLastTouch()
{
  for (unsigned int pointer = 0; pointer < TOUCH_MAX_POINTERS; pointer++)
    m_pointers[pointer].last.copy(m_pointers[pointer].current);
}

void CTouchInput::handleMultiTouchGesture()
{
  handleZoomPinch();
  handleRotation();
}

void CTouchInput::handleZoomPinch()
{
  Pointer& primaryPointer = m_pointers[0];
  Pointer& secondaryPointer = m_pointers[1];

  // calculate zoom/pinch
  CVector primary = primaryPointer.down;
  CVector secondary = secondaryPointer.down;
  CVector diagonal = primary - secondary;

  float baseDiffLength = diagonal.length();
  if (baseDiffLength != 0.0f)
  {
    CVector primaryNow = primaryPointer.current;
    CVector secondaryNow = secondaryPointer.current;
    CVector diagonalNow = primaryNow - secondaryNow;
    float curDiffLength = diagonalNow.length();

    float centerX = (primary.x + secondary.x) / 2;
    float centerY = (primary.y + secondary.y) / 2;

    float zoom = curDiffLength / baseDiffLength;

    OnZoomPinch(centerX, centerY, zoom);
  }
}

void CTouchInput::handleRotation()
{
  Pointer& primaryPointer = m_pointers[0];
  Pointer& secondaryPointer = m_pointers[1];

  CVector last = primaryPointer.last - secondaryPointer.last;
  CVector current = primaryPointer.current - secondaryPointer.current;

  float length = last.length() * current.length();
  if (length != 0.0f)
  {
    float centerX = (primaryPointer.current.x + secondaryPointer.current.x) / 2;
    float centerY = (primaryPointer.current.y + secondaryPointer.current.y) / 2;

    float scalar = last.scalar(current);
    float angle = acos(scalar / length) * 180.0f / M_PI;

    // make sure the result of acos is a valid number
    if (angle == angle)
    {
      // calculate the direction of the rotation using the
      // z-component of the cross-product of last and current
      float direction = last.x * current.y - current.x * last.y;
      if (direction < 0.0f)
        m_fRotateAngle -= angle;
      else
        m_fRotateAngle += angle;

      OnRotate(centerX, centerY, m_fRotateAngle);
    }
  }
}

void CTouchInput::OnTimeout()
{
  CSingleLock lock(m_critical);

  switch (m_gestureState)
  {
    case TouchGestureSingleTouch:
      setGestureState(TouchGestureSingleTouchHold);

      OnSingleTouchHold(m_pointers[0].down.x, m_pointers[0].down.y);
      OnSingleLongPress(m_pointers[0].down.x, m_pointers[0].down.y);
      break;

    case TouchGestureMultiTouchStart:
      if (!m_pointers[0].moving && !m_pointers[1].moving)
      {
        setGestureState(TouchGestureMultiTouchHold);

        OnMultiTouchHold(m_pointers[0].down.x, m_pointers[0].down.y);
        OnDoubleLongPress(m_pointers[0].down.x, m_pointers[0].down.y, m_pointers[1].down.x, m_pointers[1].down.y);
      }
      break;

    default:
      break;
  }
}

void CTouchInput::OnTouchAbort()
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    m_handler->OnTouchAbort();
}

bool CTouchInput::OnSingleTouchStart(float x, float y)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnSingleTouchStart(x, y);

  return true;
}

bool CTouchInput::OnSingleTouchHold(float x, float y)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnSingleTouchHold(x, y);

  return true;
}

bool CTouchInput::OnSingleTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnSingleTouchMove(x, y, offsetX, offsetY, velocityX, velocityY);

  return true;
}

bool CTouchInput::OnSingleTouchEnd(float x, float y)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnSingleTouchEnd(x, y);

  return true;
}

bool CTouchInput::OnMultiTouchStart(float x, float y, int32_t pointers /* = 2 */)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnMultiTouchStart(x, y, pointers);

  return true;
}

bool CTouchInput::OnMultiTouchDown(float x, float y, int32_t pointer)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnMultiTouchDown(x, y, pointer);

  return true;
}

bool CTouchInput::OnMultiTouchHold(float x, float y, int32_t pointers /* = 2 */)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnMultiTouchHold(x, y, pointers);

  return true;
}

bool CTouchInput::OnMultiTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY, int32_t pointer)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnMultiTouchMove(x, y, offsetX, offsetY, velocityX, velocityY, pointer);

  return true;
}

bool CTouchInput::OnMultiTouchUp(float x, float y, int32_t pointer)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnMultiTouchUp(x, y, pointer);

  return true;
}

bool CTouchInput::OnMultiTouchEnd(float x, float y, int32_t pointers /* = 2 */)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnMultiTouchEnd(x, y, pointers);

  return true;
}

bool CTouchInput::OnTouchGesturePanStart(float x, float y)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnTouchGesturePanStart(x, y);

  return true;
}

bool CTouchInput::OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnTouchGesturePan(x, y, offsetX, offsetY, velocityX, velocityY);

  return true;
}

bool CTouchInput::OnTouchGesturePanEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    return m_handler->OnTouchGesturePanEnd(x, y, offsetX, offsetY, velocityX, velocityY);

  return true;
}

void CTouchInput::OnSingleTap(float x, float y)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    m_handler->OnSingleTap(x, y);
}

void CTouchInput::OnSingleLongPress(float x, float y)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    m_handler->OnSingleLongPress(x, y);
}

void CTouchInput::OnDoubleTap(float x1, float y1, float x2, float y2)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    m_handler->OnDoubleTap(x1, y1, x2, y2);
}

void CTouchInput::OnDoubleLongPress(float x1, float y1, float x2, float y2)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);
  
  if (m_handler)
    m_handler->OnDoubleLongPress(x1, y1, x2, y2);
}

void CTouchInput::OnZoomPinch(float centerX, float centerY, float zoomFactor)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);
  
  if (m_handler)
    m_handler->OnZoomPinch(centerX, centerY, zoomFactor);
}

void CTouchInput::OnRotate(float centerX, float centerY, float angle)
{
  CLog::Log(LOGDEBUG, "%s", __FUNCTION__);

  if (m_handler)
    m_handler->OnRotate(centerX, centerY, angle);
}
