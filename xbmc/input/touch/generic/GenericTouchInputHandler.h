/*
 *  Copyright (C) 2012-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/touch/ITouchInputHandler.h"
#include "input/touch/TouchTypes.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"

#include <array>
#include <memory>
#include <set>

class IGenericTouchGestureDetector;

/*!
 * \ingroup touch_generic
 * \brief Generic implementation of ITouchInputHandler to handle low level (raw)
 *        touch events and translate them into touch actions which are passed
 *        on to the registered ITouchActionHandler implementation.
 *
 * The generic implementation supports single a double touch and hold
 * actions and basic gesture recognition for panning, swiping, pinching/zooming
 * and rotating.
 *
 * \sa ITouchInputHandler
 */
class CGenericTouchInputHandler : public ITouchInputHandler, private ITimerCallback
{
public:
  /*!
   \brief Get an instance of the touch input manager
   */
  static CGenericTouchInputHandler& GetInstance();
  static constexpr int MAX_POINTERS = 2;

  // implementation of ITouchInputHandler
  bool HandleTouchInput(TouchInput event,
                        float x,
                        float y,
                        int64_t time,
                        int32_t pointer = 0,
                        float size = 0.0f) override;
  bool UpdateTouchPointer(
      int32_t pointer, float x, float y, int64_t time, float size = 0.0f) override;

private:
  // private construction, and no assignments; use the provided singleton methods
  CGenericTouchInputHandler();
  ~CGenericTouchInputHandler() override;
  CGenericTouchInputHandler(const CGenericTouchInputHandler&) = delete;
  CGenericTouchInputHandler const& operator=(CGenericTouchInputHandler const&) = delete;

  typedef enum
  {
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

  // implementation of ITimerCallback
  void OnTimeout() override;

  void saveLastTouch();
  void setGestureState(TouchGestureState gestureState)
  {
    m_gestureStateOld = m_gestureState;
    m_gestureState = gestureState;
  }
  void triggerDetectors(TouchInput event, int32_t pointer);
  float AdjustPointerSize(float size);

  CCriticalSection m_critical;
  std::unique_ptr<CTimer> m_holdTimer;
  std::array<Pointer, MAX_POINTERS> m_pointers;
  std::set<std::unique_ptr<IGenericTouchGestureDetector>> m_detectors;

  TouchGestureState m_gestureState = TouchGestureUnknown;
  TouchGestureState m_gestureStateOld = TouchGestureUnknown;
};
