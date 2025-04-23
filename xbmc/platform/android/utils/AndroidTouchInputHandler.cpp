/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidTouchInputHandler.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIControl.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "windowing/GraphicContext.h"
#include "windowing/android/WinSystemAndroid.h"
#include "utils/MathUtils.h"
#include "platform/android/utils/AndroidTextureManager.h"
#include <androidjni/SystemClock.h>
#include <cmath>

// Helper function to calculate distance between two points
static float Distance(float x1, float y1, float x2, float y2)
{
  return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

// Helper function to calculate angle between two vectors
static float CalculateAngle(float x1, float y1, float x2, float y2)
{
  return std::atan2(y2 - y1, x2 - x1);
}

CAndroidTouchInputHandler::CAndroidTouchInputHandler()
  : m_enabled(true),
    m_multiTouchEnabled(true),
    m_sensitivity(1.0f),
    m_dpi(160), // Default DPI
    m_activePointerId(-1),
    m_gestureTriggerPointerId(-1),
    m_currentGestureType(TOUCH_GESTURE_NONE),
    m_lastTapTime(0),
    m_lastTapX(0),
    m_lastTapY(0),
    m_startX(0),
    m_startY(0),
    m_lastX(0),
    m_lastY(0),
    m_downTime(0),
    m_longPressTriggered(false),
    m_velocityX(0),
    m_velocityY(0),
    m_isScrolling(false),
    m_scrollVelocityX(0),
    m_scrollVelocityY(0)
{
}

CAndroidTouchInputHandler::~CAndroidTouchInputHandler()
{
  // Ensure timers are stopped
  m_longPressTimer.Stop();
  m_scrollTimer.Stop();
}

void CAndroidTouchInputHandler::Initialize()
{
  // Set up long press timer
  m_longPressTimer.SetTimeout(LONG_PRESS_TIMEOUT_MS);
  
  // Set up scroll deceleration timer
  m_scrollTimer.SetTimeout(16); // ~60fps
  
  CLog::Log(LOGINFO, "CAndroidTouchInputHandler: Initialized with DPI: %d", m_dpi);
}

bool CAndroidTouchInputHandler::OnTouchEvent(const CJNIMotionEvent& event)
{
  if (!m_enabled)
    return false;

  int action = event.getAction();
  int actionMasked = action & CJNIMotionEvent::ACTION_MASK;
  int pointerIndex = (action & CJNIMotionEvent::ACTION_POINTER_INDEX_MASK) >> CJNIMotionEvent::ACTION_POINTER_INDEX_SHIFT;
  
  // Get the current pointer ID and coordinates
  int pointerId = event.getPointerId(pointerIndex);
  float x = event.getX(pointerIndex);
  float y = event.getY(pointerIndex);
  int64_t eventTime = CJNISystemClock::uptimeMillis();
  
  // Create a new touch point
  TouchPoint point;
  point.x = x;
  point.y = y;
  point.timestamp = eventTime;
  point.pressure = event.getPressure(pointerIndex);
  point.pointerId = pointerId;
  point.velocityX = 0;
  point.velocityY = 0;
  
  // Calculate velocity if we have history
  std::unique_lock<std::mutex> lock(m_historyMutex);
  if (!m_touchHistory.empty())
  {
    const TouchPoint& prevPoint = m_touchHistory.back();
    float dt = (eventTime - prevPoint.timestamp) / 1000.0f;
    if (dt > 0)
    {
      point.velocityX = (x - prevPoint.x) / dt;
      point.velocityY = (y - prevPoint.y) / dt;
    }
  }
  
  // Update touch history
  m_touchHistory.push_back(point);
  
  // Limit history size
  if (m_touchHistory.size() > 10)
    m_touchHistory.erase(m_touchHistory.begin());
  lock.unlock();
  
  // Process the action
  switch (actionMasked)
  {
    case CJNIMotionEvent::ACTION_DOWN:
    {
      // Start of touch
      m_pointers.clear();
      m_pointers[pointerId] = point;
      m_activePointerId = pointerId;
      m_gestureTriggerPointerId = pointerId;
      m_startX = x;
      m_startY = y;
      m_lastX = x;
      m_lastY = y;
      m_downTime = eventTime;
      m_longPressTriggered = false;
      m_currentGestureType = TOUCH_GESTURE_NONE;
      
      // Start timer for long press
      m_longPressTimer.Start([this]() {
        this->HandleLongPress(m_lastX, m_lastY);
        m_longPressTriggered = true;
      });
      
      // If we're scrolling, stop
      if (m_isScrolling)
      {
        m_isScrolling = false;
        m_scrollTimer.Stop();
      }
      
      // Pause texture loading during gesture for smoother interaction
      CAndroidTextureManager::PauseBackgroundLoading(true);
      
      return true;
    }
    
    case CJNIMotionEvent::ACTION_POINTER_DOWN:
    {
      // Additional finger touch
      m_pointers[pointerId] = point;
      
      // If we now have 2 pointers, we may start a multi-touch gesture
      if (m_pointers.size() == 2 && m_multiTouchEnabled)
      {
        // Cancel any pending long press
        m_longPressTimer.Stop();
        m_currentGestureType = TOUCH_GESTURE_NONE; // Will detect multi-touch gesture type on move
      }
      
      return true;
    }
    
    case CJNIMotionEvent::ACTION_MOVE:
    {
      // Movement with touch
      if (pointerId == m_activePointerId)
      {
        float dx = x - m_lastX;
        float dy = y - m_lastY;
        float totalDistX = x - m_startX;
        float totalDistY = y - m_startY;
        float totalDist = Distance(m_startX, m_startY, x, y);
        
        // Update velocities with exponential smoothing
        m_velocityX = 0.8f * m_velocityX + 0.2f * point.velocityX;
        m_velocityY = 0.8f * m_velocityY + 0.2f * point.velocityY;
        
        // Update position
        m_lastX = x;
        m_lastY = y;
        m_pointers[pointerId] = point;
        
        // Check if we should transition to a scrolling gesture
        // Cancel long press if we've moved significantly
        if (totalDist > 20 * m_sensitivity && m_currentGestureType == TOUCH_GESTURE_NONE)
        {
          m_longPressTimer.Stop();
          m_currentGestureType = TOUCH_GESTURE_SCROLL;
        }
        
        // Handle scroll if we're in scroll mode
        if (m_currentGestureType == TOUCH_GESTURE_SCROLL)
        {
          return HandleScroll(dx, dy, m_velocityX, m_velocityY);
        }
      }
      else if (m_pointers.size() >= 2 && m_multiTouchEnabled)
      {
        // Update position for this pointer
        m_pointers[pointerId] = point;
        
        // Extract two main pointers for multi-touch gestures
        auto it = m_pointers.begin();
        const TouchPoint& p1 = it->second;
        ++it;
        const TouchPoint& p2 = it->second;
        
        // Calculate pinch/zoom gesture
        float currentDistance = Distance(p1.x, p1.y, p2.x, p2.y);
        static float startDistance = -1;
        
        if (startDistance < 0)
          startDistance = currentDistance;
        else
        {
          float scale = currentDistance / startDistance;
          if (std::abs(scale - 1.0f) > 0.05f)
          {
            // We have a zooming gesture
            m_currentGestureType = TOUCH_GESTURE_ZOOM;
            
            // Process zoom action
            CAction action;
            if (scale > 1.0f)
              action = CAction(ACTION_ZOOM_IN);
            else
              action = CAction(ACTION_ZOOM_OUT);
            
            CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_GESTURE_NOTIFY, 0, 0, static_cast<void*>(&action));
            startDistance = currentDistance; // Reset for next scale calculation
          }
        }
        
        // Also check for rotation gesture
        static float startAngle = -1000;
        float currentAngle = CalculateAngle(p1.x, p1.y, p2.x, p2.y);
        
        if (startAngle < -999)
          startAngle = currentAngle;
        else
        {
          float rotation = currentAngle - startAngle;
          if (std::abs(rotation) > 0.1f) // About 5.7 degrees
          {
            // We have a rotation gesture
            m_currentGestureType = TOUCH_GESTURE_ROTATE;
            
            // Process rotation action
            CAction action;
            if (rotation > 0)
              action = CAction(ACTION_ROTATE_CLOCKWISE);
            else
              action = CAction(ACTION_ROTATE_COUNTERCLOCKWISE);
            
            CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_GESTURE_NOTIFY, 0, 0, static_cast<void*>(&action));
            startAngle = currentAngle; // Reset for next rotation calculation
          }
        }
      }
      
      return true;
    }
    
    case CJNIMotionEvent::ACTION_POINTER_UP:
    {
      // One finger lifted while others remain
      m_pointers.erase(pointerId);
      
      // If this was the active pointer, choose another one
      if (pointerId == m_activePointerId && !m_pointers.empty())
      {
        // Choose first remaining pointer as the primary
        m_activePointerId = m_pointers.begin()->first;
        TouchPoint& newActive = m_pointers.begin()->second;
        m_lastX = newActive.x;
        m_lastY = newActive.y;
      }
      
      return true;
    }
    
    case CJNIMotionEvent::ACTION_UP:
    {
      // Final finger lifted
      m_pointers.erase(pointerId);
      m_longPressTimer.Stop();
      
      // Resume texture loading
      CAndroidTextureManager::PauseBackgroundLoading(false);
      
      // Determine what gesture ended
      if (m_longPressTriggered)
      {
        // Long press already handled
        m_currentGestureType = TOUCH_GESTURE_NONE;
        return true;
      }
      
      // Calculate time and distance
      int64_t pressDuration = eventTime - m_downTime;
      float totalDistX = x - m_startX;
      float totalDistY = y - m_startY;
      float totalDist = Distance(m_startX, m_startY, x, y);
      
      if (m_currentGestureType == TOUCH_GESTURE_SCROLL)
      {
        // Check if we should trigger a fling
        float velocityMagnitude = std::sqrt(m_velocityX * m_velocityX + m_velocityY * m_velocityY);
        if (velocityMagnitude > MIN_FLING_VELOCITY)
        {
          HandleFling(m_velocityX, m_velocityY);
        }
      }
      else if (pressDuration < TAP_TIMEOUT_MS && totalDist < 20 * m_sensitivity)
      {
        // This is a tap
        bool isDoubleTap = false;
        if ((eventTime - m_lastTapTime) < DOUBLE_TAP_TIMEOUT_MS)
        {
          // Check if this is a double-tap (second tap close to first tap)
          float tapDistance = Distance(x, y, m_lastTapX, m_lastTapY);
          if (tapDistance < 40 * m_sensitivity)
            isDoubleTap = true;
        }
        
        HandleTap(x, y, isDoubleTap);
        
        // Store for potential double-tap detection
        m_lastTapX = x;
        m_lastTapY = y;
        m_lastTapTime = eventTime;
      }
      
      m_currentGestureType = TOUCH_GESTURE_NONE;
      m_activePointerId = -1;
      return true;
    }
    
    case CJNIMotionEvent::ACTION_CANCEL:
    {
      // Touch canceled
      m_pointers.clear();
      m_longPressTimer.Stop();
      m_currentGestureType = TOUCH_GESTURE_NONE;
      m_activePointerId = -1;
      
      // Resume texture loading
      CAndroidTextureManager::PauseBackgroundLoading(false);
      
      return true;
    }
  }
  
  return false;
}

