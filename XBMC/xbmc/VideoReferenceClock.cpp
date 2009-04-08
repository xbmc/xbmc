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
#include <iostream> //for debugging, please remove
#include "VideoReferenceClock.h"
#include "Util.h"

#ifdef HAS_GLX
  #include <X11/extensions/Xrandr.h>
  #define NVSETTINGSCMD "nvidia-settings -nt -q RefreshRate"
#endif

using namespace std;

CVideoReferenceClock::CVideoReferenceClock()
{
  QueryPerformanceFrequency(&m_SystemFrequency);
  m_AdjustedFrequency = m_SystemFrequency;
  m_PrevAdjustedFrequency = m_SystemFrequency;
  m_ClockOffset.QuadPart = 0;
  m_UseVblank = false;

#ifdef HAS_SDL
  m_VblankCond = SDL_CreateCond();
  m_VblankMutex = SDL_CreateMutex();
#endif
}

CVideoReferenceClock::~CVideoReferenceClock()
{
#ifdef HAS_SDL
  SDL_DestroyCond(m_VblankCond);
  SDL_DestroyMutex(m_VblankMutex);
#endif
}

void CVideoReferenceClock::OnStartup()
{
  bool PrevSetupSuccess = true;
  bool SetupSuccess;
  LARGE_INTEGER Now;
  
  QueryPerformanceCounter(&m_CurrTime);
  m_CurrTime.QuadPart -= m_ClockOffset.QuadPart;

  while(!m_bStop)
  {
#ifdef HAS_GLX
    SetupSuccess = SetupGLX();
#elif defined(_WIN32)
    SetupSuccess = SetupD3D();
#endif
    if (SetupSuccess)
    {
      m_UseVblank = true;
#ifdef HAS_GLX
      RunGLX();
#elif defined(_WIN32)
      RunD3D();
#endif
    }
    else if (!SetupSuccess && !PrevSetupSuccess)
    {
      Lock();
      m_UseVblank = false;
      QueryPerformanceCounter(&Now);
      m_ClockOffset.QuadPart = Now.QuadPart - m_CurrTime.QuadPart;
      SendVblankSignal();
      Unlock();
      
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setup failed twice in a row, falling back to QueryPerformanceCounter");
      break;
    }
    PrevSetupSuccess = SetupSuccess;
  }
  if (SetupSuccess)
  {
#ifdef HAS_GLX
    CleanupGLX();
#elif defined(_WIN32)
    CleanupD3D();
#endif
  }
}

#ifdef HAS_GLX
bool CVideoReferenceClock::SetupGLX()
{
  int Num = 0, ReturnV, Depth;
  unsigned int VblankCount;
  int singleVisAttributes[] =
  {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
    None
  };
  Visual *Visual;
  
  m_Dpy = NULL;
  m_fbConfigs = NULL;
  m_vInfo = NULL;
  m_Context = NULL;
  m_Pxmp = NULL;
  m_GLXPxmp = NULL;
  
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setting up GLX");
  
  m_Dpy = XOpenDisplay(NULL);
  if (!m_Dpy)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to open display");
    CleanupGLX();
    return false;
  }
  
  m_fbConfigs = glXChooseFBConfig(m_Dpy, DefaultScreen(m_Dpy), singleVisAttributes, &Num);
  if (!m_fbConfigs)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXChooseFBConfig returned NULL");
    CleanupGLX();
    return false;
  }
  
  m_vInfo = glXGetVisualFromFBConfig(m_Dpy, m_fbConfigs[0]);
  if (!m_vInfo)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVisualFromFBConfig returned NULL");
    CleanupGLX();
    return false;
  }
  
  m_Screen = DefaultScreen(m_Dpy);
  Visual = DefaultVisual(m_Dpy, m_Screen);
  Depth = DefaultDepth(m_Dpy, m_Screen); 

  m_Pxmp = XCreatePixmap(m_Dpy, RootWindow(m_Dpy, m_Screen), 64, 64, Depth);
  m_GLXPxmp = glXCreateGLXPixmap(m_Dpy, m_vInfo, m_Pxmp);
  m_Context = glXCreateNewContext(m_Dpy, m_fbConfigs[0], GLX_RGBA_TYPE, NULL, true);
  glXMakeCurrent(m_Dpy, m_GLXPxmp, m_Context);
  
  m_glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  if (!m_glXWaitVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI not found");
    CleanupGLX();
    return false;
  }
  
  ReturnV = m_glXWaitVideoSyncSGI(2, 0, &VblankCount);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
    CleanupGLX();
    return false;
  }
  
  m_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!m_glXWaitVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI not found");
    CleanupGLX();
    return false;
  }
  
  ReturnV = m_glXGetVideoSyncSGI(&VblankCount);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI returned %i", ReturnV);
    CleanupGLX();
    return false;
  }
  
  XRRSizes(m_Dpy, m_Screen, &ReturnV);
  if (ReturnV == 0)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: RandR not supported");
    CleanupGLX();
    return false;
  }
  
  float fRefreshRate;
  m_UseNvSettings = false;
  FILE* NvSettings = popen(NVSETTINGSCMD, "r");
  if (NvSettings)
  {
    if (fscanf(NvSettings, "%f", &fRefreshRate) == 1)
    {
      if (fRefreshRate > 0.0)
      {
        m_UseNvSettings = true;
      }
    }
    pclose(NvSettings);
  }
  
  if (m_UseNvSettings)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Using %s for refreshrate detection", NVSETTINGSCMD);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Using RandR for refreshrate detection");
  }
  
  m_PrevRefreshRate = -1;
  m_LastRefreshTime.QuadPart = 0;
  UpdateRefreshrate();
  
  return true;
}

