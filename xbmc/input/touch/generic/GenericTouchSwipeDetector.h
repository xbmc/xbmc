#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "input/touch/generic/IGenericTouchGestureDetector.h"

/*!
 * \ingroup touch_generic
 * \brief Implementation of IGenericTouchGestureDetector to detect swipe
 *        gestures in any direction.
 *
 * \sa IGenericTouchGestureDetector
 */
class CGenericTouchSwipeDetector : public IGenericTouchGestureDetector
{
public:
  CGenericTouchSwipeDetector(ITouchActionHandler *handler, float dpi);
  ~CGenericTouchSwipeDetector() override { }

  bool OnTouchDown(unsigned int index, const Pointer &pointer) override;
  bool OnTouchUp(unsigned int index, const Pointer &pointer) override;
  bool OnTouchMove(unsigned int index, const Pointer &pointer) override;
  bool OnTouchUpdate(unsigned int index, const Pointer &pointer) override;

private:
  /*!
   * \brief Swipe directions that are still possible to detect
   *
   * The directions are stored as a combination (bitwise OR) of
   * TouchMoveDirection enum values
   *
   * \sa TouchMoveDirection
   */
  unsigned int m_directions;
  /*!
   * \brief Whether a swipe gesture has been detected or not
   */
  bool m_swipeDetected;
  /*!
   * \brief Number of active pointers
   */
  unsigned int m_size;
};
