#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include "threads/CriticalSection.h"
#include "threads/Timer.h"
#include "utils/Vector.h"

#define TOUCH_MAX_POINTERS  2

class ITouchHandler
{
public:
  virtual ~ITouchHandler() { }

  /*!
   \brief A touch action has been aborted
   */
  virtual void OnTouchAbort() { };

  /*!
   \brief A single touch has started

   \param x     The x coordinate (with sub-pixel) of the touch
   \param y     The y coordinate (with sub-pixel) of the touch
   \return True if the event was handled otherwise false
   \sa OnSingleTap
   */
  virtual bool OnSingleTouchStart(float x, float y) { return true; }
  /*!
   \brief A single touch has been held down for a certain amount of time

   \param x     The x coordinate (with sub-pixel) of the touch
   \param y     The y coordinate (with sub-pixel) of the touch
   \return True if the event was handled otherwise false
   \sa OnSingleLongPress
   */
  virtual bool OnSingleTouchHold(float x, float y) { return true; }
  /*!
   \brief A single touch has moved                                                                                                                        *

   \param x             The x coordinate (with sub-pixel) of the current touch
   \param y             The y coordinate (with sub-pixel) of the current touch
   \param offsetX       The covered distance on the x axis (with sub-pixel)
   \param offsetX       The covered distance on the y axis (with sub-pixel)
   \param velocityX     The velocity of the gesture in x direction (pixels/second)
   \param velocityX     The velocity of the gesture in y direction (pixels/second)
   \return True if the event was handled otherwise false
   \sa OnTouchGesturePan
   */
  virtual bool OnSingleTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) { return true; }
  /*!
   \brief A single touch has been lifted

   \param x     The x coordinate (with sub-pixel) of the touch
   \param y     The y coordinate (with sub-pixel) of the touch
   \return True if the event was handled otherwise false
   \sa OnSingleTap
   */
  virtual bool OnSingleTouchEnd(float x, float y) { return true; }

  /*!
   \brief A multi touch gesture has started

   \param x             The x coordinate (with sub-pixel) of the touch
   \param y             The y coordinate (with sub-pixel) of the touch
   \param pointers      The number of pointers involved (default 2)
   \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchStart(float x, float y, int32_t pointers = 2) { return true; }
  /*!
   \brief An additional touch has been performed

   \param x             The x coordinate (with sub-pixel) of the touch
   \param y             The y coordinate (with sub-pixel) of the touch
   \param pointer       The pointer that has performed the touch
   \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchDown(float x, float y, int32_t pointer) { return true; }
  /*!
   \brief Multiple simultaneous touches have been held down for a certain amount of time

   \param x             The x coordinate (with sub-pixel) of the touch
   \param y             The y coordinate (with sub-pixel) of the touch
   \param pointers      The number of pointers involved (default 2)
   \return True if the event was handled otherwise false
   \sa OnDoubleLongPress
   */
  virtual bool OnMultiTouchHold(float x, float y, int32_t pointers = 2) { return true; }
  /*!
   \brief A touch has moved

   \param x             The x coordinate (with sub-pixel) of the current touch
   \param y             The y coordinate (with sub-pixel) of the current touch
   \param offsetX       The covered distance on the x axis (with sub-pixel)
   \param offsetX       The covered distance on the y axis (with sub-pixel)
   \param velocityX     The velocity of the gesture in x direction (pixels/second)
   \param velocityX     The velocity of the gesture in y direction (pixels/second)
   \param pointer       The pointer that has performed the touch
   \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY, int32_t pointer) { return true; }
  /*!
   \brief A touch has been lifted (but there are still active touches)

   \param x             The x coordinate (with sub-pixel) of the touch
   \param y             The y coordinate (with sub-pixel) of the touch
   \param pointer       The pointer that has performed the touch
   \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchUp(float x, float y, int32_t pointer) { return true; }
  /*!
   \brief A multi touch gesture has ended (the last touch has been lifted)

   \param x             The x coordinate (with sub-pixel) of the touch
   \param y             The y coordinate (with sub-pixel) of the touch
   \param pointers      The number of pointers involved (default 2)
   \return True if the event was handled otherwise false
   */
  virtual bool OnMultiTouchEnd(float x, float y, int32_t pointers = 2) { return true; }

  /*!
   \brief A pan gesture with a single touch has been started

   \param x     The x coordinate (with sub-pixel) of the initial touch
   \param y     The y coordinate (with sub-pixel) of the initial touch
   \return True if the event was handled otherwise false
   */
  virtual bool OnTouchGesturePanStart(float x, float y) { return true; }
  /*!
   \brief A pan gesture with a single touch is in progress

   \param x             The x coordinate (with sub-pixel) of the current touch
   \param y             The y coordinate (with sub-pixel) of the current touch
   \param offsetX       The covered distance on the x axis (with sub-pixel)
   \param offsetX       The covered distance on the y axis (with sub-pixel)
   \param velocityX     The velocity of the gesture in x direction (pixels/second)
   \param velocityX     The velocity of the gesture in y direction (pixels/second)
   \return True if the event was handled otherwise false
   */
  virtual bool OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) { return true; }
  /*!
   \brief A pan gesture with a single touch has ended

   \param x             The x coordinate (with sub-pixel) of the current touch
   \param y             The y coordinate (with sub-pixel) of the current touch
   \param offsetX       The covered distance on the x axis (with sub-pixel)
   \param offsetX       The covered distance on the y axis (with sub-pixel)
   \param velocityX     The velocity of the gesture in x direction (pixels/second)
   \param velocityX     The velocity of the gesture in y direction (pixels/second)
   \return True if the event was handled otherwise false
   */
  virtual bool OnTouchGesturePanEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY) { return true; }

  // convenience events
  /*!
   \brief A tap with a single touch has been performed

   \param x     The x coordinate (with sub-pixel) of the touch
   \param y     The y coordinate (with sub-pixel) of the touch
   \return True if the event was handled otherwise false
   */
  virtual void OnSingleTap(float x, float y) { }
  /*!
   \brief A single touch has been held down for a certain amount of time

   \param x     The x coordinate (with sub-pixel) of the touch
   \param y     The y coordinate (with sub-pixel) of the touch
   \return True if the event was handled otherwise false
   \sa OnSingleTouchHold
   */
  virtual void OnSingleLongPress(float x, float y) { }
  /*!
   \brief A tap with two simultaneous touches has been performed

   \param x1    The x coordinate (with sub-pixel) of the first touch
   \param y1    The y coordinate (with sub-pixel) of the first touch
   \param x2    The x coordinate (with sub-pixel) of the second touch
   \param y2    The y coordinate (with sub-pixel) of the second touch
   \return True if the event was handled otherwise false
   \sa 
   */
  virtual void OnDoubleTap(float x1, float y1, float x2, float y2) { }
  /*!
   \brief Two simultaneous touches have been held down for a certain amount of time

   \param x1    The x coordinate (with sub-pixel) of the first touch
   \param y1    The y coordinate (with sub-pixel) of the first touch
   \param x2    The x coordinate (with sub-pixel) of the second touch
   \param y2    The y coordinate (with sub-pixel) of the second touch
   \return True if the event was handled otherwise false
   \sa OnMultiTouchHold
   */
  virtual void OnDoubleLongPress(float x1, float y1, float x2, float y2) { }
  /*!
   \brief Two simultaneous touches have been held down and moved to perform a zooming/pinching gesture

   \param centerX       The x coordinate (with sub-pixel) of the center of the two touches
   \param centerY       The y coordinate (with sub-pixel) of the center of the two touches
   \param zoomFactor    The zoom (> 1.0) or pinch (< 1.0) factor of the two touches
   \return True if the event was handled otherwise false
   \sa 
   */
  virtual void OnZoomPinch(float centerX, float centerY, float zoomFactor) { }
  /*!
   \brief Two simultaneous touches have been held down and moved to perform a rotating gesture

   \param centerX       The x coordinate (with sub-pixel) of the center of the two touches
   \param centerY       The y coordinate (with sub-pixel) of the center of the two touches
   \param angle         The clockwise angle in degrees of the rotation
   \return True if the event was handled otherwise false
   \sa
   */
  virtual void OnRotate(float centerX, float centerY, float angle) { }
};

