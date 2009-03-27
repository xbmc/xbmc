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
  #include <GL/glx.h>
  #define NVSETTINGSCMD "nvidia-settings -nt -q RefreshRate"
#endif

using namespace std;

CVideoReferenceClock::CVideoReferenceClock()
{
  QueryPerformanceFrequency(&m_SystemFrequency);
  m_AdjustedFrequency = m_SystemFrequency;
  m_UseVblank = false;
}

void CVideoReferenceClock::OnStartup()
{
  QueryPerformanceCounter(&m_CurrTime);
  
#ifdef HAS_GLX
  m_UseVblank = SetupGLX();
  if (m_UseVblank) RunGLX();
#elif defined(_WIN32)
  m_UseVblank = SetupD3D();
  if (m_UseVblank) RunD3D();
#endif
}

//TODO: check if unused mem can be freed

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
  GLXFBConfig *fbConfigs = 0;
  XVisualInfo *vInfo = NULL;
  Visual      *Visual;
  GLXContext   Context;
  Pixmap       Pxmp;
  GLXPixmap    GLXPxmp;
  
  m_Dpy = XOpenDisplay(NULL);
  if (!m_Dpy)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to open display, falling back to QueryPerformanceCounter");
    return false;
  }
  
  fbConfigs = glXChooseFBConfig(m_Dpy, DefaultScreen(m_Dpy), singleVisAttributes, &Num);
  if (!fbConfigs)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXChooseFBConfig returned NULL, falling back to QueryPerformanceCounter");
    return false;
  }
  
  vInfo = glXGetVisualFromFBConfig(m_Dpy, fbConfigs[0]);
  if (!vInfo)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVisualFromFBConfig returned NULL, falling back to QueryPerformanceCounter");
    return false;
  } 
  
  m_Screen = DefaultScreen(m_Dpy);
  Visual = DefaultVisual(m_Dpy, m_Screen);
  Depth = DefaultDepth(m_Dpy, m_Screen); 

  Pxmp = XCreatePixmap(m_Dpy, RootWindow(m_Dpy, m_Screen), 100, 100, Depth);
  GLXPxmp = glXCreatePixmap(m_Dpy, fbConfigs[0], Pxmp, NULL);
  Context = glXCreateNewContext(m_Dpy, fbConfigs[0], GLX_RGBA_TYPE, NULL, true);
  glXMakeCurrent(m_Dpy, GLXPxmp, Context);
  
  m_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!m_glXGetVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI not found, falling back to QueryPerformanceCounter");
    return false;
  }
  
  ReturnV = m_glXGetVideoSyncSGI(&VblankCount);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI returned %i, falling back to QueryPerformanceCounter", ReturnV);
    return false;
  }
  
  XRRSizes(m_Dpy, m_Screen, &ReturnV);
  if (ReturnV == 0)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: RandR not supported, falling back to QueryPerformanceCounter");
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

void CVideoReferenceClock::RunGLX()
{
  unsigned int PrevVblankCount;
  unsigned int VblankCount;

  m_glXGetVideoSyncSGI(&PrevVblankCount);
  UpdateRefreshrate();
  
  while(1)
  {
    m_glXGetVideoSyncSGI(&VblankCount); //TODO: check if this has reset
    
    if (VblankCount - PrevVblankCount > 0)
    {
      m_CurrTime.QuadPart += (__int64)(VblankCount - PrevVblankCount) * m_AdjustedFrequency.QuadPart / m_RefreshRate;
      PrevVblankCount = VblankCount;
      if (!UpdateRefreshrate()) Sleep(1);
    }
    else
    {
      Sleep(1);
    }
  }
}
#elif defined(_WIN32)

#define CLASSNAME "videoreferenceclock"

void CVideoReferenceClock::RunD3D()
{
  D3dClock::D3DRASTER_STATUS RasterStatus;
  LARGE_INTEGER              CurrVBlankTime;
  LARGE_INTEGER              LastVBlankTime;

  unsigned int    LastLine;
  int             NrVBlanks;
  double          VBlankTime;

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  m_D3dDev->GetRasterStatus(0, &RasterStatus);

  if (RasterStatus.InVBlank) LastLine = 0;
  else LastLine = RasterStatus.ScanLine;

  QueryPerformanceCounter(&LastVBlankTime);

  while(1)
  {
    m_D3dDev->GetRasterStatus(0, &RasterStatus);

    if (RasterStatus.InVBlank || (RasterStatus.ScanLine < LastLine))
    {
      QueryPerformanceCounter(&CurrVBlankTime);
      VBlankTime = (double)(CurrVBlankTime.QuadPart - LastVBlankTime.QuadPart) / (double)m_SystemFrequency.QuadPart;
      NrVBlanks = MathUtils::round_int(VBlankTime / (1.0 / (double)m_RefreshRate));

      m_CurrTime.QuadPart += (__int64)NrVBlanks * m_SystemFrequency.QuadPart / m_RefreshRate;

      LastVBlankTime = CurrVBlankTime;
      UpdateRefreshrate();
    }
    if (RasterStatus.InVBlank) LastLine = 0;
    else LastLine = RasterStatus.ScanLine;

    Sleep(1);
  }
}

