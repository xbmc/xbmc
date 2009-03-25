#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "stdafx.h"
#include "Thread.h"
#include "Surface.h"

class CVideoReferenceClock : public CThread
{
  public:
    CVideoReferenceClock();
    
    void   GetTime(LARGE_INTEGER *ptime);
    void   SetSpeed(double Speed);
    double GetSpeed();
    int    GetRefreshRate();
    
  protected:
    void OnStartup();
    bool UpdateRefreshrate();
    
    LARGE_INTEGER m_CurrTime;
    LARGE_INTEGER m_LastRefreshTime;
    LARGE_INTEGER m_SystemFrequency;
    LARGE_INTEGER m_AdjustedFrequency;
    
    bool    m_UseVblank;
    __int64 m_RefreshRate;
    int     m_PrevRefreshRate;
    
#ifdef HAS_GLX
    bool SetupGLX();
    void RunGLX();
    int  (*m_glXGetVideoSyncSGI)(unsigned int*);
    
    Surface::Display* m_Dpy;
    int               m_Screen;
    bool              m_UseNvSettings;
#endif
};

extern CVideoReferenceClock g_VideoReferenceClock;