void CVideoReferenceClock::CleanupGLX()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cleaning up GLX");
  
  if (m_fbConfigs)
  {
    XFree(m_fbConfigs);
    m_fbConfigs = NULL;
  }
  if (m_vInfo)
  {
    XFree(m_vInfo);
    m_vInfo = NULL;
  }
  if (m_Context)
  {
    glXDestroyContext(m_Dpy, m_Context);
    m_Context = NULL;
  }
  if (m_GLXPxmp)
  {
    glXDestroyGLXPixmap(m_Dpy, m_GLXPxmp);
    m_GLXPxmp = NULL;
  }
  if (m_Pxmp)
  {
    XFreePixmap(m_Dpy, m_Pxmp);
    m_Pxmp = NULL;
  }
  if (m_Dpy)
  {
    XCloseDisplay(m_Dpy);
    m_Dpy = NULL;
  }
}

void CVideoReferenceClock::RunGLX()
{
  unsigned int  PrevVblankCount;
  unsigned int  VblankCount;
  int           ReturnV;
  
  m_glXGetVideoSyncSGI(&PrevVblankCount);
  UpdateRefreshrate();
  
  while(!m_bStop)
  {
    ReturnV = m_glXWaitVideoSyncSGI(2, ((PrevVblankCount % 2) + 1) % 2, &VblankCount);
    if(ReturnV)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
      CleanupGLX();
      return;
    }
    
    if (VblankCount > PrevVblankCount)
    {
      Lock();
      m_CurrTime.QuadPart += (__int64)(VblankCount - PrevVblankCount) * m_AdjustedFrequency.QuadPart / m_RefreshRate;
      SendVblankSignal();
      Unlock();
      
      UpdateRefreshrate();
    }
    else
    {
      m_glXGetVideoSyncSGI(&VblankCount);
      if (VblankCount <= PrevVblankCount)
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: Vblank counter has reset");
        
        Lock();
        m_CurrTime.QuadPart += m_AdjustedFrequency.QuadPart / m_RefreshRate;
        SendVblankSignal();
        Unlock();
        
        glXMakeCurrent(m_Dpy, None, NULL);
        glXMakeCurrent(m_Dpy, m_GLXPxmp, m_Context);
      }        
    }
    PrevVblankCount = VblankCount;
  }
}
#elif defined(_WIN32)

#define CLASSNAME "videoreferenceclock"

