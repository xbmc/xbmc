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
 * \brief Implementation of IGenericTouchGestureDetector to detect rotation
 *        gestures with at least two active touch pointers.
 *
 * \sa IGenericTouchGestureDetector
 */
class CGenericTouchRotateDetector : public IGenericTouchGestureDetector
{
public:
  CGenericTouchRotateDetector(ITouchActionHandler* handler, float dpi);
  ~CGenericTouchRotateDetector() override = default;

  bool OnTouchDown(unsigned int index, const Pointer& pointer) override;
  bool OnTouchUp(unsigned int index, const Pointer& pointer) override;
  bool OnTouchMove(unsigned int index, const Pointer& pointer) override;
  bool OnTouchUpdate(unsigned int index, const Pointer& pointer) override;

private:
  /*!
   * \brief Angle of the detected rotation
   */
  float m_angle = 0.0f;
};
