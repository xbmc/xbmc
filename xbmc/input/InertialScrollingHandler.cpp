/*
 *      Copyright (C) 2011-2012 Team XBMC
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


#include "threads/SystemClock.h"
#include "InertialScrollingHandler.h"
#include "Application.h"
#include "utils/TimeUtils.h"
#include "guilib/Key.h"
#include "guilib/GUIWindowManager.h"
#include "windowing/WindowingFactory.h"

#include <cmath>

//time for reaching velocitiy 0 in secs
#define TIME_TO_ZERO_SPEED 1.0f
//time for decreasing the deaccelleration (for doing a smooth stop) in secs
#define TIME_FOR_DEACELLERATION_DECREASE 0.5f
//the factor for decreasing the deacceleration
#define DEACELLERATION_DECREASE_FACTOR 0.9f
//minimum speed for doing inertial scroll is 100 pixels / s
#define MINIMUM_SPEED_FOR_INERTIA 100

CInertialScrollingHandler::CInertialScrollingHandler()
: m_bScrolling(false)
, m_bAborting(false)
, m_iFlickVelocity(CPoint(0,0))
, m_iLastGesturePoint(CPoint(0,0))
, m_inertialDeacceleration(CPoint(0,0))
, m_inertialStartTime(0)
{
}

bool CInertialScrollingHandler::CheckForInertialScrolling(const CAction* action)
{
  bool ret = false;//return value - false no inertial scrolling - true - inertial scrolling

  if(g_Windowing.HasInertialGestures())
  {
    return ret;//no need for emulating inertial scrolling - windowing does support it nativly.
  }

  //reset screensaver during pan
  if( action->GetID() == ACTION_GESTURE_PAN )
  {
    g_application.ResetScreenSaver();
    return false;
  }

  //mouse click aborts scrolling
  if( m_bScrolling && action->GetID() == ACTION_MOUSE_LEFT_CLICK )
  {
    ret = true;
    m_bAborting = true;//lets abort
  }

  //on begin/tap stop all inertial scrolling
  if ( action->GetID() == ACTION_GESTURE_BEGIN )
  {
    //release any former exclusive mouse mode
    //for making switching between multiple lists
    //possible
    CGUIMessage message(GUI_MSG_EXCLUSIVE_MOUSE, 0, 0);
    g_windowManager.SendMessage(message);     
    m_bScrolling = false;
    //wakeup screensaver on pan begin
    g_application.ResetScreenSaver();    
    g_application.WakeUpScreenSaverAndDPMS();    
  }
  else//do we need to animate inertial scrolling?
  {
    if (action->GetID() == ACTION_GESTURE_END && ( fabs(action->GetAmount(0)) > MINIMUM_SPEED_FOR_INERTIA || fabs(action->GetAmount(1)) > MINIMUM_SPEED_FOR_INERTIA ) )
    {
      bool inertialRequested = false;
      CGUIMessage message(GUI_MSG_GESTURE_NOTIFY, 0, 0, (int)action->GetAmount(2), (int)action->GetAmount(3));

      //ask if the control wants inertial scrolling
      if(g_windowManager.SendMessage(message))
      {
        if( message.GetParam1() == EVENT_RESULT_PAN_HORIZONTAL ||
            message.GetParam1() == EVENT_RESULT_PAN_VERTICAL)
        {
          inertialRequested = true;
        }
      }

      if( inertialRequested )                                                                                                             
      {        
        m_iFlickVelocity.x = action->GetAmount(0)/2;//in pixels per sec
        m_iFlickVelocity.y = action->GetAmount(1)/2;//in pixels per sec     
        m_iLastGesturePoint.x = action->GetAmount(2);//last gesture point x
        m_iLastGesturePoint.y = action->GetAmount(3);//last gesture point y

        //calc deacceleration for fullstop in TIME_TO_ZERO_SPEED secs
        //v = a*t + v0 -> set v = 0 because we want to stop scrolling
        //a = -v0 / t
        m_inertialDeacceleration.x = -1*m_iFlickVelocity.x/TIME_TO_ZERO_SPEED;    
        m_inertialDeacceleration.y = -1*m_iFlickVelocity.y/TIME_TO_ZERO_SPEED;

        //CLog::Log(LOGDEBUG, "initial vel: %f dec: %f", m_iFlickVelocity.y, m_inertialDeacceleration.y);
        m_inertialStartTime = CTimeUtils::GetFrameTime();//start time of inertial scrolling
        ret = true;
        m_bScrolling = true;//activate the inertial scrolling animation
      }
    }
  }
  return ret;
}

bool CInertialScrollingHandler::ProcessInertialScroll(float frameTime)
{  
  //do inertial scroll animation by sending gesture_pan
  if( m_bScrolling)
  {
    float yMovement = 0.0;
    float xMovement = 0.0;

    //decrease based on negativ acceleration
    //calc the overall inertial scrolling time in secs
    float absolutInertialTime = (CTimeUtils::GetFrameTime() - m_inertialStartTime)/(float)1000;

    //as long as we aren't over the overall inertial scroll time - do the deacceleration
    if ( absolutInertialTime < TIME_TO_ZERO_SPEED + TIME_FOR_DEACELLERATION_DECREASE )
    {
      //v = s/t -> s = t * v
      yMovement = frameTime * m_iFlickVelocity.y;
      xMovement = frameTime * m_iFlickVelocity.x;

      //calc new velocity based on deacceleration
      //v = a*t + v0
      m_iFlickVelocity.y = m_inertialDeacceleration.y * frameTime + m_iFlickVelocity.y;
      m_iFlickVelocity.x = m_inertialDeacceleration.x * frameTime + m_iFlickVelocity.x;      

      //CLog::Log(LOGDEBUG,"velocity: %f dec: %f time: %f", m_iFlickVelocity.y, m_inertialDeacceleration.y, absolutInertialTime);      

      //check if the signs are equal - which would mean we deaccelerated to long and reversed the direction
      if( (m_inertialDeacceleration.y < 0) == (m_iFlickVelocity.y < 0) )
      {
        m_iFlickVelocity.y = 0;
      }

      if( (m_inertialDeacceleration.x < 0) == (m_iFlickVelocity.x < 0) )
      {
        m_iFlickVelocity.x = 0;
      }      

      //did we scroll long enought for decrease the deacceleration?
      if( absolutInertialTime > TIME_TO_ZERO_SPEED - TIME_FOR_DEACELLERATION_DECREASE )
      {
        //decrease deacceleration by deacceleration decrease factor
        m_inertialDeacceleration.y*=DEACELLERATION_DECREASE_FACTOR;
        m_inertialDeacceleration.x *= DEACELLERATION_DECREASE_FACTOR;                          
      }         
    }

    //if we have movement
    if( xMovement || yMovement )
    {
      //fire the pan action
      g_application.OnAction(CAction(ACTION_GESTURE_PAN, 0, m_iLastGesturePoint.x, m_iLastGesturePoint.y, xMovement, yMovement));
      //save new gesture point
      m_iLastGesturePoint.y += yMovement;
      m_iLastGesturePoint.x += xMovement;      
    }   
    else//no movement -> done
    {
      m_bAborting = true;//we are done
    }
  }

  //if we are done - or we where aborted
  if( m_bAborting )
  {
    //fire gesture end action
    g_application.OnAction(CAction(ACTION_GESTURE_END, 0, (float)0.0, (float)0.0, (float)0.0, (float)0.0));
    m_bAborting = false;
    m_bScrolling = false; //stop scrolling
    m_iFlickVelocity.x = 0;
    m_iFlickVelocity.y = 0;      
  }

  return true;
}
