/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <EGL/egl.h>

namespace KODI::RETRO
{
struct EGLClientFunctions
{
  decltype(&eglGetError) getError{eglGetError};
  decltype(&eglQueryAPI) queryAPI{eglQueryAPI};
  decltype(&eglBindAPI) bindAPI{eglBindAPI};
  decltype(&eglGetCurrentDisplay) getCurrentDisplay{eglGetCurrentDisplay};
  decltype(&eglGetCurrentSurface) getCurrentSurface{eglGetCurrentSurface};
  decltype(&eglGetCurrentContext) getCurrentContext{eglGetCurrentContext};
  decltype(&eglCreatePbufferSurface) createPbufferSurface{eglCreatePbufferSurface};
  decltype(&eglCreateContext) createContext{eglCreateContext};
  decltype(&eglMakeCurrent) makeCurrent{eglMakeCurrent};
  decltype(&eglDestroyContext) destroyContext{eglDestroyContext};
  decltype(&eglDestroySurface) destroySurface{eglDestroySurface};
  decltype(&eglReleaseThread) releaseThread{eglReleaseThread};
};

class CEGLClientContext
{
public:
  explicit CEGLClientContext(EGLenum api, const EGLClientFunctions& egl = {})
    : m_api(api),
      m_egl(egl)
  {
  }
  ~CEGLClientContext();
  CEGLClientContext(const CEGLClientContext&) = delete;
  CEGLClientContext& operator=(const CEGLClientContext&) = delete;

  bool Create(EGLDisplay display,
              EGLConfig config,
              EGLContext shared,
              const EGLint* attributes,
              bool surfaceless);
  bool IsCreated() const { return m_context != EGL_NO_CONTEXT; }
  bool MakeCurrent();
  bool RestoreCurrent();
  void Destroy();

private:
  void ReleaseCurrent();
  void ClearPrevious();

  const EGLenum m_api;
  const EGLClientFunctions m_egl;
  EGLDisplay m_display{EGL_NO_DISPLAY};
  EGLContext m_context{EGL_NO_CONTEXT};
  EGLSurface m_surface{EGL_NO_SURFACE};
  bool m_hasPrevious{false};
  EGLenum m_prevAPI{EGL_OPENGL_ES_API};
  EGLDisplay m_prevDisplay{EGL_NO_DISPLAY};
  EGLContext m_prevContext{EGL_NO_CONTEXT};
  EGLSurface m_prevDraw{EGL_NO_SURFACE};
  EGLSurface m_prevRead{EGL_NO_SURFACE};
};
} // namespace KODI::RETRO