void CVideoReferenceClock::RunD3D()
{
  D3dClock::D3DRASTER_STATUS RasterStatus;
  LARGE_INTEGER              CurrVBlankTime;
  LARGE_INTEGER              LastVBlankTime;

  unsigned int LastLine;
  int          NrVBlanks;
  double       VBlankTime;
  int          ReturnV;
  int          SleepCount = 0;

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  m_D3dDev->GetRasterStatus(0, &RasterStatus);

  if (RasterStatus.InVBlank) LastLine = 0;
  else LastLine = RasterStatus.ScanLine;

  QueryPerformanceCounter(&LastVBlankTime);

  while(!m_bStop)
  {
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);

    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned %i", ReturnV & 0xFFFF);
      return;
    }

    if (RasterStatus.InVBlank || (RasterStatus.ScanLine < LastLine))
    {
      QueryPerformanceCounter(&CurrVBlankTime);
      VBlankTime = (double)(CurrVBlankTime.QuadPart - LastVBlankTime.QuadPart) / (double)m_SystemFrequency.QuadPart;
      NrVBlanks = MathUtils::round_int(VBlankTime * (double)m_RefreshRate);

      Lock();
      m_CurrTime.QuadPart += (__int64)NrVBlanks * m_AdjustedFrequency.QuadPart / m_RefreshRate;
      SendVblankSignal();
      Unlock();
      
      LastVBlankTime = CurrVBlankTime;
      SleepCount = 0;
      
      if (UpdateRefreshrate())
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: Displaymode changed");
        CleanupD3D();
        return;
      }
      HandleWindowMessages();
    }
    if (RasterStatus.InVBlank) LastLine = 0;
    else LastLine = RasterStatus.ScanLine;

    Sleep(1);
    SleepCount++;
    if (SleepCount >= 1000)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus is not responding");
      CleanupD3D();
      return;
    }
  }
}

bool CVideoReferenceClock::SetupD3D()
{
  D3dClock::D3DPRESENT_PARAMETERS D3dPP;
  D3dClock::D3DCAPS9              DevCaps;
  D3dClock::D3DRASTER_STATUS      RasterStatus;
  D3dClock::D3DDISPLAYMODE        DisplayMode;

  int ReturnV;

  m_D3d = NULL;
  m_D3dDev = NULL;
  m_Hwnd = NULL;
  m_HasWinCl = false;

  m_Adapter = 0;
  if (getenv("SDL_FULLSCREEN_HEAD"))
  {
    unsigned int Adapter;
    if (sscanf(getenv("SDL_FULLSCREEN_HEAD"), "%u", &Adapter) == 1)
    {
      m_Adapter = Adapter - 1;
    }
  }

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setting up Direct3d on adapter %i", m_Adapter);
  
  if (!CreateHiddenWindow())
  {
    CleanupD3D();
    return false;
  }

  m_D3d = D3dClock::Direct3DCreate9(D3D_SDK_VERSION);

  if (!m_D3d)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Direct3DCreate9 failed");
    CleanupD3D();
    return false;
  }

  ZeroMemory(&D3dPP, sizeof(D3dPP));
  D3dPP.Windowed = TRUE;
  D3dPP.SwapEffect = D3dClock::D3DSWAPEFFECT_DISCARD;
  D3dPP.hDeviceWindow = m_Hwnd;
  D3dPP.BackBufferWidth = 64;
  D3dPP.BackBufferHeight = 64;
  D3dPP.BackBufferFormat = D3dClock::D3DFMT_UNKNOWN;
  D3dPP.BackBufferCount = 1;
  D3dPP.MultiSampleType = D3dClock::D3DMULTISAMPLE_NONE;
  D3dPP.MultiSampleQuality = 0;
  D3dPP.SwapEffect = D3dClock::D3DSWAPEFFECT_FLIP;
  D3dPP.EnableAutoDepthStencil = FALSE;
  D3dPP.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

  ReturnV = m_D3d->CreateDevice(m_Adapter, D3dClock::D3DDEVTYPE_HAL, m_Hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3dPP, &m_D3dDev);

  if (ReturnV != D3D_OK && ReturnV != D3DERR_DEVICELOST)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: CreateDevice returned %i", ReturnV & 0xFFFF);
    CleanupD3D();
    return false;
  }
  else if (ReturnV == D3DERR_DEVICELOST)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: CreateDevice returned D3DERR_DEVICELOST, resetting device");
    ReturnV = m_D3dDev->Reset(&D3dPP);
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Reset returned %i", ReturnV & 0xFFFF);
      CleanupD3D();
      return false;
    }
  }

  ReturnV = m_D3dDev->GetDeviceCaps(&DevCaps);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetDeviceCaps returned %i",
              ReturnV & 0xFFFF);
    CleanupD3D();
    return false;
  }

  if (DevCaps.Caps != D3DCAPS_READ_SCANLINE)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Hardware does not support GetRasterStatus");
    CleanupD3D();
    return false;
  }

  ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned %i",
              ReturnV & 0xFFFF);
    CleanupD3D();
    return false;
  }

  ReturnV = m_D3d->GetAdapterDisplayMode(m_Adapter, &DisplayMode);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetAdapterDisplayMode returned %i",
              ReturnV & 0xFFFF);
    CleanupD3D();
    return false;
  }

  m_PrevRefreshRate = -1;
  m_LastRefreshTime.QuadPart = 0;
  m_Width = 0;
  m_Height = 0;
  UpdateRefreshrate();

  return true;
}

