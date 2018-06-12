/*
 *      Copyright (C) 2013 Team XBMC
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

#include "GenericTouchPinchDetector.h"

bool CGenericTouchPinchDetector::OnTouchDown(unsigned int index, const Pointer &pointer)
{
  if (index >= MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  m_pointers[index] = pointer;
  return true;
}

bool CGenericTouchPinchDetector::OnTouchUp(unsigned int index, const Pointer &pointer)
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

bool CGenericTouchPinchDetector::OnTouchMove(unsigned int index, const Pointer &pointer)
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
