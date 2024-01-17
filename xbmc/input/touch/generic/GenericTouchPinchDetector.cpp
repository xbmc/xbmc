/*
 *  Copyright (C) 2013-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GenericTouchPinchDetector.h"

bool CGenericTouchPinchDetector::OnTouchDown(unsigned int index, const Pointer& pointer)
{
  if (index >= MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  m_pointers[index] = pointer;
  return true;
}

bool CGenericTouchPinchDetector::OnTouchUp(unsigned int index, const Pointer& pointer)
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

bool CGenericTouchPinchDetector::OnTouchMove(unsigned int index, const Pointer& pointer)
{
  if (index >= MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  // update the internal pointers
  m_pointers[index] = pointer;

  const Pointer& primaryPointer = m_pointers[0];
  const Pointer& secondaryPointer = m_pointers[1];

  if (!primaryPointer.valid() || !secondaryPointer.valid() ||
      (!primaryPointer.moving && !secondaryPointer.moving))
    return false;

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

  return true;
}
