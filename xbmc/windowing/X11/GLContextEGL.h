/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLContext.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "EGL/eglextchromium.h"
#include <X11/Xutil.h>

class CGLContextEGL : public CGLContext
{
public:
  explicit CGLContextEGL(Display *dpy);
  ~CGLContextEGL() override;
  bool Refresh(bool force, int screen, Window glWindow, bool &newContext) override;
  bool CreatePB() override;
  void Destroy() override;
  void Detach() override;
  void SetVSync(bool enable) override;
  void SwapBuffers() override;
  void QueryExtensions() override;
  uint64_t GetVblankTiming(uint64_t &msc, uint64_t &interval) override;

  EGLDisplay m_eglDisplay;
  EGLSurface m_eglSurface;
  EGLContext m_eglContext;
  EGLConfig m_eglConfig;
protected:
  bool IsSuitableVisual(XVisualInfo *vInfo);
  EGLConfig GetEGLConfig(EGLDisplay eglDisplay, XVisualInfo *vInfo);
  PFNEGLGETSYNCVALUESCHROMIUMPROC eglGetSyncValuesCHROMIUM = nullptr;
  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = nullptr;

  struct Sync
  {
    uint64_t cont = 0;
    uint64_t ust1 = 0;
    uint64_t ust2 = 0;
    uint64_t msc1 = 0;
    uint64_t msc2 = 0;
    uint64_t interval = 0;
  } m_sync;

  bool m_usePB = false;
};