void CVideoReferenceClock::CleanupD3D()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cleaning up Direct3d");
  if (m_D3dDev)
  {
    m_D3dDev->Release();
    m_D3dDev = NULL;
  }
  if (m_D3d)
  {
    m_D3d->Release();
    m_D3d = NULL;
  }
  if (m_Hwnd)
  {
    DestroyWindow(m_Hwnd);
    m_Hwnd = NULL;
  }
  if (m_HasWinCl)
  {
    UnregisterClass(CLASSNAME, GetModuleHandle(NULL));
    m_HasWinCl = false;
  }
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProc (hwnd, message, wParam, lParam);
}

bool CVideoReferenceClock::CreateHiddenWindow()
{
  m_WinCl.hInstance = GetModuleHandle(NULL);
  m_WinCl.lpszClassName = CLASSNAME;
  m_WinCl.lpfnWndProc = WindowProcedure;
  m_WinCl.style = CS_DBLCLKS;          
  m_WinCl.cbSize = sizeof(WNDCLASSEX);
  m_WinCl.hIcon = NULL;
  m_WinCl.hIconSm = NULL;
  m_WinCl.hCursor = LoadCursor(NULL, IDC_ARROW);
  m_WinCl.lpszMenuName = NULL;
  m_WinCl.cbClsExtra = 0;
  m_WinCl.cbWndExtra = 0;
  m_WinCl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

  if (!RegisterClassEx (&m_WinCl))
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to register window class");
    return false;
  }
  m_HasWinCl = true;
  
  m_Hwnd = CreateWindowEx(WS_EX_LEFT|WS_EX_LTRREADING|WS_EX_WINDOWEDGE, m_WinCl.lpszClassName, m_WinCl.lpszClassName,
                          WS_OVERLAPPED|WS_MINIMIZEBOX|WS_SYSMENU|WS_CLIPSIBLINGS|WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT,
                          400, 430, HWND_DESKTOP, NULL, m_WinCl.hInstance, NULL);

  if (!m_Hwnd)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: CreateWindowEx failed");
    return false;
  }

  ShowWindow(m_Hwnd, SW_HIDE);
  UpdateWindow(m_Hwnd);
  HandleWindowMessages();

  return true;
}

void CVideoReferenceClock::HandleWindowMessages()
{
  MSG Message;

  while(PeekMessage(&Message, m_Hwnd, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&Message); 
    DispatchMessage(&Message);
  }
}

#endif /*_WIN32*/

void CVideoReferenceClock::GetTime(LARGE_INTEGER *ptime)
{
  if (m_UseVblank)
  {
    *ptime = m_CurrTime;
  }
  else
  {
    QueryPerformanceCounter(ptime);
    ptime->QuadPart -= m_ClockOffset.QuadPart;
  }
}

void CVideoReferenceClock::GetFrequency(LARGE_INTEGER *pfreq)
{
  *pfreq = m_SystemFrequency;
}

void CVideoReferenceClock::SetSpeed(double Speed)
{
  if (m_UseVblank)
  {
    m_AdjustedFrequency.QuadPart = (__int64)((double)m_SystemFrequency.QuadPart * Speed);
    if (m_AdjustedFrequency.QuadPart != m_PrevAdjustedFrequency.QuadPart)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Clock speed %f%%", GetSpeed() * 100);
      m_PrevAdjustedFrequency = m_AdjustedFrequency;
    }
  }
}

