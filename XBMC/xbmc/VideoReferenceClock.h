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

#if defined(HAS_GLX) && defined(HAS_XRANDR)
  #include <X11/X.h>
  #include <X11/Xlib.h>
  #include <GL/glx.h>
#elif defined(_WIN32)
  #ifdef _DEBUG
    #define D3D_DEBUG_INFO
  #endif
  #include <d3d9.h>
  #if(DIRECT3D_VERSION > 0x0900)
    #include <Dxerr.h>
  #else
    #include <dxerr9.h>
    #define DXGetErrorString(hr)      DXGetErrorString9(hr)
    #define DXGetErrorDescription(hr) DXGetErrorDescription9(hr)
  #endif
#endif

class CVideoReferenceClock : public CThread
{
  public:
    CVideoReferenceClock();

    void    GetTime(LARGE_INTEGER *ptime);
    void    GetFrequency(LARGE_INTEGER *pfreq);
    void    SetSpeed(double Speed);
    double  GetSpeed();
    int     GetRefreshRate();
    int64_t Wait(int64_t Target);
    bool    WaitStarted(int MSecs);
    bool    GetClockInfo(int& MissedVblanks, double& ClockSpeed, int& RefreshRate);

#ifdef _WIN32
    void SetMonitor(MONITORINFOEX &Monitor);
#elif defined(__APPLE__)
    void VblankHandler(int64_t nowtime, double fps);
#endif
    
  private:
    void Process();
    bool UpdateRefreshrate(bool Forced = false);
    void SendVblankSignal();
    void UpdateClock(int NrVBlanks, bool CheckMissed);

    int64_t m_CurrTime;          //the current time of the clock when using vblank as clock source
    int64_t m_AdjustedFrequency; //the frequency of the clock set by dvdplayer
    int64_t m_ClockOffset;       //the difference between the vblank clock and systemclock, set when vblank clock is stopped
    int64_t m_LastRefreshTime;   //last time we updated the refreshrate
    int64_t m_SystemFrequency;   //frequency of the systemclock

    bool    m_UseVblank;         //set to true when vblank is used as clock source
    int64_t m_RefreshRate;       //current refreshrate
    int     m_PrevRefreshRate;   //previous refresrate, used for log printing and getting refreshrate from nvidia-settings
    int     m_MissedVblanks;     //number of clock updates missed by the vblank clock
    int     m_TotalMissedVblanks;//total number of clock updates missed, used by codec information screen
    int64_t m_VblankTime;        //last time the clock was updated when using vblank as clock

    CEvent m_Started;            //set when the vblank clock is started
    CEvent m_VblankEvent;        //set when a vblank happens

    CCriticalSection m_CritSection;

#if defined(HAS_GLX) && defined(HAS_XRANDR)
    bool SetupGLX();
    void RunGLX();
    void CleanupGLX();
    bool ParseNvSettings(int& RefreshRate);

    int  (*m_glXWaitVideoSyncSGI)(int, int, unsigned int*);
    int  (*m_glXGetVideoSyncSGI)(unsigned int*);

    Display*     m_Dpy;
    XVisualInfo *m_vInfo;
    Window       m_Window;
    GLXContext   m_Context;

    bool     m_UseNvSettings;

#elif defined(_WIN32)
    bool   CreateHiddenWindow();
    bool   SetupD3D();
    double MeasureRefreshrate(int MSecs);
    void   RunD3D();
    void   CleanupD3D();
    void   HandleWindowMessages();

    LPDIRECT3D9       m_D3d;
    LPDIRECT3DDEVICE9 m_D3dDev;

    HWND          m_Hwnd;
    WNDCLASSEX    m_WinCl;
    bool          m_HasWinCl;
    unsigned int  m_Width;
    unsigned int  m_Height;
    unsigned int  m_Adapter;
    MONITORINFOEX m_Monitor;
    MONITORINFOEX m_PrevMonitor;
    bool          m_IsVista;

#elif defined(__APPLE__)
    bool SetupCocoa();
    void RunCocoa();
    void CleanupCocoa();
    
    int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
                               //not the same as m_VblankTime
#endif
};

extern CVideoReferenceClock g_VideoReferenceClock;
