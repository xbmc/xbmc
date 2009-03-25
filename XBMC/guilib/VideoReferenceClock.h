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
    void GetTime(LARGE_INTEGER *ptime);
    CVideoReferenceClock();
    void SetSpeed(double Speed);
    double GetSpeed();
    int GetRefreshRate();
    
  protected:
    void OnStartup();
    LARGE_INTEGER CurrTime;
    LARGE_INTEGER LastRefreshTime;
    LARGE_INTEGER SystemFrequency;
    LARGE_INTEGER AdjustedFrequency;
    bool UseVblank;
    __int64 RefreshRate;
    int PrevRefreshRate;
    bool UpdateRefreshrate();
    
#ifdef HAS_GLX
    Surface::Display* Dpy;
    int Screen;
    bool SetupGLX();
    void RunGLX();
    int (*p_glXGetVideoSyncSGI)(unsigned int*);
#endif
};

extern CVideoReferenceClock g_VideoReferenceClock;