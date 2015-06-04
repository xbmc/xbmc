/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#pragma once

#if defined(HAVE_X11)
#include "GLContext.h"
#include "GL/glx.h"

class CGLContextGLX : public CGLContext
{
public:
  CGLContextGLX(Display *dpy);
  virtual bool Refresh(bool force, int screen, Window glWindow, bool &newContext);
  virtual void Destroy();
  virtual void Detach();
  virtual void SetVSync(bool enable, int &mode);
  virtual bool SwapBuffers(const CDirtyRegionList& dirty, int &mode);
  virtual void QueryExtensions();
  virtual bool IsExtSupported(const char* extension);
protected:
  bool IsSuitableVisual(XVisualInfo *vInfo);

  int (*m_glXGetVideoSyncSGI)(unsigned int*);
  int (*m_glXWaitVideoSyncSGI)(int, int, unsigned int*);
  int (*m_glXSwapIntervalMESA)(int);
  PFNGLXSWAPINTERVALEXTPROC m_glXSwapIntervalEXT;
  int m_nScreen;
  int m_iVSyncErrors;
};

#endif
