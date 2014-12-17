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
 * \brief Implementation of IGenericTouchGestureDetector to detect pinch/zoom
 *        gestures with at least two active touch pointers.
 *
 * \sa IGenericTouchGestureDetector
 */
class CGenericTouchPinchDetector : public IGenericTouchGestureDetector
{
public:
  CGenericTouchPinchDetector(ITouchActionHandler *handler, float dpi)
    : IGenericTouchGestureDetector(handler, dpi)
  { }
  virtual ~CGenericTouchPinchDetector() { }

  virtual bool OnTouchDown(unsigned int index, const Pointer &pointer);
  virtual bool OnTouchUp(unsigned int index, const Pointer &pointer);
  virtual bool OnTouchMove(unsigned int index, const Pointer &pointer);
};