bool CAndroidTouchInputHandler::HandleTap(float x, float y, bool isDoubleTap)
{
  CGUIControl* pControl = GetControlUnderPoint(x, y);
  
  if (pControl)
  {
    // Send tap event to the control
    return SendTouchEventToControl(pControl, x, y, isDoubleTap ? ACTION_DOUBLE_CLICK : ACTION_MOUSE_LEFT_CLICK);
  }
  else
  {
    // No control found, send to active window
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui == nullptr)
      return false;
      
    // Convert to action based on screen position and state
    CAction action;
    
    // Map tap location to different actions based on screen quadrants
    CWinSystemAndroid* winSystem = dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem());
    if (winSystem)
    {
      RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
      
      if (isDoubleTap)
      {
        // Double tap detected
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        
        // Double tap to play/pause if we're playing a video
        if (appPlayer->IsPlaying() && appPlayer->IsPlayingVideo())
        {
          action = CAction(ACTION_PLAYER_PLAYPAUSE);
        }
        else
        {
          action = CAction(ACTION_DOUBLE_CLICK);
        }
      }
      else
      {
        // Regular tap
        
        // Calculate normalized position (0-1)
        float xRatio = x / res.iWidth;
        float yRatio = y / res.iHeight;
        
        // Map to different actions based on tap location
        if (xRatio < 0.1f) // Left edge
          action = CAction(ACTION_PREVIOUS_MENU);
        else if (xRatio > 0.9f) // Right edge
          action = CAction(ACTION_NEXT_ITEM);
        else if (yRatio < 0.1f) // Top edge
          action = CAction(ACTION_CONTEXT_MENU);
        else if (yRatio > 0.9f) // Bottom edge
          action = CAction(ACTION_SHOW_INFO);
        else
          action = CAction(ACTION_SELECT_ITEM);
      }
    }
    else
    {
      // Fallback if we can't get window system
      action = CAction(isDoubleTap ? ACTION_DOUBLE_CLICK : ACTION_SELECT_ITEM);
    }
    
    gui->GetWindowManager().OnAction(action);
    return true;
  }
}