class CTouchInput : private ITouchHandler, private ITimerCallback
{
public:
  typedef enum {
    TouchEventAbort,
    TouchEventDown,
    TouchEventUp,
    TouchEventMove
  } TouchEvent;

  /*!
   \brief Get an instance of the touch input manager
   */
  static CTouchInput &Get();

  /*!
   \brief Register a touch input handler

   There can only be one touch input handler
   active.

   \param touchHandler An instance of a touch handler implementing the ITouchHandler interface
   */
  void RegisterHandler(ITouchHandler *touchHandler);
  /*!
   \brief Unregister the touch handler
   */
  void UnregisterHandler();

  /*!
   \brief Set the timeout after which a touch turns into a touch hold
   \param timeout  Timeout in milliseconds
   */
  void SetTouchHoldTimeout(int32_t timeout);

  /*!
   \brief Handle a touch event

   Handles the given touch event at the given location.
   This takes into account all the currently active pointers
   which need to be updated before calling this method to
   actually interprete and handle the changes in touch.

   \param event    The actual touch event (abort, down, up, move)
   \param x        The x coordinate (with sub-pixel) of the touch
   \param y        The y coordinate (with sub-pixel) of the touch
   \param time     The time (in nanoseconds) when this touch occured
   \param pointer  The number of the touch pointer which caused this event (default 0)
   \param size     The size of the touch pointer (with sub-pixel) (default 0.0)
   \return True if the event was handled otherwise false.
   \sa Update
   */
  bool Handle(TouchEvent event, float x, float y, int64_t time, int32_t pointer = 0, float size = 0.0f);

