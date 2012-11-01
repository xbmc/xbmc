#pragma once

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

#include "guilib/Geometry.h"

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
    bool          m_bScrolling;        //flag indicating that we currently do the inertial scrolling emulation
    bool          m_bAborting;         //flag indicating an abort of scrolling
    CPoint        m_iFlickVelocity;
    CPoint        m_iLastGesturePoint;
    CPoint        m_inertialDeacceleration;
    unsigned int  m_inertialStartTime;  
};
