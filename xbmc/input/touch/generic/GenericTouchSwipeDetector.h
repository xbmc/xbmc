/*
 *  Copyright (C) 2013-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  CGenericTouchSwipeDetector(ITouchActionHandler* handler, float dpi);
  ~CGenericTouchSwipeDetector() override = default;

  bool OnTouchDown(unsigned int index, const Pointer& pointer) override;
  bool OnTouchUp(unsigned int index, const Pointer& pointer) override;
  bool OnTouchMove(unsigned int index, const Pointer& pointer) override;
  bool OnTouchUpdate(unsigned int index, const Pointer& pointer) override;

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
  bool m_swipeDetected = false;
  /*!
   * \brief Number of active pointers
   */
  unsigned int m_size = 0;
};
