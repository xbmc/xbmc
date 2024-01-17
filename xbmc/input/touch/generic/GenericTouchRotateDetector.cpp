/*
 *  Copyright (C) 2013-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GenericTouchRotateDetector.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028842
#endif

CGenericTouchRotateDetector::CGenericTouchRotateDetector(ITouchActionHandler* handler, float dpi)
  : IGenericTouchGestureDetector(handler, dpi)
{
}

bool CGenericTouchRotateDetector::OnTouchDown(unsigned int index, const Pointer& pointer)
{
  if (index >= MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  m_pointers[index] = pointer;
  m_angle = 0.0f;
  return true;
}

bool CGenericTouchRotateDetector::OnTouchUp(unsigned int index, const Pointer& pointer)
{
  if (index >= MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  // after lifting the primary pointer, the secondary pointer will
  // become the primary pointer in the next event
  if (index == 0)
  {
    m_pointers[0] = m_pointers[1];
    index = 1;
  }

  m_pointers[index].reset();

  if (!m_pointers[0].valid() && !m_pointers[1].valid())
    m_done = true;

  return true;
}

bool CGenericTouchRotateDetector::OnTouchMove(unsigned int index, const Pointer& pointer)
{
  if (index >= MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  // update the internal pointers
  m_pointers[index] = pointer;

  Pointer& primaryPointer = m_pointers[0];
  Pointer& secondaryPointer = m_pointers[1];

  if (!primaryPointer.valid() || !secondaryPointer.valid() ||
      (!primaryPointer.moving && !secondaryPointer.moving))
    return false;

  CVector last = primaryPointer.last - secondaryPointer.last;
  CVector current = primaryPointer.current - secondaryPointer.current;

  float length = last.length() * current.length();
  if (length != 0.0f)
  {
    float centerX = (primaryPointer.current.x + secondaryPointer.current.x) / 2;
    float centerY = (primaryPointer.current.y + secondaryPointer.current.y) / 2;
    float scalar = last.scalar(current);
    float angle = acos(scalar / length) * 180.0f / static_cast<float>(M_PI);

    // make sure the result of acos is a valid number
    if (angle == angle)
    {
      // calculate the direction of the rotation using the
      // z-component of the cross-product of last and current
      float direction = last.x * current.y - current.x * last.y;
      if (direction < 0.0f)
        m_angle -= angle;
      else
        m_angle += angle;

      OnRotate(centerX, centerY, m_angle);
    }
  }

  return true;
}

bool CGenericTouchRotateDetector::OnTouchUpdate(unsigned int index, const Pointer& pointer)
{
  if (index >= MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  // update the internal pointers
  m_pointers[index] = pointer;
  return true;
}
