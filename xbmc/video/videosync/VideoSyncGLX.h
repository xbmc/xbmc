#pragma once
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

#if defined(HAS_GLX)

#include "video/videosync/VideoSync.h"
#include "system_gl.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include "guilib/DispResource.h"
#include "threads/Event.h"

class CVideoSyncGLX : public CVideoSync, IDispResource
{
public:
  virtual bool Setup(PUPDATECLOCK func);
  virtual void Run(volatile bool& stop);
  virtual void Cleanup();
  virtual float GetFps();
  virtual void OnLostDevice();
  virtual void OnResetDevice();

private:
  int  (*m_glXWaitVideoSyncSGI) (int, int, unsigned int*);
  int  (*m_glXGetVideoSyncSGI)  (unsigned int*);

  static Display* m_Dpy;
  XVisualInfo *m_vInfo;
  Window       m_Window;
  GLXContext   m_Context;
  volatile bool m_displayLost;
  volatile bool m_displayReset;
  CEvent m_lostEvent;
};

#endif
