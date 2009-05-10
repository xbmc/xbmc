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

#ifdef HAS_GLX
  #include <X11/X.h>
  #include <X11/Xlib.h>
  #include <GL/glx.h>
#elif defined(_WIN32)
  namespace D3dClock
  {
    #include <d3d9.h>
  }
#endif

class CVideoReferenceClock : public CThread
{
  public:
    CVideoReferenceClock();
    ~CVideoReferenceClock();
    
    void   GetTime(LARGE_INTEGER *ptime);
    void   GetFrequency(LARGE_INTEGER *pfreq);
    void   SetSpeed(double Speed);
    double GetSpeed();
    int    GetRefreshRate();
    void   Wait(__int64 Target);
    void   WaitStarted(int MSecs);

  protected:
    void Process();
    bool UpdateRefreshrate(bool Forced = false);
    void Lock();
    void Unlock();
    void SendVblankSignal();
    void UpdateClock(int NrVBlanks, bool CheckMissed);
    
    __int64 m_CurrTime;
    __int64 m_AdjustedFrequency;
    __int64 m_ClockOffset;
    __int64 m_LastRefreshTime;
    __int64 m_SystemFrequency;
    
    bool    m_UseVblank;
    __int64 m_RefreshRate;
    int     m_PrevRefreshRate;
    int     m_MissedVblanks;
    __int64 m_VblankTime;
    
    CEvent m_Started;

#ifdef HAS_SDL
    SDL_cond*  m_VblankCond;
    SDL_mutex* m_VblankMutex;
#endif
    
#ifdef HAS_GLX
    bool SetupGLX();
    void RunGLX();
    void CleanupGLX();
    int  (*m_glXWaitVideoSyncSGI)(int, int, unsigned int*);
    int  (*m_glXGetVideoSyncSGI)(unsigned int*);
    
    Display*     m_Dpy;
    GLXFBConfig *m_fbConfigs;
    XVisualInfo *m_vInfo;
    Window       m_Window;
    GLXWindow    m_GLXWindow;
    GLXContext   m_Context;
            
    bool     m_UseNvSettings;
#elif defined(_WIN32)
    bool CreateHiddenWindow();
    bool SetupD3D();
    void RunD3D();
    void CleanupD3D();
    void HandleWindowMessages();

    D3dClock::LPDIRECT3D9       m_D3d;
    D3dClock::LPDIRECT3DDEVICE9 m_D3dDev;

    HWND         m_Hwnd;
    WNDCLASSEX   m_WinCl;
    bool         m_HasWinCl;
    unsigned int m_Width;
    unsigned int m_Height;
    unsigned int m_Adapter;
#endif
};

extern CVideoReferenceClock g_VideoReferenceClock;