double CVideoReferenceClock::GetSpeed()
{
  if (m_UseVblank)
    return (double)m_AdjustedFrequency.QuadPart / (double)m_SystemFrequency.QuadPart;
  else
    return 1.0;
}

bool CVideoReferenceClock::UpdateRefreshrate()
{
  bool Changed = false;
  if (m_CurrTime.QuadPart - m_LastRefreshTime.QuadPart > m_SystemFrequency.QuadPart)
  {
#ifdef HAS_GLX
    m_LastRefreshTime = m_CurrTime;
    
    XRRScreenConfiguration *CurrInfo;
    CurrInfo = XRRGetScreenInfo(m_Dpy, RootWindow(m_Dpy, m_Screen));
    int RRRefreshRate = XRRConfigCurrentRate(CurrInfo);
    XRRFreeScreenConfigInfo(CurrInfo);
    
    if (RRRefreshRate != m_PrevRefreshRate)
    {
      m_PrevRefreshRate = RRRefreshRate;
      if (m_UseNvSettings)
      {
        float fRefreshRate;
        char Buff[256];
        struct lconv *Locale = localeconv();
        
        FILE* NvSettings = popen(NVSETTINGSCMD, "r");
        fscanf(NvSettings, "%255s", Buff);
        pclose(NvSettings);
        Buff[255] = 0;
        
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: Output of %s: %s", NVSETTINGSCMD, Buff);
        
        for (int i = 0; i < 256 && Buff[i]; i++)
        {
          if (Buff[i] == '.' || Buff[i] == ',') Buff[i] = *Locale->decimal_point;
          
          if ((Buff[i] < '0' || Buff[i] > '9') && Buff[i] != *Locale->decimal_point)
            Buff[i] = ' ';
        }
        
        sscanf(Buff, "%f", &fRefreshRate);
        m_RefreshRate = MathUtils::round_int(fRefreshRate);
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate by nvidia-settings: %f hertz, rounding to %i hertz",
                             fRefreshRate, (int)m_RefreshRate);
      }
      else
      {
        m_RefreshRate = RRRefreshRate;
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", (int)m_RefreshRate);
      }
      Changed = true;
    }
#elif defined(_WIN32)
    D3dClock::D3DDISPLAYMODE DisplayMode;
    m_D3d->GetAdapterDisplayMode(m_Adapter, &DisplayMode);
    m_RefreshRate = DisplayMode.RefreshRate;

    if (m_RefreshRate == 0) m_RefreshRate = 60;

    if (m_RefreshRate != m_PrevRefreshRate)
    {
      m_PrevRefreshRate = m_RefreshRate;
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", (int)m_RefreshRate);
      Changed = true;
    }
    if (m_Width != DisplayMode.Width || m_Height != DisplayMode.Height)
    {
      Changed = true;
    }
    m_Width = DisplayMode.Width;
    m_Height = DisplayMode.Height;
#endif
    return Changed;
  }
  else
  {
    return false;
  }
}

int CVideoReferenceClock::GetRefreshRate()
{
  if (m_UseVblank)
  {
    return m_RefreshRate;
  }
  else
  {
    return -1;
  }
}

void CVideoReferenceClock::Wait()
{
#ifdef HAS_SDL
  if (m_UseVblank)
  {
    Lock();
    SDL_CondWaitTimeout(m_VblankCond, m_VblankMutex, 100);
    Unlock();
  }
  else
  {
    Sleep(1);
  }
#else
  Sleep(1);
#endif
}

void CVideoReferenceClock::SendVblankSignal()
{
#ifdef HAS_SDL
  SDL_CondSignal(m_VblankCond);
#endif
}

void CVideoReferenceClock::Lock()
{
#ifdef HAS_SDL
  SDL_mutexP(m_VblankMutex);
#endif
}

void CVideoReferenceClock::Unlock()
{
#ifdef HAS_SDL
  SDL_mutexV(m_VblankMutex);
#endif
}

CVideoReferenceClock g_VideoReferenceClock;