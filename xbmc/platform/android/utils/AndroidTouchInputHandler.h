/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/View.h>
#include <androidjni/MotionEvent.h>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include "threads/Timer.h"
#include "guilib/GUIControl.h"

// Touch gesture types
#define TOUCH_GESTURE_NONE       0
#define TOUCH_GESTURE_TAP        1
#define TOUCH_GESTURE_LONG_PRESS 2
#define TOUCH_GESTURE_SCROLL     3
#define TOUCH_GESTURE_FLING      4
#define TOUCH_GESTURE_ZOOM       5
#define TOUCH_GESTURE_ROTATE     6

// Fling velocity thresholds
#define MIN_FLING_VELOCITY 500  // Minimum velocity for fling gesture (dp/s)
#define MAX_FLING_VELOCITY 8000 // Maximum velocity for fling gesture (dp/s)

// Gesture timing constants
#define TAP_TIMEOUT_MS 100       // Maximum time for a tap
#define LONG_PRESS_TIMEOUT_MS 500 // Time for long press
#define DOUBLE_TAP_TIMEOUT_MS 300 // Maximum time between taps for double-tap

// Motion prediction lookahead time (ms)
#define MOTION_PREDICTION_MS 16   // Predict ~1 frame ahead

struct TouchPoint 
{
  float x;
  float y;
  int64_t timestamp;
  float pressure;
  int pointerId;
  float velocityX;
  float velocityY;
};

class CAndroidTouchInputHandler
{
public:
  CAndroidTouchInputHandler();
  ~CAndroidTouchInputHandler();

  /**
   * @brief Initialize the touch handler
   */
  void Initialize();
  
  /**
   * @brief Process Android motion event
   * @param event The Android motion event
   * @return True if the event was handled, false otherwise
   */
  bool OnTouchEvent(const CJNIMotionEvent& event);
  
  /**
   * @brief Enable or disable touch input processing
   * @param enable True to enable, false to disable
   */
  void SetEnabled(bool enable);
  
  /**
   * @brief Set the touch input sensitivity
   * @param sensitivity Value between 0.0 and 2.0, with 1.0 being normal
   */
  void SetSensitivity(float sensitivity);
  
  /**
   * @brief Enable or disable multi-touch gestures
   * @param enable True to enable, false to disable
   */
  void EnableMultiTouch(bool enable);
  
  /**
   * @brief Update the screen DPI for touch calculations
   * @param dpi The screen DPI value
   */
  void SetDPI(int dpi);

private:
  /**
   * @brief Process a tap event
   * @param x X-coordinate of the tap
   * @param y Y-coordinate of the tap
   * @param isDoubleTap True if double tap detected
   * @return True if the tap was handled, false otherwise
   */
  bool HandleTap(float x, float y, bool isDoubleTap);
  
  /**
   * @brief Process a long press event
   * @param x X-coordinate of the long press
   * @param y Y-coordinate of the long press
   * @return True if the long press was handled, false otherwise
   */
  bool HandleLongPress(float x, float y);
  
  /**
   * @brief Process a scroll event
   * @param distanceX X distance scrolled
   * @param distanceY Y distance scrolled
   * @param velocityX X velocity of scroll
   * @param velocityY Y velocity of scroll
   * @return True if the scroll was handled, false otherwise
   */
  bool HandleScroll(float distanceX, float distanceY, float velocityX, float velocityY);
  
  /**
   * @brief Process a fling event
   * @param velocityX X velocity of fling
   * @param velocityY Y velocity of fling
   * @return True if the fling was handled, false otherwise
   */
  bool HandleFling(float velocityX, float velocityY);
  
  /**
   * @brief Apply kinetic scrolling after a fling
   * @param initialVelocity Initial velocity of the scroll
   * @param friction Friction coefficient
   */
  void ApplyKineticScrolling(float initialVelocity, float friction = 0.98f);
  
  /**
   * @brief Predict touch motion for reduced latency
   * @param points History of touch points
   * @param lookaheadMs Milliseconds to predict ahead
   * @return Predicted position
   */
  TouchPoint PredictMotion(const std::vector<TouchPoint>& points, int lookaheadMs);
  
  /**
   * @brief Find control under a point
   * @param x X-coordinate
   * @param y Y-coordinate
   * @return Pointer to the control or nullptr if none found
   */
  CGUIControl* GetControlUnderPoint(float x, float y);

  /**
   * @brief Handle touch on a specific control
   * @param control Control to send touch event to
   * @param x X-coordinate
   * @param y Y-coordinate
   * @param gestureType Type of gesture
   * @return True if handled, false otherwise
   */
  bool SendTouchEventToControl(CGUIControl* control, float x, float y, int gestureType);

  // Touch input state
  bool m_enabled;
  bool m_multiTouchEnabled;
  float m_sensitivity;
  int m_dpi;
  int m_activePointerId;
  int m_gestureTriggerPointerId;
  int m_currentGestureType;
  int64_t m_lastTapTime;
  float m_lastTapX;
  float m_lastTapY;
  float m_startX;
  float m_startY;
  float m_lastX;
  float m_lastY;
  int64_t m_downTime;
  bool m_longPressTriggered;
  
  // Timer for delayed actions like long-press
  CTimer m_longPressTimer;
  
  // Track touch velocity for gestures
  float m_velocityX;
  float m_velocityY;
  
  // History of recent touch points for prediction
  std::vector<TouchPoint> m_touchHistory;
  std::mutex m_historyMutex;
  
  // Kinetic scrolling variables
  bool m_isScrolling;
  float m_scrollVelocityX;
  float m_scrollVelocityY;
  CTimer m_scrollTimer;
  
  // Pointer tracking for multi-touch
  std::map<int, TouchPoint> m_pointers;
};