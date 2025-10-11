/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLContext.h"
#include "threads/CriticalSection.h"

#include <cstdint>

#include "system_egl.h"

#include <EGL/eglext.h>
#ifdef HAVE_EGLEXTANGLE
#include <EGL/eglext_angle.h>
#elif __has_include(<EGL/eglextchromium.h>)
#include <EGL/eglextchromium.h>
#else
#ifndef EGL_CHROMIUM_sync_control
#define EGL_CHROMIUM_sync_control 1
typedef EGLBoolean(EGLAPIENTRYP PFNEGLGETSYNCVALUESCHROMIUMPROC)(EGLDisplay dpy,
                                                                 EGLSurface surface,
                                                                 EGLuint64KHR* ust,
                                                                 EGLuint64KHR* msc,
                                                                 EGLuint64KHR* sbc);
#endif
#endif
#include <X11/Xutil.h>

class CGLContextEGL : public CGLContext
{
public:
  explicit CGLContextEGL(Display *dpy, EGLint renderingApi);
  ~CGLContextEGL() override;
  bool Refresh(bool force, int screen, Window glWindow, bool &newContext) override;
  bool CreatePB() override;
  void Destroy() override;
  void Detach() override;
  void SetVSync(bool enable) override;
  void SwapBuffers() override;
  void QueryExtensions() override;
  bool IsBufferAgeSupported() override { return IsExtSupported("EGL_EXT_buffer_age"); }
  int GetBufferAge() override;
  uint64_t GetVblankTiming(uint64_t &msc, uint64_t &interval) override;

  bool BindTextureUploadContext();
  bool UnbindTextureUploadContext();
  bool HasContext();

  EGLint m_renderingApi;
  EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
  EGLSurface m_eglSurface = EGL_NO_SURFACE;
  EGLContext m_eglContext = EGL_NO_CONTEXT;
  EGLConfig m_eglConfig;
protected:
  bool SuitableCheck(EGLDisplay eglDisplay, EGLConfig config);
  EGLConfig GetEGLConfig(EGLDisplay eglDisplay, XVisualInfo *vInfo);
  PFNEGLGETSYNCVALUESCHROMIUMPROC m_eglGetSyncValuesCHROMIUM = nullptr;
  PFNEGLGETPLATFORMDISPLAYEXTPROC m_eglGetPlatformDisplayEXT = nullptr;

  struct Sync
  {
    uint64_t cont = 0;
    uint64_t ust1 = 0;
    uint64_t ust2 = 0;
    uint64_t msc1 = 0;
    uint64_t msc2 = 0;
    uint64_t interval = 0;
  } m_sync;

  CCriticalSection m_syncLock;

  bool m_usePB = false;

  EGLContext m_eglUploadContext = EGL_NO_CONTEXT;
  mutable CCriticalSection m_textureUploadLock;
};