bool CVideoReferenceClock::SetupD3D()
{
  D3dClock::D3DPRESENT_PARAMETERS D3dPP;
  D3dClock::D3DCAPS9              DevCaps;
  D3dClock::D3DRASTER_STATUS      RasterStatus;
  D3dClock::D3DDISPLAYMODE        DisplayMode;

  int ReturnV;

  if (!CreateHiddenWindow())
    return false;

  m_D3d = D3dClock::Direct3DCreate9(D3D_SDK_VERSION);

  if (!m_D3d)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Direct3DCreate9 failed, falling back to QueryPerformanceCounter");
    return false;
  }

  ZeroMemory(&D3dPP, sizeof(D3dPP));
  D3dPP.Windowed = TRUE;
  D3dPP.SwapEffect = D3dClock::D3DSWAPEFFECT_DISCARD;
  D3dPP.hDeviceWindow = m_Hwnd;

  ReturnV = m_D3d->CreateDevice(D3DADAPTER_DEFAULT, D3dClock::D3DDEVTYPE_HAL, m_Hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3dPP, &m_D3dDev);

  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: CreateDevice returned %i, falling back to QueryPerformanceCounter", ReturnV);
    return false;
  }

  ReturnV = m_D3dDev->GetDeviceCaps(&DevCaps);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetDeviceCaps returned %i, falling back to QueryPerformanceCounter", ReturnV);
    return false;
  }

  if (DevCaps.Caps != D3DCAPS_READ_SCANLINE)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Hardware does not support GetRasterStatus, falling back to QueryPerformanceCounter");
    return false;
  }

  ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned %i, falling back to QueryPerformanceCounter", ReturnV);
    return false;
  }

  ReturnV = m_D3dDev->GetDisplayMode(0, &DisplayMode);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetDisplayMode returned %i, falling back to QueryPerformanceCounter", ReturnV);
    return false;
  }

  m_PrevRefreshRate = -1;
  m_LastRefreshTime.QuadPart = 0;
  UpdateRefreshrate();

  return true;
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
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to register window class, falling back to QueryPerformanceCounter");
    return false;
  }

  m_Hwnd = CreateWindowEx(WS_EX_LEFT|WS_EX_LTRREADING|WS_EX_WINDOWEDGE, m_WinCl.lpszClassName, m_WinCl.lpszClassName,
                          WS_OVERLAPPED|WS_MINIMIZEBOX|WS_SYSMENU|WS_CLIPSIBLINGS|WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT,
                          400, 430, HWND_DESKTOP, NULL, m_WinCl.hInstance, NULL);

  if (!m_Hwnd)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: CreateWindowEx failed, falling back to QueryPerformanceCounter");
    return false;
  }

  return true;
}

#endif

void CVideoReferenceClock::GetTime(LARGE_INTEGER *ptime)
{
  if (m_UseVblank)
  {
    *ptime = m_CurrTime;
  }
  else
  {
    QueryPerformanceCounter(ptime);
  }
}

void CVideoReferenceClock::GetFrequency(LARGE_INTEGER *pfreq)
{
  *pfreq = m_SystemFrequency;
}

void CVideoReferenceClock::SetSpeed(double Speed)
{
  if (m_UseVblank)
    m_AdjustedFrequency.QuadPart = (__int64)((double)m_SystemFrequency.QuadPart * Speed);
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
        FILE* NvSettings = popen(NVSETTINGSCMD, "r");
        fscanf(NvSettings, "%f", &fRefreshRate);
        pclose(NvSettings);
        m_RefreshRate = MathUtils::round_int(fRefreshRate);
      }
      else
      {
        m_RefreshRate = RRRefreshRate;
      }
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", (int)m_RefreshRate);
    }
#elif defined(_WIN32)
    D3dClock::D3DDISPLAYMODE DisplayMode;
    m_D3dDev->GetDisplayMode(0, &DisplayMode);
    m_RefreshRate = DisplayMode.RefreshRate;

    if (m_RefreshRate == 0) m_RefreshRate = 60;

    if (m_RefreshRate != m_PrevRefreshRate)
    {
      m_PrevRefreshRate = m_RefreshRate;
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", (int)m_RefreshRate);
    }
#endif
    return true;
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

CVideoReferenceClock g_VideoReferenceClock;