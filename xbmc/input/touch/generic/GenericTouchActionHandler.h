/*
 *  Copyright (C) 2013-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  static CGenericTouchActionHandler& GetInstance();

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

  /*!
   \brief Asks the control at the given coordinates for a list of the supported gestures.

   \param x     The x coordinate (with sub-pixel) of the touch
   \param y     The y coordinate (with sub-pixel) of the touch

   \return EVENT_RESULT value of bitwise ORed gestures.
   */
  int QuerySupportedGestures(float x, float y);

private:
  // private construction, and no assignments; use the provided singleton methods
  CGenericTouchActionHandler() = default;
  CGenericTouchActionHandler(const CGenericTouchActionHandler&) = delete;
  CGenericTouchActionHandler const& operator=(CGenericTouchActionHandler const&) = delete;
  ~CGenericTouchActionHandler() override = default;

  void sendEvent(int actionId,
                 float x,
                 float y,
                 float x2 = 0.0f,
                 float y2 = 0.0f,
                 float x3 = 0.0f,
                 float y3 = 0.0f,
                 int pointers = 1);
  void focusControl(float x, float y);
};
