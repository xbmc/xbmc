/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/DispResource.h"
#include "threads/Event.h"
#include "windowing/VideoSync.h"

#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "system_gl.h"



namespace KODI
{
namespace WINDOWING
{
namespace X11
{

class CWinSystemX11GLContext;

class CVideoSyncGLX : public CVideoSync, IDispResource
{
public:
  explicit CVideoSyncGLX(CVideoReferenceClock* clock, CWinSystemX11GLContext& winSystem)
    : CVideoSync(clock), m_winSystem(winSystem)
  {
  }
  bool Setup() override;
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

}
}
}
