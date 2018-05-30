/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://kodi.tv
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

#include "windowing/VideoSync.h"
#include "system_gl.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include "guilib/DispResource.h"
#include "threads/Event.h"

class CWinSystemX11GLContext;

class CVideoSyncGLX : public CVideoSync, IDispResource
{
public:
  explicit CVideoSyncGLX(void *clock, CWinSystemX11GLContext& winSystem) :
    CVideoSync(clock), m_winSystem(winSystem) {};
  bool Setup(PUPDATECLOCK func) override;
  void Run(CEvent& stopEvent) override;
  void Cleanup() override;
  float GetFps() override;
  void OnLostDisplay() override;
  void OnResetDisplay() override;

private:
  int  (*m_glXWaitVideoSyncSGI) (int, int, unsigned int*);
  int  (*m_glXGetVideoSyncSGI)  (unsigned int*);

  static Display* m_Dpy;
  CWinSystemX11GLContext &m_winSystem;
  XVisualInfo *m_vInfo;
  Window       m_Window;
  GLXContext   m_Context;
  volatile bool m_displayLost;
  volatile bool m_displayReset;
  CEvent m_lostEvent;
};
