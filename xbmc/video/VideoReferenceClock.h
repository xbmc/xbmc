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

#include "system.h" // for HAS_XRANDR, and Win32 types
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

//TODO: get rid of #ifdef hell, abstract implementations in separate classes

#if defined(HAS_GLX) && defined(HAS_XRANDR)
  #include "system_gl.h"
  #include <X11/X.h>
  #include <X11/Xlib.h>
  #include <GL/glx.h>
  #include "guilib/DispResource.h"
#elif defined(_WIN32) && defined(HAS_DX)
  #include <d3d9.h>
  #include "guilib/D3DResource.h"

class CD3DCallback : public ID3DResource
{
  public:
    void Reset();
    void OnDestroyDevice();
    void OnCreateDevice();
    void Aquire();
    void Release();
    bool IsValid();

  private:
    bool m_devicevalid;
    bool m_deviceused;

    CCriticalSection m_critsection;
    CEvent           m_createevent;
    CEvent           m_releaseevent;
};

#endif

class CVideoReferenceClock : public CThread
#if defined(HAS_GLX)
                            ,public IDispResource
#endif
{
  public:
    CVideoReferenceClock();
    virtual ~CVideoReferenceClock();

    int64_t GetTime(bool interpolated = true);
    int64_t GetFrequency();
    void    SetSpeed(double Speed);
    double  GetSpeed();
    int     GetRefreshRate(double* interval = NULL);
    int64_t Wait(int64_t Target);
    bool    WaitStarted(int MSecs);
    bool    GetClockInfo(int& MissedVblanks, double& ClockSpeed, int& RefreshRate);
    void    SetFineAdjust(double fineadjust);
    void    RefreshChanged() { m_RefreshChanged = 1; }

#if defined(TARGET_DARWIN)
    void VblankHandler(int64_t nowtime, double fps);
#endif

#if defined(HAS_GLX)
    virtual void OnLostDevice();
    virtual void OnResetDevice();
#endif

  private:
    void    Process();
    bool    UpdateRefreshrate(bool Forced = false);
    void    SendVblankSignal();
    void    UpdateClock(int NrVBlanks, bool CheckMissed);
    double  UpdateInterval();
    int64_t TimeOfNextVblank();

    int64_t m_CurrTime;          //the current time of the clock when using vblank as clock source
    int64_t m_LastIntTime;       //last interpolated clock value, to make sure the clock doesn't go backwards
    double  m_CurrTimeFract;     //fractional part that is lost due to rounding when updating the clock
    double  m_ClockSpeed;        //the frequency of the clock set by dvdplayer
    int64_t m_ClockOffset;       //the difference between the vblank clock and systemclock, set when vblank clock is stopped
    int64_t m_LastRefreshTime;   //last time we updated the refreshrate
    int64_t m_SystemFrequency;   //frequency of the systemclock
    double  m_fineadjust;

    bool    m_UseVblank;         //set to true when vblank is used as clock source
    int64_t m_RefreshRate;       //current refreshrate
    int     m_PrevRefreshRate;   //previous refreshrate, used for log printing and getting refreshrate from nvidia-settings
    int     m_MissedVblanks;     //number of clock updates missed by the vblank clock
    int     m_RefreshChanged;    //1 = we changed the refreshrate, 2 = we should check the refreshrate forced
    int     m_TotalMissedVblanks;//total number of clock updates missed, used by codec information screen
    int64_t m_VblankTime;        //last time the clock was updated when using vblank as clock

    CEvent  m_Started;            //set when the vblank clock is started
    CEvent  m_VblankEvent;        //set when a vblank happens

    CCriticalSection m_CritSection;

#if defined(HAS_GLX) && defined(HAS_XRANDR)
    bool SetupGLX();
    void RunGLX();
    void CleanupGLX();
    bool ParseNvSettings(int& RefreshRate);
    int  GetRandRRate();

    int  (*m_glXWaitVideoSyncSGI) (int, int, unsigned int*);
    int  (*m_glXGetVideoSyncSGI)  (unsigned int*);

    Display*     m_Dpy;
    XVisualInfo *m_vInfo;
    Window       m_Window;
    GLXContext   m_Context;
    Pixmap       m_pixmap;
    GLXPixmap    m_glPixmap;
    bool         m_xrrEvent;
    CEvent       m_releaseEvent, m_resetEvent;

    bool         m_UseNvSettings;
    bool         m_bIsATI;

#elif defined(_WIN32) && defined(HAS_DX)
    bool   SetupD3D();
    double MeasureRefreshrate(int MSecs);
    void   RunD3D();
    void   CleanupD3D();

    LPDIRECT3DDEVICE9 m_D3dDev;
    CD3DCallback      m_D3dCallback;

    unsigned int  m_Width;
    unsigned int  m_Height;
    bool          m_Interlaced;

#elif defined(TARGET_DARWIN)
    bool SetupCocoa();
    void RunCocoa();
    void CleanupCocoa();

    int64_t m_LastVBlankTime;  //timestamp of the last vblank, used for calculating how many vblanks happened
                               //not the same as m_VblankTime
#endif
};

extern CVideoReferenceClock g_VideoReferenceClock;
