/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <deque>

#include "utils/Geometry.h"
#include "utils/Vector.h"

class CApplication;
class CAction;

class CInertialScrollingHandler
{
  friend class CApplication;
  public:
    CInertialScrollingHandler();

    bool IsScrolling(){return m_bScrolling;}

  private:
    bool CheckForInertialScrolling(const CAction* action);
    bool ProcessInertialScroll(float frameTime);

    //-------------------------------------------vars for inertial scrolling animation with gestures
    bool          m_bScrolling = false;        //flag indicating that we currently do the inertial scrolling emulation
    bool          m_bAborting = false;         //flag indicating an abort of scrolling
    CVector       m_iFlickVelocity;

    struct PanPoint
    {
      unsigned int time;
      CVector velocity;
      PanPoint(unsigned int time, CVector velocity)
        : time(time), velocity(velocity) {}
      unsigned int TimeElapsed() const;
    };
    std::deque<PanPoint> m_panPoints;
    CPoint        m_iLastGesturePoint;
    CVector       m_inertialDeacceleration;
    unsigned int  m_inertialStartTime = 0;
};