bool CAndroidTouchInputHandler::HandleLongPress(float x, float y)
{
  CGUIControl* pControl = GetControlUnderPoint(x, y);
  
  if (pControl)
  {
    // Send long press event to the control
    return SendTouchEventToControl(pControl, x, y, ACTION_MOUSE_LONG_CLICK);
  }
  else
  {
    // No control found, send to active window
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui == nullptr)
      return false;
      
    CAction action(ACTION_CONTEXT_MENU);
    gui->GetWindowManager().OnAction(action);
    return true;
  }
}

bool CAndroidTouchInputHandler::HandleScroll(float distanceX, float distanceY, float velocityX, float velocityY)
{
  // Apply sensitivity
  distanceX *= m_sensitivity;
  distanceY *= m_sensitivity;
  
  // Create appropriate action based on dominant direction
  CAction action;
  if (std::abs(distanceX) > std::abs(distanceY))
  {
    // Horizontal scroll
    if (distanceX > 0)
      action = CAction(ACTION_SCROLL_RIGHT, std::abs(distanceX), 0);
    else
      action = CAction(ACTION_SCROLL_LEFT, std::abs(distanceX), 0);
  }
  else
  {
    // Vertical scroll
    if (distanceY > 0)
      action = CAction(ACTION_SCROLL_DOWN, 0, std::abs(distanceY));
    else
      action = CAction(ACTION_SCROLL_UP, 0, std::abs(distanceY));
  }
  
  // Send to GUI
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
  {
    gui->GetWindowManager().OnAction(action);
    return true;
  }
  
  return false;
}

