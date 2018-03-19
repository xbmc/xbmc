/*
 *      Copyright (C) 2017 Team XBMC
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

#include "WinSystemWaylandEGLContextGL.h"
#include "OptionalsReg.h"

#include <EGL/egl.h>

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "utils/log.h"

using namespace KODI::WINDOWING::WAYLAND;

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemWaylandEGLContextGL());
  return winSystem;
}

bool CWinSystemWaylandEGLContextGL::InitWindowSystem()
{
  if (!CWinSystemWaylandEGLContext::InitWindowSystemEGL(EGL_OPENGL_BIT, EGL_OPENGL_API))
  {
    return false;
  }

  CLinuxRendererGL::Register();
  RETRO::CRPProcessInfo::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGL);

  bool general, deepColor;
  m_vaapiProxy.reset(::WAYLAND::VaapiProxyCreate());
  ::WAYLAND::VaapiProxyConfig(m_vaapiProxy.get(),GetConnection()->GetDisplay(),
                              m_eglContext.GetEGLDisplay());
  ::WAYLAND::VAAPIRegisterRender(m_vaapiProxy.get(), general, deepColor);
  if (general)
  {
    ::WAYLAND::VAAPIRegister(m_vaapiProxy.get(), deepColor);
  }

  return true;
}

bool CWinSystemWaylandEGLContextGL::CreateContext()
{
  const EGLint glMajor = 3;
  const EGLint glMinor = 2;

  const EGLint contextAttribs[] = {
    EGL_CONTEXT_MAJOR_VERSION_KHR, glMajor,
    EGL_CONTEXT_MINOR_VERSION_KHR, glMinor,
    EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
    EGL_NONE
  };

  if (!m_eglContext.CreateContext(contextAttribs))
  {
    const EGLint fallbackContextAttribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    if (!m_eglContext.CreateContext(fallbackContextAttribs))
    {
      CLog::Log(LOGERROR, "EGL context creation failed");
      return false;
    }
    else
    {
      CLog::Log(LOGWARNING, "Your OpenGL drivers do not support OpenGL {}.{} core profile. Kodi will run in compatibility mode, but performance may suffer.", glMajor, glMinor);
    }
  }

  return true;
}

void CWinSystemWaylandEGLContextGL::SetContextSize(CSizeInt size)
{
  CWinSystemWaylandEGLContext::SetContextSize(size);

  // Propagate changed dimensions to render system if necessary
  if (CRenderSystemGL::m_width != size.Width() || CRenderSystemGL::m_height != size.Height())
  {
    CLog::LogF(LOGDEBUG, "Resetting render system to %dx%d", size.Width(), size.Height());
    CRenderSystemGL::ResetRenderSystem(size.Width(), size.Height());
  }
}

void CWinSystemWaylandEGLContextGL::SetVSyncImpl(bool enable)
{
  // Unsupported
}

void CWinSystemWaylandEGLContextGL::PresentRenderImpl(bool rendered)
{
  PresentFrame(rendered);
}

void CWinSystemWaylandEGLContextGL::delete_CVaapiProxy::operator()(CVaapiProxy *p) const
{
  ::WAYLAND::VaapiProxyDelete(p);
}
