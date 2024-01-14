/*
 *  Copyright (C) 2012-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

/*!
 * \ingroup touch
 * \brief Directions in which a touch can moved
 *
 * These values can be combined (bitwise OR) to specify multiple directions.
 */
typedef enum
{
  TouchMoveDirectionNone = 0x0,
  TouchMoveDirectionLeft = 0x1,
  TouchMoveDirectionRight = 0x2,
  TouchMoveDirectionUp = 0x4,
  TouchMoveDirectionDown = 0x8
} TouchMoveDirection;

/*!
 * \ingroup touch
 * \brief Interface defining all supported touch action events
 */
class ITouchActionHandler
{
public:
  virtual ~ITouchActionHandler() = default;

  /*!
   * \brief A touch action has been aborted
   */
  virtual void OnTouchAbort() {}

  /*!
   * \brief A single touch has started
   *
   * \param x     The x coordinate (with sub-pixel) of the touch
   * \param y     The y coordinate (with sub-pixel) of the touch
   *
   * \return True if the event was handled otherwise false
   *
   * \sa OnSingleTap
   */
  virtual bool OnSingleTouchStart(float x, float y) { return true; }
  /*!
   * \brief A single touch has been held down for a certain amount of time
   *
   * \param x     The x coordinate (with sub-pixel) of the touch
   * \param y     The y coordinate (with sub-pixel) of the touch
   *
   * \return True if the event was handled otherwise false
   *
   * \sa OnSingleLongPress
   */
  virtual bool OnSingleTouchHold(float x, float y) { return true; }
  /*!
   * \brief A single touch has moved
   *
   * \param x             The x coordinate (with sub-pixel) of the current touch
   * \param y             The y coordinate (with sub-pixel) of the current touch
   * \param offsetX       The covered distance on the x axis (with sub-pixel)
   * \param offsetX       The covered distance on the y axis (with sub-pixel)
   * \param velocityX     The velocity of the gesture in x direction (pixels/second)
   * \param velocityX     The velocity of the gesture in y direction (pixels/second)
   *
   * \return True if the event was handled otherwise false
   *
   * \sa OnTouchGesturePan
   */
  virtual bool OnSingleTouchMove(
      float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
  {
    return true;
  }
  /*!
   * \brief A single touch has been lifted
   *
   * \param x     The x coordinate (with sub-pixel) of the touch
   * \param y     The y coordinate (with sub-pixel) of the touch
   *
   * \return True if the event was handled otherwise false
   *
   * \sa OnSingleTap
   */
  virtual bool OnSingleTouchEnd(float x, float y) { return true; }

  /*!
   * \brief An additional touch has been performed
   *
   * \param x             The x coordinate (with sub-pixel) of the touch
   * \param y             The y coordinate (with sub-pixel) of the touch
   * \param pointer       The pointer that has performed the touch
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchDown(float x, float y, int32_t pointer) { return true; }
  /*!
   * \brief Multiple simultaneous touches have been held down for a certain amount of time
   *
   * \param x             The x coordinate (with sub-pixel) of the touch
   * \param y             The y coordinate (with sub-pixel) of the touch
   * \param pointers      The number of pointers involved (default 2)
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchHold(float x, float y, int32_t pointers = 2) { return true; }
  /*!
   * \brief A touch has moved
   *
   * \param x             The x coordinate (with sub-pixel) of the current touch
   * \param y             The y coordinate (with sub-pixel) of the current touch
   * \param offsetX       The covered distance on the x axis (with sub-pixel)
   * \param offsetX       The covered distance on the y axis (with sub-pixel)
   * \param velocityX     The velocity of the gesture in x direction (pixels/second)
   * \param velocityX     The velocity of the gesture in y direction (pixels/second)
   * \param pointer       The pointer that has performed the touch
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchMove(float x,
                                float y,
                                float offsetX,
                                float offsetY,
                                float velocityX,
                                float velocityY,
                                int32_t pointer)
  {
    return true;
  }
  /*!
   * \brief A touch has been lifted (but there are still active touches)
   *
   * \param x             The x coordinate (with sub-pixel) of the touch
   * \param y             The y coordinate (with sub-pixel) of the touch
   * \param pointer       The pointer that has performed the touch
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchUp(float x, float y, int32_t pointer) { return true; }

  /*!
   * \brief A pan gesture with a single touch has been started
   *
   * \param x     The x coordinate (with sub-pixel) of the initial touch
   * \param y     The y coordinate (with sub-pixel) of the initial touch
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnTouchGestureStart(float x, float y) { return true; }
  /*!
   * \brief A pan gesture with a single touch is in progress
   *
   * \param x             The x coordinate (with sub-pixel) of the current touch
   * \param y             The y coordinate (with sub-pixel) of the current touch
   * \param offsetX       The covered distance on the x axis (with sub-pixel)
   * \param offsetX       The covered distance on the y axis (with sub-pixel)
   * \param velocityX     The velocity of the gesture in x direction (pixels/second)
   * \param velocityX     The velocity of the gesture in y direction (pixels/second)
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnTouchGesturePan(
      float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
  {
    return true;
  }
  /*!
   * \brief A pan gesture with a single touch has ended
   *
   * \param x             The x coordinate (with sub-pixel) of the current touch
   * \param y             The y coordinate (with sub-pixel) of the current touch
   * \param offsetX       The covered distance on the x axis (with sub-pixel)
   * \param offsetX       The covered distance on the y axis (with sub-pixel)
   * \param velocityX     The velocity of the gesture in x direction (pixels/second)
   * \param velocityX     The velocity of the gesture in y direction (pixels/second)
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnTouchGestureEnd(
      float x, float y, float offsetX, float offsetY, float velocityX, float velocityY)
  {
    return true;
  }

  // convenience events
  /*!
   * \brief A tap with a one or more touches has been performed
   *
   * \param x           The x coordinate (with sub-pixel) of the touch
   * \param y           The y coordinate (with sub-pixel) of the touch
   * \param pointers    The number of pointers involved (default 1)
   *
   * \return True if the event was handled otherwise false
   */
  virtual void OnTap(float x, float y, int32_t pointers = 1) {}
  /*!
   * \brief One or more touches have been held down for a certain amount of time
   *
   * \param x           The x coordinate (with sub-pixel) of the touch
   * \param y           The y coordinate (with sub-pixel) of the touch
   * \param pointers    The number of pointers involved (default 1)
   *
   * \return True if the event was handled otherwise false
   *
   * \sa OnSingleTouchHold
   */
  virtual void OnLongPress(float x, float y, int32_t pointers = 1) {}
  /*!
   * \brief One or more touches has been moved quickly in a single direction in a short time
   *
   * \param direction     The direction (left, right, up, down) of the swipe gesture
   * \param xDown         The x coordinate (with sub-pixel) of the first touch
   * \param yDown         The y coordinate (with sub-pixel) of the first touch
   * \param xUp           The x coordinate (with sub-pixel) of the last touch
   * \param yUp           The y coordinate (with sub-pixel) of the last touch
   * \param velocityX     The velocity of the gesture in x direction (pixels/second)
   * \param velocityX     The velocity of the gesture in y direction (pixels/second)
   * \param pointers      The number of pointers involved (default 1)
   *
   * \return True if the event was handled otherwise false
   */
  virtual void OnSwipe(TouchMoveDirection direction,
                       float xDown,
                       float yDown,
                       float xUp,
                       float yUp,
                       float velocityX,
                       float velocityY,
                       int32_t pointers = 1)
  {
  }
  /*!
   * \brief Two simultaneous touches have been held down and moved to perform a zooming/pinching
   * gesture
   *
   * \param centerX       The x coordinate (with sub-pixel) of the center of the two touches
   * \param centerY       The y coordinate (with sub-pixel) of the center of the two touches
   * \param zoomFactor    The zoom (> 1.0) or pinch (< 1.0) factor of the two touches
   *
   * \return True if the event was handled otherwise false
   */
  virtual void OnZoomPinch(float centerX, float centerY, float zoomFactor) {}
  /*!
   * \brief Two simultaneous touches have been held down and moved to perform a rotating gesture
   *
   * \param centerX       The x coordinate (with sub-pixel) of the center of the two touches
   * \param centerY       The y coordinate (with sub-pixel) of the center of the two touches
   * \param angle         The clockwise angle in degrees of the rotation
   *
   * \return True if the event was handled otherwise false
   */
  virtual void OnRotate(float centerX, float centerY, float angle) {}
};