bool CAndroidTouchInputHandler::HandleFling(float velocityX, float velocityY)
{
  // Limit velocity to reasonable range
  velocityX = MathUtils::Clamp(velocityX, -MAX_FLING_VELOCITY, MAX_FLING_VELOCITY);
  velocityY = MathUtils::Clamp(velocityY, -MAX_FLING_VELOCITY, MAX_FLING_VELOCITY);
  
  // Apply sensitivity
  velocityX *= m_sensitivity;
  velocityY *= m_sensitivity;
  
  // Determine dominant direction
  if (std::abs(velocityX) > std::abs(velocityY))
  {
    // Horizontal fling
    m_scrollVelocityX = velocityX;
    m_scrollVelocityY = 0;
  }
  else
  {
    // Vertical fling
    m_scrollVelocityX = 0;
    m_scrollVelocityY = velocityY;
  }
  
  // Set up kinetic scrolling animation
  m_isScrolling = true;
  m_scrollTimer.Start([this]() {
    // Get delta time in seconds
    static int64_t lastTime = CJNISystemClock::uptimeMillis();
    int64_t now = CJNISystemClock::uptimeMillis();
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;
    
    // Calculate scroll distance for this frame
    float dx = m_scrollVelocityX * dt;
    float dy = m_scrollVelocityY * dt;
    
    // Apply deceleration
    const float friction = 0.95f;
    m_scrollVelocityX *= friction;
    m_scrollVelocityY *= friction;
    
    // Stop scrolling if velocity gets too low
    if (std::abs(m_scrollVelocityX) < 50 && std::abs(m_scrollVelocityY) < 50)
    {
      m_isScrolling = false;
      m_scrollTimer.Stop();
      return;
    }
    
    // Apply the scroll
    HandleScroll(dx, dy, m_scrollVelocityX, m_scrollVelocityY);
  });
  
  return true;
}

