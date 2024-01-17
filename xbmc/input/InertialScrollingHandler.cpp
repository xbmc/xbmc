/*
 *  Copyright (C) 2011-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InertialScrollingHandler.h"

#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "utils/TimeUtils.h"
#include "windowing/WinSystem.h"

#include <cmath>
#include <numeric>

// time for reaching velocity 0 in secs
#define TIME_TO_ZERO_SPEED 1.0f
// minimum speed for doing inertial scroll is 100 pixels / s
#define MINIMUM_SPEED_FOR_INERTIA 200
// maximum speed for reducing time to zero
#define MAXIMUM_SPEED_FOR_REDUCTION 750
// maximum time between last movement and gesture end in ms to consider as moving
#define MAXIMUM_DELAY_FOR_INERTIA 200

CInertialScrollingHandler::CInertialScrollingHandler() : m_iLastGesturePoint(CPoint(0, 0))
{
}

unsigned int CInertialScrollingHandler::PanPoint::TimeElapsed() const
{
  return CTimeUtils::GetFrameTime() - time;
}

bool CInertialScrollingHandler::CheckForInertialScrolling(const CAction* action)
{
  bool ret = false; // return value - false no inertial scrolling - true - inertial scrolling

  if (CServiceBroker::GetWinSystem()->HasInertialGestures())
  {
    return ret; // no need for emulating inertial scrolling - windowing does support it natively.
  }

  // reset screensaver during pan
  if (action->GetID() == ACTION_GESTURE_PAN)
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPower = components.GetComponent<CApplicationPowerHandling>();
    if (appPower)
      appPower->ResetScreenSaver();
    if (!m_bScrolling)
    {
      m_panPoints.emplace_back(CTimeUtils::GetFrameTime(),
                               CVector{action->GetAmount(4), action->GetAmount(5)});
    }
    return false;
  }

  // mouse click aborts scrolling
  if (m_bScrolling && action->GetID() == ACTION_MOUSE_LEFT_CLICK)
  {
    ret = true;
    m_bAborting = true; // lets abort
  }

  // trim saved pan points to time range that qualifies for inertial scrolling
  while (!m_panPoints.empty() && m_panPoints.front().TimeElapsed() > MAXIMUM_DELAY_FOR_INERTIA)
    m_panPoints.pop_front();

  // on begin/tap stop all inertial scrolling
  if (action->GetID() == ACTION_GESTURE_BEGIN)
  {
    // release any former exclusive mouse mode
    // for making switching between multiple lists
    // possible
    CGUIMessage message(GUI_MSG_EXCLUSIVE_MOUSE, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    m_bScrolling = false;
    // wakeup screensaver on pan begin
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPower = components.GetComponent<CApplicationPowerHandling>();
    appPower->ResetScreenSaver();
    appPower->WakeUpScreenSaverAndDPMS();
  }
  else if (action->GetID() == ACTION_GESTURE_END &&
           !m_panPoints.empty()) // do we need to animate inertial scrolling?
  {
    // Calculate velocity in the last MAXIMUM_DELAY_FOR_INERTIA milliseconds.
    // Do not use the velocity given by the ACTION_GESTURE_END data - it is calculated
    // for the whole duration of the touch and thus useless for inertia. The user
    // may scroll around for a few seconds and then only at the end flick in one
    // direction. Only the last flick should be relevant here.
    auto velocitySum =
        std::accumulate(m_panPoints.cbegin(), m_panPoints.cend(), CVector{},
                        [](CVector val, PanPoint const& p) { return val + p.velocity; });
    auto velocityX = velocitySum.x / m_panPoints.size();
    auto velocityY = velocitySum.y / m_panPoints.size();

    m_timeToZero = TIME_TO_ZERO_SPEED;
    auto velocityMax = std::max(std::abs(velocityX), std::abs(velocityY));
#ifdef TARGET_DARWIN_OSX
    float dpiScale = 1.0;
#else
    float dpiScale = CGenericTouchInputHandler::GetInstance().GetScreenDPI() / 160.0f;
#endif
    if (velocityMax > MINIMUM_SPEED_FOR_INERTIA * dpiScale)
    {
      if (velocityMax < MAXIMUM_SPEED_FOR_REDUCTION * dpiScale)
        m_timeToZero = (m_timeToZero * velocityMax) / (MAXIMUM_SPEED_FOR_REDUCTION * dpiScale);

      bool inertialRequested = false;
      CGUIMessage message(GUI_MSG_GESTURE_NOTIFY, 0, 0, static_cast<int>(velocityX),
                          static_cast<int>(velocityY));

      // ask if the control wants inertial scrolling
      if (CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message))
      {
        int result = 0;
        if (message.GetPointer())
        {
          int* p = static_cast<int*>(message.GetPointer());
          message.SetPointer(nullptr);
          result = *p;
          delete p;
        }
        if (result == EVENT_RESULT_PAN_HORIZONTAL || result == EVENT_RESULT_PAN_VERTICAL)
        {
          inertialRequested = true;
        }
      }

      if (inertialRequested)
      {
        m_iFlickVelocity.x = velocityX; // in pixels per sec
        m_iFlickVelocity.y = velocityY; // in pixels per sec
        m_iLastGesturePoint.x = action->GetAmount(2); // last gesture point x
        m_iLastGesturePoint.y = action->GetAmount(3); // last gesture point y

        // calc deacceleration for fullstop in TIME_TO_ZERO_SPEED secs
        // v = a*t + v0 -> set v = 0 because we want to stop scrolling
        // a = -v0 / t
        m_inertialDeacceleration.x = -1 * m_iFlickVelocity.x / m_timeToZero;
        m_inertialDeacceleration.y = -1 * m_iFlickVelocity.y / m_timeToZero;

        m_inertialStartTime = CTimeUtils::GetFrameTime(); // start time of inertial scrolling
        ret = true;
        m_bScrolling = true; // activate the inertial scrolling animation
      }
    }
  }

  if (action->GetID() == ACTION_GESTURE_BEGIN || action->GetID() == ACTION_GESTURE_END ||
      action->GetID() == ACTION_GESTURE_ABORT)
  {
    m_panPoints.clear();
  }

  return ret;
}

bool CInertialScrollingHandler::ProcessInertialScroll(float frameTime)
{
  // do inertial scroll animation by sending gesture_pan
  if (m_bScrolling)
  {
    float xMovement = 0.0;
    float yMovement = 0.0;

    // decrease based on negative acceleration
    // calc the overall inertial scrolling time in secs
    float absoluteInertialTime = (CTimeUtils::GetFrameTime() - m_inertialStartTime) / (float)1000;

    // as long as we aren't over the overall inertial scroll time - do the deacceleration
    if (absoluteInertialTime < m_timeToZero)
    {
      // v = s/t -> s = t * v
      xMovement = frameTime * m_iFlickVelocity.x;
      yMovement = frameTime * m_iFlickVelocity.y;

      // save new gesture point
      m_iLastGesturePoint.x += xMovement;
      m_iLastGesturePoint.y += yMovement;

      // fire the pan action
      if (!g_application.OnAction(CAction(ACTION_GESTURE_PAN, 0, m_iLastGesturePoint.x,
                                          m_iLastGesturePoint.y, xMovement, yMovement,
                                          m_iFlickVelocity.x, m_iFlickVelocity.y)))
      {
        m_bAborting = true; // we are done
      }

      // calc new velocity based on deacceleration
      // v = a*t + v0
      m_iFlickVelocity.x = m_inertialDeacceleration.x * frameTime + m_iFlickVelocity.x;
      m_iFlickVelocity.y = m_inertialDeacceleration.y * frameTime + m_iFlickVelocity.y;

      // check if the signs are equal - which would mean we deaccelerated to long and reversed the
      // direction
      if ((m_inertialDeacceleration.x < 0) == (m_iFlickVelocity.x < 0))
      {
        m_iFlickVelocity.x = 0;
      }
      if ((m_inertialDeacceleration.y < 0) == (m_iFlickVelocity.y < 0))
      {
        m_iFlickVelocity.y = 0;
      }
    }
    else // no movement -> done
    {
      m_bAborting = true; // we are done
    }
  }

  // if we are done - or we where aborted
  if (m_bAborting)
  {
    // fire gesture end action
    g_application.OnAction(CAction(ACTION_GESTURE_END, 0, 0.0f, 0.0f, 0.0f, 0.0f));
    m_bAborting = false;
    m_bScrolling = false; // stop scrolling
    m_iFlickVelocity.x = 0;
    m_iFlickVelocity.y = 0;
  }

  return true;
}
