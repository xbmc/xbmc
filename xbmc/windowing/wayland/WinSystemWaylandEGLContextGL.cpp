/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWaylandEGLContextGL.h"

#include "OptionalsReg.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererDMA.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "rendering/gl/ScreenshotSurfaceGL.h"
#include "utils/BufferObjectFactory.h"
#include "utils/DMAHeapBufferObject.h"
#include "utils/UDMABufferObject.h"
#include "utils/log.h"
#include "windowing/WindowSystemFactory.h"

#include <EGL/eglext.h>

using namespace KODI::WINDOWING::WAYLAND;

void CWinSystemWaylandEGLContextGL::Register()
{
  CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem, "wayland");
}

std::unique_ptr<CWinSystemBase> CWinSystemWaylandEGLContextGL::CreateWinSystem()
{
  return std::make_unique<CWinSystemWaylandEGLContextGL>();
}

bool CWinSystemWaylandEGLContextGL::InitWindowSystem()
{
  if (!CWinSystemWaylandEGLContext::InitWindowSystemEGL(EGL_OPENGL_BIT, EGL_OPENGL_API))
  {
    return false;
  }

  CLinuxRendererGL::Register();
  RETRO::CRPProcessInfo::RegisterRendererFactory(new RETRO::CRendererFactoryDMA);
  RETRO::CRPProcessInfo::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGL);

  bool general, deepColor;
  m_vaapiProxy.reset(WAYLAND::VaapiProxyCreate());
  WAYLAND::VaapiProxyConfig(m_vaapiProxy.get(), GetConnection()->GetDisplay(),
                            m_eglContext.GetEGLDisplay());
  WAYLAND::VAAPIRegisterRender(m_vaapiProxy.get(), general, deepColor);
  if (general)
  {
    WAYLAND::VAAPIRegister(m_vaapiProxy.get(), deepColor);
  }

  CBufferObjectFactory::ClearBufferObjects();
#if defined(HAVE_LINUX_MEMFD) && defined(HAVE_LINUX_UDMABUF)
  CUDMABufferObject::Register();
#endif
#if defined(HAVE_LINUX_DMA_HEAP)
  CDMAHeapBufferObject::Register();
#endif

  CScreenshotSurfaceGL::Register();

  return true;
}

bool CWinSystemWaylandEGLContextGL::CreateContext()
{
  const EGLint glMajor = 3;
  const EGLint glMinor = 2;

  CEGLAttributesVec contextAttribs;
  contextAttribs.Add({{EGL_CONTEXT_MAJOR_VERSION_KHR, glMajor},
                      {EGL_CONTEXT_MINOR_VERSION_KHR, glMinor},
                      {EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR}});

  if (!m_eglContext.CreateContext(contextAttribs))
  {
    CEGLAttributesVec fallbackContextAttribs;
    fallbackContextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, 2}});

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
    CLog::LogF(LOGDEBUG, "Resetting render system to {}x{}", size.Width(), size.Height());
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
  WAYLAND::VaapiProxyDelete(p);
}
