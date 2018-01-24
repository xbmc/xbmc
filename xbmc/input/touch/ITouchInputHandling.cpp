/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#include "ITouchInputHandling.h"

void ITouchInputHandling::RegisterHandler(ITouchActionHandler *touchHandler)
{
  m_handler = touchHandler;
}

void ITouchInputHandling::UnregisterHandler()
{
  m_handler = NULL;
}

void ITouchInputHandling::OnTouchAbort()
{
  if (m_handler)
    m_handler->OnTouchAbort();
}

bool ITouchInputHandling::OnSingleTouchStart(float x, float y)
{
  if (m_handler)
    return m_handler->OnSingleTouchStart(x, y);

  return true;
}

bool ITouchInputHandling::OnSingleTouchHold(float x, float y)
{
  if (m_handler)
    return m_handler->OnSingleTouchHold(x, y);

  return true;
}

bool ITouchInputHandling::OnSingleTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  if (m_handler)
    return m_handler->OnSingleTouchMove(x, y, offsetX, offsetY, velocityX, velocityY);

  return true;
}

bool ITouchInputHandling::OnSingleTouchEnd(float x, float y)
{
  if (m_handler)
    return m_handler->OnSingleTouchEnd(x, y);

  return true;
}

bool ITouchInputHandling::OnMultiTouchDown(float x, float y, int32_t pointer)
{
  if (m_handler)
    return m_handler->OnMultiTouchDown(x, y, pointer);

  return true;
}

bool ITouchInputHandling::OnMultiTouchHold(float x, float y, int32_t pointers /* = 2 */)
{
  if (m_handler)
    return m_handler->OnMultiTouchHold(x, y, pointers);

  return true;
}

bool ITouchInputHandling::OnMultiTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY, int32_t pointer)
{
  if (m_handler)
    return m_handler->OnMultiTouchMove(x, y, offsetX, offsetY, velocityX, velocityY, pointer);

  return true;
}

bool ITouchInputHandling::OnMultiTouchUp(float x, float y, int32_t pointer)
{
  if (m_handler)
    return m_handler->OnMultiTouchUp(x, y, pointer);

  return true;
}

bool ITouchInputHandling::OnTouchGestureStart(float x, float y)
{
  if (m_handler)
    return m_handler->OnTouchGestureStart(x, y);

  return true;
}

bool ITouchInputHandling::OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  if (m_handler)
    return m_handler->OnTouchGesturePan(x, y, offsetX, offsetY, velocityX, velocityY);

  return true;
}

bool ITouchInputHandling::OnTouchGestureEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
{
  if (m_handler)
    return m_handler->OnTouchGestureEnd(x, y, offsetX, offsetY, velocityX, velocityY);

  return true;
}

void ITouchInputHandling::OnTap(float x, float y, int32_t pointers /* = 1 */)
{
  if (m_handler)
    m_handler->OnTap(x, y, pointers);
}

void ITouchInputHandling::OnLongPress(float x, float y, int32_t pointers /* = 1 */)
{
  if (m_handler)
    m_handler->OnLongPress(x, y, pointers);
}

void ITouchInputHandling::OnSwipe(TouchMoveDirection direction, float xDown, float yDown, float xUp, float yUp, float velocityX, float velocityY, int32_t pointers /* = 1 */)
{
  if (m_handler)
    m_handler->OnSwipe(direction, xDown, yDown, xUp, yUp, velocityX, velocityY, pointers);
}

void ITouchInputHandling::OnZoomPinch(float centerX, float centerY, float zoomFactor)
{
  if (m_handler)
    m_handler->OnZoomPinch(centerX, centerY, zoomFactor);
}

void ITouchInputHandling::OnRotate(float centerX, float centerY, float angle)
{
  if (m_handler)
    m_handler->OnRotate(centerX, centerY, angle);
}
