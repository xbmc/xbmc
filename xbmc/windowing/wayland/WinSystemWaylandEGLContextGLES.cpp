/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWaylandEGLContextGLES.h"

#include "OptionalsReg.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "rendering/gles/ScreenshotSurfaceGLES.h"
#include "utils/log.h"

#include <EGL/egl.h>

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

  CScreenshotSurfaceGLES::Register();

  return true;
}

bool CWinSystemWaylandEGLContextGLES::CreateContext()
{
  CEGLAttributesVec contextAttribs;
  contextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, 2}});

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
