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

#include <stdlib.h>

#include "input/touch/ITouchActionHandler.h"

/*!
 * \ingroup touch
 * \brief Convenience interface implementing ITouchActionHandler with an
 *        implementation that forwards any ITouchActionHandler-related calls
 *        to a previously registered ITouchActionHandler
 *
 * \sa ITouchActionHandler
 */
class ITouchInputHandling : protected ITouchActionHandler
{
public:
  ITouchInputHandling()
    : m_handler(NULL)
  { }
  ~ITouchInputHandling() override = default;

  /*!
   * \brief Register a touch input handler
   *
   * There can only be one touch input handler.
   *
   * \param touchHandler    An instance of a touch handler implementing the
   *                        ITouchActionHandler interface
   *
   * \sa UnregisterHandler
   */
  void RegisterHandler(ITouchActionHandler *touchHandler);
  /*!
   * \brief Unregister the previously registered touch handler
   *
   * \sa RegisterHandler
   */
  void UnregisterHandler();

protected:
  // implementation of ITouchActionHandler
  void OnTouchAbort() override;

  bool OnSingleTouchStart(float x, float y) override;
  bool OnSingleTouchHold(float x, float y) override;
  bool OnSingleTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) override;
  bool OnSingleTouchEnd(float x, float y) override;

  bool OnMultiTouchDown(float x, float y, int32_t pointer) override;
  bool OnMultiTouchHold(float x, float y, int32_t pointers = 2) override;
  bool OnMultiTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY, int32_t pointer) override;
  bool OnMultiTouchUp(float x, float y, int32_t pointer) override;

  bool OnTouchGestureStart(float x, float y) override;
  bool OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) override;
  bool OnTouchGestureEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) override;

  // convenience events
  void OnTap(float x, float y, int32_t pointers = 1) override;
  void OnLongPress(float x, float y, int32_t pointers = 1) override;
  void OnSwipe(TouchMoveDirection direction, float xDown, float yDown, float xUp, float yUp, float velocityX, float velocityY, int32_t pointers = 1) override;
  void OnZoomPinch(float centerX, float centerY, float zoomFactor) override;
  void OnRotate(float centerX, float centerY, float angle) override;

private:
  ITouchActionHandler *m_handler;
};
