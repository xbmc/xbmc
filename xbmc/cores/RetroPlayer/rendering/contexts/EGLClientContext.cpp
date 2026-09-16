/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EGLClientContext.h"

#include "utils/log.h"

using namespace KODI::RETRO;

CEGLClientContext::~CEGLClientContext()
{
  Destroy();
}

bool CEGLClientContext::Create(EGLDisplay display,
                               EGLConfig config,
                               EGLContext shared,
                               const EGLint* attributes,
                               bool surfaceless)
{
  Destroy();
  if (display == EGL_NO_DISPLAY || !config || shared == EGL_NO_CONTEXT)
    return false;

  m_display = display;
  const EGLenum previousAPI = m_egl.queryAPI();
  if (!m_egl.bindAPI(m_api))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to select client EGL API (error {:#x})",
              m_egl.getError());
    m_display = EGL_NO_DISPLAY;
    return false;
  }

  if (!surfaceless)
  {
    const EGLint surfaceAttributes[]{EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    m_surface = m_egl.createPbufferSurface(display, config, surfaceAttributes);
    if (m_surface == EGL_NO_SURFACE)
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to create client pbuffer (EGL error {:#x})",
                m_egl.getError());
  }
  if (surfaceless || m_surface != EGL_NO_SURFACE)
  {
    m_context = m_egl.createContext(display, config, shared, attributes);
    if (!IsCreated())
      CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Shared EGL context request failed (error {:#x})",
                m_egl.getError());
  }
  const bool restored = m_egl.bindAPI(previousAPI) == EGL_TRUE;
  if (!restored)
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Failed to restore EGL API after creation (error {:#x})",
              m_egl.getError());
  if (!IsCreated() || !restored)
  {
    Destroy();
    return false;
  }
  return true;
}

bool CEGLClientContext::MakeCurrent()
{
  if (!IsCreated() || m_hasPrevious || m_egl.getCurrentContext() == m_context)
    return false;

  m_prevAPI = m_egl.queryAPI();
  m_prevDisplay = m_egl.getCurrentDisplay();
  m_prevContext = m_egl.getCurrentContext();
  m_prevDraw = m_egl.getCurrentSurface(EGL_DRAW);
  m_prevRead = m_egl.getCurrentSurface(EGL_READ);
  if (!m_egl.bindAPI(m_api) || !m_egl.makeCurrent(m_display, m_surface, m_surface, m_context))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to bind client context (EGL error {:#x})",
              m_egl.getError());
    m_egl.bindAPI(m_prevAPI);
    ClearPrevious();
    return false;
  }
  m_hasPrevious = true;
  return true;
}

void CEGLClientContext::ReleaseCurrent()
{
  if (!m_egl.bindAPI(m_api) ||
      !m_egl.makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Releasing EGL thread after unbind failed (error {:#x})",
              m_egl.getError());
    if (!m_egl.releaseThread())
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to release EGL thread (error {:#x})",
                m_egl.getError());
  }
}

bool CEGLClientContext::RestoreCurrent()
{
  if (!m_hasPrevious)
    return true;

  if (m_prevAPI != m_api)
    ReleaseCurrent();
  const bool restored =
      m_egl.bindAPI(m_prevAPI) &&
      (m_prevContext != EGL_NO_CONTEXT
           ? m_egl.makeCurrent(m_prevDisplay, m_prevDraw, m_prevRead, m_prevContext)
           : m_egl.makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  if (!restored)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to restore EGL context (error {:#x})",
              m_egl.getError());
    ReleaseCurrent();
    m_egl.bindAPI(m_prevAPI);
  }
  ClearPrevious();
  return restored;
}

void CEGLClientContext::ClearPrevious()
{
  m_hasPrevious = false;
  m_prevDisplay = EGL_NO_DISPLAY;
  m_prevContext = EGL_NO_CONTEXT;
  m_prevDraw = m_prevRead = EGL_NO_SURFACE;
}

void CEGLClientContext::Destroy()
{
  if (m_hasPrevious)
    RestoreCurrent();
  else if (IsCreated() && m_egl.getCurrentContext() == m_context)
  {
    const EGLenum previousAPI = m_egl.queryAPI();
    ReleaseCurrent();
    m_egl.bindAPI(previousAPI);
  }

  // Unbind before deleting the context, then release only our own pbuffer.
  if (IsCreated() && !m_egl.destroyContext(m_display, m_context))
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Abandoning EGL context after destroy failed (error {:#x})",
              m_egl.getError());
  if (m_surface != EGL_NO_SURFACE && !m_egl.destroySurface(m_display, m_surface))
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Abandoning EGL pbuffer after destroy failed (error {:#x})",
              m_egl.getError());
  // Lost displays can reject destruction; stale handles must never reach the next session.
  m_context = EGL_NO_CONTEXT;
  m_surface = EGL_NO_SURFACE;
  m_display = EGL_NO_DISPLAY;
  ClearPrevious();
}
