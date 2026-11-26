/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWaylandEGLContext.h"

#include "Connection.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <EGL/eglext.h>

namespace
{
/// Consecutive failed swaps tolerated before treating the failure as permanent
/// (about one second at 60 Hz)
constexpr unsigned int MAX_CONSECUTIVE_SWAP_FAILURES = 60;
} // namespace

using namespace KODI::WINDOWING::WAYLAND;
using namespace KODI::WINDOWING::LINUX;

CWinSystemWaylandEGLContext::CWinSystemWaylandEGLContext()
  : CWinSystemEGL{EGL_PLATFORM_WAYLAND_EXT, "EGL_EXT_platform_wayland"}
{}

bool CWinSystemWaylandEGLContext::InitWindowSystemEGL(EGLint renderableType, EGLint apiType)
{
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CDVDFactoryCodec::ClearHWAccels();
  if (!CWinSystemWaylandImpl::InitWindowSystem())
  {
    return false;
  }

  if (!m_eglContext.CreatePlatformDisplay(GetConnection()->GetDisplay(), GetConnection()->GetDisplay()))
  {
    return false;
  }

  if (!m_eglContext.InitializeDisplay(apiType))
  {
    return false;
  }

  if (!m_eglContext.ChooseConfig(renderableType))
  {
    return false;
  }

  return true;
}

bool CWinSystemWaylandEGLContext::CreateNewWindow(const std::string& name,
                                                  bool fullScreen,
                                                  RESOLUTION_INFO& res)
{
  if (!CWinSystemWaylandImpl::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!CreateContext())
  {
    return false;
  }

  m_nativeWindow = wayland::egl_window_t{GetMainSurface(), GetBufferSize().Width(), GetBufferSize().Height()};

  // CWinSystemWayland::CreateNewWindow sets internal m_bufferSize
  // to the resolution that should be used for the initial surface size
  // - the compositor might want something other than the resolution given
  if (!m_eglContext.CreatePlatformSurface(
          m_nativeWindow.c_ptr(), reinterpret_cast<khronos_uintptr_t>(m_nativeWindow.c_ptr())))
  {
    return false;
  }

  if (!m_eglContext.BindContext())
  {
    return false;
  }

  // Never enable the vsync of the EGL implementation, we handle that ourselves
  // in WinSystemWayland
  m_eglContext.SetVSync(false);

  return true;
}

bool CWinSystemWaylandEGLContext::DestroyWindow()
{
  m_eglContext.DestroySurface();
  m_nativeWindow = {};

  return CWinSystemWaylandImpl::DestroyWindow();
}

bool CWinSystemWaylandEGLContext::DestroyWindowSystem()
{
  m_eglContext.Destroy();

  return CWinSystemWaylandImpl::DestroyWindowSystem();
}

bool CWinSystemWaylandEGLContext::BindTextureUploadContext()
{
  return m_eglContext.BindTextureUploadContext();
}

bool CWinSystemWaylandEGLContext::UnbindTextureUploadContext()
{
  return m_eglContext.UnbindTextureUploadContext();
}

bool CWinSystemWaylandEGLContext::HasContext()
{
  return m_eglContext.HasContext();
}

CSizeInt CWinSystemWaylandEGLContext::GetNativeWindowAttachedSize()
{
  int width, height;
  m_nativeWindow.get_attached_size(width, height);
  return {width, height};
}

void CWinSystemWaylandEGLContext::SetContextSize(CSizeInt size)
{
  // Change EGL surface size if necessary
  if (GetNativeWindowAttachedSize() != size)
  {
    CLog::LogF(LOGDEBUG, "Updating egl_window size to {}x{}", size.Width(), size.Height());
    m_nativeWindow.resize(size.Width(), size.Height(), 0, 0);
  }
}

void CWinSystemWaylandEGLContext::PresentFrame(bool rendered)
{
  PrepareFramePresentation();

  if (rendered)
  {
    if (m_eglContext.TrySwapBuffers())
    {
      if (m_swapFailures > 0)
      {
        CLog::LogF(LOGINFO, "eglSwapBuffers succeeded again after {} failed frame(s)",
                   m_swapFailures);
        m_swapFailures = 0;
      }
    }
    else
    {
      // Read the error first, CEGLUtils::Log() would consume it
      const EGLint error = eglGetError();

      // Errors that mean the display, context or surface is gone. Recovering
      // from these would require recreating them, which is not implemented.
      const bool fatal = error == EGL_CONTEXT_LOST || error == EGL_BAD_CONTEXT ||
                         error == EGL_BAD_SURFACE || error == EGL_BAD_NATIVE_WINDOW ||
                         error == EGL_BAD_DISPLAY || error == EGL_NOT_INITIALIZED;

      // Anything else (e.g. EGL_BAD_ALLOC, or EGL_FALSE without any error set)
      // can be transient: NVIDIA's egl-wayland2 fails a swap without setting an
      // error when it cannot get the next back buffer, and reallocates its
      // swapchain on the next call. Drop the frame and keep going, but do not
      // spin forever if the failure turns out to be persistent.
      ++m_swapFailures;
      if (fatal || m_swapFailures > MAX_CONSECUTIVE_SWAP_FAILURES)
      {
        CLog::LogF(LOGFATAL, "eglSwapBuffers failed ({:#06x}) {} time(s) in a row, giving up",
                   error, m_swapFailures);
        throw std::runtime_error("eglSwapBuffers failed");
      }
      if (m_swapFailures == 1)
      {
        CLog::LogF(LOGWARNING, "eglSwapBuffers failed ({:#06x}), dropping frame", error);
      }

      // The swap may have failed before committing the surface. Commit anyway,
      // as for a frame that was not rendered, so the presentation feedback and
      // frame callback requested for this frame are not left pending.
      GetMainSurface().commit();
      GetConnection()->GetDisplay().flush();
    }
    // eglSwapBuffers() (hopefully) calls commit on the surface and flushes
    // ... well mesa does anyway
  }
  else
  {
    // For presentation feedback: Get notification of the next vblank even
    // when contents did not change
    GetMainSurface().commit();
    // Make sure it reaches the compositor
    GetConnection()->GetDisplay().flush();
  }

  FinishFramePresentation();
}
