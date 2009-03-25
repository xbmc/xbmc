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
#include <iostream> //for debugging, please remove
#include "VideoReferenceClock.h"
#include "../xbmc/cores/VideoRenderers/RenderManager.h"
#include "../xbmc/Util.h"

#ifdef HAS_GLX

#include <X11/extensions/Xrandr.h>
#define NVSETTINGSCMD "nvidia-settings -nt -q RefreshRate"

#endif

using namespace std;
using namespace Surface;

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
#endif
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

void CVideoReferenceClock::SetSpeed(double Speed)
{
  m_AdjustedFrequency.QuadPart = (__int64)((double)m_SystemFrequency.QuadPart * Speed);
}

double CVideoReferenceClock::GetSpeed()
{
  return (double)m_AdjustedFrequency.QuadPart / (double)m_SystemFrequency.QuadPart;
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
    
    return true;
#endif
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