  /*!
   \brief Update the coordinates of a pointer

   Depending on how a platform handles touch input and provides the necessary events
   this method needs to be called at different times. If there's an event for every
   touch action this method does not need to be called at all. If there's only a
   touch event for the primary pointer (and no special events for any secondary
   pointers in a multi touch gesture) this method should be called for every active
   secondary pointer before calling Handle.

   \param pointer  The number of the touch pointer which caused this event (default 0)
   \param x        The x coordinate (with sub-pixel) of the touch
   \param y        The y coordinate (with sub-pixel) of the touch
   \param time     The time (in nanoseconds) when this touch occured
   \param size     The size of the touch pointer (with sub-pixel) (default 0.0)
   \return True if the pointer was updated otherwise false.
   \sa Handle
   */
  bool Update(int32_t pointer, float x, float y, int64_t time, float size = 0.0f);

private:
  // private construction, and no assignements; use the provided singleton methods
  CTouchInput();
  CTouchInput(const CTouchInput&);
  CTouchInput const& operator=(CTouchInput const&);
  virtual ~CTouchInput();

  void saveLastTouch();

  void handleMultiTouchGesture();
  void handleZoomPinch();
  void handleRotation();

  // implementation of ITimerCallback
  virtual void OnTimeout();

  // implementation of ITouchHandler
  virtual void OnTouchAbort();

  virtual bool OnSingleTouchStart(float x, float y);
  virtual bool OnSingleTouchHold(float x, float y);
  virtual bool OnSingleTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);
  virtual bool OnSingleTouchEnd(float x, float y);

  virtual bool OnMultiTouchStart(float x, float y, int32_t pointers = 2);
  virtual bool OnMultiTouchDown(float x, float y, int32_t pointer);
  virtual bool OnMultiTouchHold(float x, float y, int32_t pointers = 2);
  virtual bool OnMultiTouchMove(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY, int32_t pointer);
  virtual bool OnMultiTouchUp(float x, float y, int32_t pointer);
  virtual bool OnMultiTouchEnd(float x, float y, int32_t pointers = 2);

  virtual bool OnTouchGesturePanStart(float x, float y);
  virtual bool OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);
  virtual bool OnTouchGesturePanEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);

  // convenience events
  virtual void OnSingleTap(float x, float y);
  virtual void OnSingleLongPress(float x, float y);
  virtual void OnDoubleTap(float x1, float y1, float x2, float y2);
  virtual void OnDoubleLongPress(float x1, float y1, float x2, float y2);
  virtual void OnZoomPinch(float centerX, float centerY, float zoomFactor);
  virtual void OnRotate(float centerX, float centerY, float angle);

  CCriticalSection m_critical;

  int32_t m_holdTimeout;
  ITouchHandler *m_handler;
  CTimer *m_holdTimer;

  float m_fRotateAngle;

  class Touch : public CVector {
    public:
      Touch() { reset(); }
      virtual ~Touch() { }

      virtual void reset() { CVector::reset(); time = -1; }

      bool valid() const { return x >= 0.0f && y >= 0.0f && time >= 0; }
      void copy(const Touch &other) { x = other.x; y = other.y; time = other.time; }

      int64_t time; // in nanoseconds
  };

  class Pointer {
    public:
      Pointer() { reset(); }

      bool valid() const { return down.valid(); }
      void reset() { down.reset(); last.reset(); moving = false; size = 0.0f; }

      bool velocity(float &velocityX, float &velocityY, bool fromLast = true)
      {
        Touch &from = last;
        if (!fromLast)
          from = down;

        velocityX = 0.0f; // number of pixels per second
        velocityY = 0.0f; // number of pixels per second

        int64_t timeDiff = current.time - from.time;
        if (timeDiff <= 0)
          return false;

        velocityX = ((current.x - from.x) * 1000000000) / timeDiff;
        velocityY = ((current.y - from.y) * 1000000000) / timeDiff;
        return true;
      }

      Touch down;
      Touch last;
      Touch current;
      bool moving;
      float size;
  };

  Pointer m_pointers[TOUCH_MAX_POINTERS];

  typedef enum {
    TouchGestureUnknown = 0,
    // only primary pointer active but stationary so far
    TouchGestureSingleTouch,
    // primary pointer active but stationary for a certain time
    TouchGestureSingleTouchHold,
    // primary pointer moving
    TouchGesturePan,
    // at least two pointers active but stationary so far
    TouchGestureMultiTouchStart,
    // at least two pointers active but stationary for a certain time
    TouchGestureMultiTouchHold,
    // at least two pointers active and moving
    TouchGestureMultiTouch,
    // all but primary pointer have been lifted
    TouchGestureMultiTouchDone
  } TouchGestureState;

  TouchGestureState m_gestureState;
  TouchGestureState m_gestureStateOld;
  
  void setGestureState(TouchGestureState gestureState) { m_gestureStateOld = m_gestureState; m_gestureState = gestureState; }
};
