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

#include "WinSystemWaylandEGLContextGLES.h"
#include "OptionalsReg.h"

#include <EGL/egl.h>

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "utils/log.h"

using namespace KODI::WINDOWING::WAYLAND;

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemWaylandEGLContextGLES());
  return winSystem;
}

bool CWinSystemWaylandEGLContextGLES::InitWindowSystem()
{
  if (!CWinSystemWaylandEGLContext::InitWindowSystemEGL(EGL_OPENGL_ES2_BIT, EGL_OPENGL_ES_API))
  {
    return false;
  }

  CLinuxRendererGLES::Register();
  RETRO::CRPProcessInfo::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);

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

bool CWinSystemWaylandEGLContextGLES::CreateContext()
{
  const EGLint contextAttribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  if (!m_eglContext.CreateContext(contextAttribs))
  {
    CLog::Log(LOGERROR, "EGL context creation failed");
    return false;
  }
  return true;
}

void CWinSystemWaylandEGLContextGLES::SetContextSize(CSizeInt size)
{
  CWinSystemWaylandEGLContext::SetContextSize(size);

  // Propagate changed dimensions to render system if necessary
  if (CRenderSystemGLES::m_width != size.Width() || CRenderSystemGLES::m_height != size.Height())
  {
    CLog::LogF(LOGDEBUG, "Resetting render system to %dx%d", size.Width(), size.Height());
    CRenderSystemGLES::ResetRenderSystem(size.Width(), size.Height());
  }
}

void CWinSystemWaylandEGLContextGLES::SetVSyncImpl(bool enable)
{
  // Unsupported
}

void CWinSystemWaylandEGLContextGLES::PresentRenderImpl(bool rendered)
{
  PresentFrame(rendered);
}

void CWinSystemWaylandEGLContextGLES::delete_CVaapiProxy::operator()(CVaapiProxy *p) const
{
  ::WAYLAND::VaapiProxyDelete(p);
}
