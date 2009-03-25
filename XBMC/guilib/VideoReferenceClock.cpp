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

using namespace std;
using namespace Surface;

CVideoReferenceClock::CVideoReferenceClock()
{
  QueryPerformanceFrequency(&SystemFrequency);
  AdjustedFrequency = SystemFrequency;
  UseVblank = false;
}

void CVideoReferenceClock::OnStartup()
{
  QueryPerformanceCounter(&CurrTime);
  
#ifdef HAS_GLX
  UseVblank = SetupGLX();
  if (UseVblank) RunGLX();
#endif
}

#ifdef HAS_GLX
bool CVideoReferenceClock::SetupGLX()
{
  int Num = 0, ReturnV, Screen, Depth;
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
  Visual *Visual;
  Display* Dpy;
  GLXContext Context;
  Pixmap Pxmp;
  GLXPixmap GLXPxmp;
  
  Dpy = XOpenDisplay(NULL);
  if (!Dpy)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to open display, falling back to QueryPerformanceCounter");
    return false;
  }
  
  fbConfigs = glXChooseFBConfig(Dpy, DefaultScreen(Dpy), singleVisAttributes, &Num);
  if (!fbConfigs)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXChooseFBConfig returned NULL, falling back to QueryPerformanceCounter");
    return false;
  }
  
  vInfo = glXGetVisualFromFBConfig(Dpy, fbConfigs[0]);
  if (!vInfo)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVisualFromFBConfig returned NULL, falling back to QueryPerformanceCounter");
    return false;
  } 
  
  Screen = DefaultScreen(Dpy);
  Visual = DefaultVisual(Dpy, Screen);
  Depth  = DefaultDepth(Dpy, Screen); 

  Pxmp = XCreatePixmap(Dpy, RootWindow(Dpy, Screen), 100, 100, Depth);
  GLXPxmp = glXCreatePixmap(Dpy, fbConfigs[0], Pxmp, NULL);
  Context = glXCreateNewContext(Dpy, fbConfigs[0], GLX_RGBA_TYPE, NULL, true);
  glXMakeCurrent(Dpy, GLXPxmp, Context);
  
  p_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!p_glXGetVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI not found, falling back to QueryPerformanceCounter");
    return false;
  }
  
  ReturnV = p_glXGetVideoSyncSGI(&VblankCount);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI returned %i, falling back to QueryPerformanceCounter", ReturnV);
    return false;
  }
  
  return true;
}

void CVideoReferenceClock::RunGLX()
{
  unsigned int PrevVblankCount;
  unsigned int VblankCount;
  __int64 RefreshRate;

  p_glXGetVideoSyncSGI(&PrevVblankCount);
  
  while(1)
  {
    p_glXGetVideoSyncSGI(&VblankCount);
    
    if (VblankCount - PrevVblankCount > 0)
    {
      RefreshRate = MathUtils::round_int(g_renderManager.GetMaximumFPS());
      if (RefreshRate <= 0) RefreshRate = 60;
      
      CurrTime.QuadPart += (__int64)(VblankCount - PrevVblankCount) * AdjustedFrequency.QuadPart / RefreshRate;
      PrevVblankCount = VblankCount;
    }    
    Sleep(1);
  }
}

#endif

void CVideoReferenceClock::GetTime(LARGE_INTEGER *ptime)
{
  if (UseVblank)
  {
    *ptime = CurrTime;
  }
  else
  {
    QueryPerformanceCounter(ptime);
  }
}

void CVideoReferenceClock::SetSpeed(double Speed)
{
  AdjustedFrequency.QuadPart = (__int64)((double)SystemFrequency.QuadPart * Speed);
}

double CVideoReferenceClock::GetSpeed()
{
  return (double)AdjustedFrequency.QuadPart / (double)SystemFrequency.QuadPart;
}

CVideoReferenceClock g_VideoReferenceClock;