void CAndroidTouchInputHandler::ApplyKineticScrolling(float initialVelocity, float friction)
{
  // This is handled in HandleFling
}

TouchPoint CAndroidTouchInputHandler::PredictMotion(const std::vector<TouchPoint>& points, int lookaheadMs)
{
  // Need at least two points for prediction
  if (points.size() < 2)
    return points.empty() ? TouchPoint() : points.back();
  
  // Use last two points to extrapolate future position
  const TouchPoint& p1 = *(points.end() - 2);
  const TouchPoint& p2 = points.back();
  
  // Calculate velocity
  float dt = static_cast<float>(p2.timestamp - p1.timestamp);
  if (dt == 0)
    return p2;
    
  float vx = (p2.x - p1.x) / dt;
  float vy = (p2.y - p1.y) / dt;
  
  // Predict position
  TouchPoint prediction = p2;
  prediction.x += vx * lookaheadMs;
  prediction.y += vy * lookaheadMs;
  prediction.velocityX = vx;
  prediction.velocityY = vy;
  
  return prediction;
}

CGUIControl* CAndroidTouchInputHandler::GetControlUnderPoint(float x, float y)
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (!gui)
    return nullptr;
    
  CGUIWindow* pWindow = gui->GetWindowManager().GetWindow(gui->GetWindowManager().GetActiveWindow());
  if (!pWindow)
    return nullptr;
    
  // Transform coordinates to GUI space
  CPoint point(x, y);
  CWinSystemAndroid* winSystem = dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem());
  if (winSystem)
  {
    CRect rc = CXBMCApp::Get().MapRenderToDroid(CRect(0, 0, 
      CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
      CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()));
      
    // Scale coordinates to GUI space
    point.x = (point.x - rc.x1) / (rc.Width() / CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth());
    point.y = (point.y - rc.y1) / (rc.Height() / CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
  }
  
  return pWindow->GetControl(pWindow->GetFocusedControlID());
}

bool CAndroidTouchInputHandler::SendTouchEventToControl(CGUIControl* control, float x, float y, int gestureType)
{
  if (!control)
    return false;
    
  int actionId = ACTION_NONE;
  
  // Map gesture to appropriate action
  switch (gestureType)
  {
    case TOUCH_GESTURE_TAP:
      actionId = ACTION_MOUSE_LEFT_CLICK;
      break;
    case TOUCH_GESTURE_LONG_PRESS:
      actionId = ACTION_MOUSE_LONG_CLICK;
      break;
    case TOUCH_GESTURE_SCROLL:
      // The actual scroll handling is done in HandleScroll
      return false;
    case TOUCH_GESTURE_FLING:
      // Fling is handled separately
      return false;
    default:
      // Direct action ID provided
      actionId = gestureType;
      break;
  }
  
  if (actionId == ACTION_NONE)
    return false;
    
  CAction action(actionId);
  return control->OnAction(action);
}

void CAndroidTouchInputHandler::SetEnabled(bool enable)
{
  m_enabled = enable;
}

void CAndroidTouchInputHandler::SetSensitivity(float sensitivity)
{
  // Clamp to reasonable range
  m_sensitivity = MathUtils::Clamp(sensitivity, 0.1f, 3.0f);
}

void CAndroidTouchInputHandler::EnableMultiTouch(bool enable)
{
  m_multiTouchEnabled = enable;
}

void CAndroidTouchInputHandler::SetDPI(int dpi)
{
  if (dpi > 0)
    m_dpi = dpi;
}