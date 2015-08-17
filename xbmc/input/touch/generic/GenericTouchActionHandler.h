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

#include "input/touch/ITouchActionHandler.h"

/*!
 * \ingroup touch_generic
 * \brief Generic implementation of ITouchActionHandler to translate
 *        touch actions into XBMC specific and mappable actions.
 *
 * \sa ITouchActionHandler
 */
class CGenericTouchActionHandler : public ITouchActionHandler
{
public:
  /*!
   \brief Get an instance of the touch input manager
   */
  static CGenericTouchActionHandler &GetInstance();

  // implementation of ITouchActionHandler
  virtual void OnTouchAbort();

  virtual bool OnSingleTouchStart(float x, float y);
  virtual bool OnSingleTouchHold(float x, float y);
  virtual bool OnSingleTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);
  virtual bool OnSingleTouchEnd(float x, float y);

  virtual bool OnMultiTouchDown(float x, float y, int32_t pointer);
  virtual bool OnMultiTouchHold(float x, float y, int32_t pointers = 2);
  virtual bool OnMultiTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY, int32_t pointer);
  virtual bool OnMultiTouchUp(float x, float y, int32_t pointer);

  virtual bool OnTouchGestureStart(float x, float y);
  virtual bool OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);
  virtual bool OnTouchGestureEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);

  // convenience events
  virtual void OnTap(float x, float y, int32_t pointers = 1);
  virtual void OnLongPress(float x, float y, int32_t pointers = 1);
  virtual void OnSwipe(TouchMoveDirection direction, float xDown, float yDown, float xUp, float yUp, float velocityX, float velocityY, int32_t pointers = 1);
  virtual void OnZoomPinch(float centerX, float centerY, float zoomFactor);
  virtual void OnRotate(float centerX, float centerY, float angle);

  /*!
   \brief Asks the control at the given coordinates for a list of the supported gestures.

   \param x     The x coordinate (with sub-pixel) of the touch
   \param y     The y coordinate (with sub-pixel) of the touch

   \return EVENT_RESULT value of bitwise ORed gestures.
   */
  int QuerySupportedGestures(float x, float y);

private:
  // private construction, and no assignements; use the provided singleton methods
  CGenericTouchActionHandler() { }
  CGenericTouchActionHandler(const CGenericTouchActionHandler&);
  CGenericTouchActionHandler const& operator=(CGenericTouchActionHandler const&);
  virtual ~CGenericTouchActionHandler() { }

  void touch(uint8_t type, uint8_t button, uint16_t x, uint16_t y);
  void sendEvent(int actionId, float x, float y, float x2 = 0.0f, float y2 = 0.0f, int pointers = 1);
  void focusControl(float x, float y);
};
