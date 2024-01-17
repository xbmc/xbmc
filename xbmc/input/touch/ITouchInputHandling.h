/*
 *  Copyright (C) 2013-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/touch/ITouchActionHandler.h"

#include <stdlib.h>

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
  ITouchInputHandling() : m_handler(NULL) {}
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
  void RegisterHandler(ITouchActionHandler* touchHandler);
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
  bool OnSingleTouchMove(
      float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) override;
  bool OnSingleTouchEnd(float x, float y) override;

  bool OnMultiTouchDown(float x, float y, int32_t pointer) override;
  bool OnMultiTouchHold(float x, float y, int32_t pointers = 2) override;
  bool OnMultiTouchMove(float x,
                        float y,
                        float offsetX,
                        float offsetY,
                        float velocityX,
                        float velocityY,
                        int32_t pointer) override;
  bool OnMultiTouchUp(float x, float y, int32_t pointer) override;

  bool OnTouchGestureStart(float x, float y) override;
  bool OnTouchGesturePan(
      float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) override;
  bool OnTouchGestureEnd(
      float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) override;

  // convenience events
  void OnTap(float x, float y, int32_t pointers = 1) override;
  void OnLongPress(float x, float y, int32_t pointers = 1) override;
  void OnSwipe(TouchMoveDirection direction,
               float xDown,
               float yDown,
               float xUp,
               float yUp,
               float velocityX,
               float velocityY,
               int32_t pointers = 1) override;
  void OnZoomPinch(float centerX, float centerY, float zoomFactor) override;
  void OnRotate(float centerX, float centerY, float angle) override;

private:
  ITouchActionHandler* m_handler;
};
