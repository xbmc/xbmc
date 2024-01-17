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
 * \brief Implementation of IGenericTouchGestureDetector to detect pinch/zoom
 *        gestures with at least two active touch pointers.
 *
 * \sa IGenericTouchGestureDetector
 */
class CGenericTouchPinchDetector : public IGenericTouchGestureDetector
{
public:
  CGenericTouchPinchDetector(ITouchActionHandler* handler, float dpi)
    : IGenericTouchGestureDetector(handler, dpi)
  {
  }
  ~CGenericTouchPinchDetector() override = default;

  bool OnTouchDown(unsigned int index, const Pointer& pointer) override;
  bool OnTouchUp(unsigned int index, const Pointer& pointer) override;
  bool OnTouchMove(unsigned int index, const Pointer& pointer) override;
};
