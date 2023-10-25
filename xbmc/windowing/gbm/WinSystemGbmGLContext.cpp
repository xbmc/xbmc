/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemGbmGLContext.h"

#include "OptionalsReg.h"
#include "cores/RetroPlayer/process/gbm/RPProcessInfoGbm.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererDMA.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "rendering/gl/ScreenshotSurfaceGL.h"
#include "utils/BufferObjectFactory.h"
#include "utils/DMAHeapBufferObject.h"
#include "utils/DumbBufferObject.h"
#include "utils/GBMBufferObject.h"
#include "utils/UDMABufferObject.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/WindowSystemFactory.h"

#include <mutex>

#include <EGL/eglext.h>

using namespace KODI::WINDOWING::GBM;

using namespace std::chrono_literals;

CWinSystemGbmGLContext::CWinSystemGbmGLContext()
: CWinSystemGbmEGLContext(EGL_PLATFORM_GBM_MESA, "EGL_MESA_platform_gbm")
{}

void CWinSystemGbmGLContext::Register()
{
  CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem, "gbm");
}

std::unique_ptr<CWinSystemBase> CWinSystemGbmGLContext::CreateWinSystem()
{
  return std::make_unique<CWinSystemGbmGLContext>();
}

bool CWinSystemGbmGLContext::InitWindowSystem()
{
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CDVDFactoryCodec::ClearHWAccels();
  CLinuxRendererGL::Register();
  RETRO::CRPProcessInfoGbm::Register();
  RETRO::CRPProcessInfoGbm::RegisterRendererFactory(new RETRO::CRendererFactoryDMA);
  RETRO::CRPProcessInfoGbm::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGL);

  if (!CWinSystemGbmEGLContext::InitWindowSystemEGL(EGL_OPENGL_BIT, EGL_OPENGL_API))
  {
    return false;
  }

  bool general, deepColor;
  m_vaapiProxy.reset(VaapiProxyCreate(m_DRM->GetRenderNodeFileDescriptor()));
  VaapiProxyConfig(m_vaapiProxy.get(), m_eglContext.GetEGLDisplay());
  VAAPIRegisterRenderGL(m_vaapiProxy.get(), general, deepColor);

  if (general)
  {
    VAAPIRegister(m_vaapiProxy.get(), deepColor);
  }

  CScreenshotSurfaceGL::Register();

  CBufferObjectFactory::ClearBufferObjects();
  CDumbBufferObject::Register();
#if defined(HAS_GBM_BO_MAP)
  CGBMBufferObject::Register();
#endif
#if defined(HAVE_LINUX_MEMFD) && defined(HAVE_LINUX_UDMABUF)
  CUDMABufferObject::Register();
#endif
#if defined(HAVE_LINUX_DMA_HEAP)
  CDMAHeapBufferObject::Register();
#endif

  return true;
}

bool CWinSystemGbmGLContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  if (res.iWidth != m_nWidth ||
      res.iHeight != m_nHeight)
  {
    CLog::Log(LOGDEBUG, "CWinSystemGbmGLContext::{} - resolution changed, creating a new window",
              __FUNCTION__);
    CreateNewWindow("", fullScreen, res);
  }

  if (!m_eglContext.TrySwapBuffers())
  {
    CEGLUtils::Log(LOGERROR, "eglSwapBuffers failed");
    throw std::runtime_error("eglSwapBuffers failed");
  }

  CWinSystemGbm::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight);

  return true;
}

void CWinSystemGbmGLContext::PresentRender(bool rendered, bool videoLayer)
{
  if (!m_bRenderCreated)
    return;

  if (rendered || videoLayer)
  {
    if (rendered)
    {
#if defined(EGL_ANDROID_native_fence_sync) && defined(EGL_KHR_fence_sync)
      if (m_eglFence)
      {
        int fd = m_DRM->TakeOutFenceFd();
        if (fd != -1)
        {
          m_eglFence->CreateKMSFence(fd);
          m_eglFence->WaitSyncGPU();
        }

        m_eglFence->CreateGPUFence();
      }
#endif

      if (!m_eglContext.TrySwapBuffers())
      {
        CEGLUtils::Log(LOGERROR, "eglSwapBuffers failed");
        throw std::runtime_error("eglSwapBuffers failed");
      }

#if defined(EGL_ANDROID_native_fence_sync) && defined(EGL_KHR_fence_sync)
      if (m_eglFence)
      {
        int fd = m_eglFence->FlushFence();
        m_DRM->SetInFenceFd(fd);

        m_eglFence->WaitSyncCPU();
      }
#endif
    }
    CWinSystemGbm::FlipPage(rendered, videoLayer, static_cast<bool>(m_eglFence));

    if (m_dispReset && m_dispResetTimer.IsTimePast())
    {
      CLog::Log(LOGDEBUG, "CWinSystemGbmGLContext::{} - Sending display reset to all clients",
                __FUNCTION__);
      m_dispReset = false;
      std::unique_lock<CCriticalSection> lock(m_resourceSection);

      for (auto resource : m_resources)
        resource->OnResetDisplay();
    }
  }
  else
  {
    KODI::TIME::Sleep(10ms);
  }
}

bool CWinSystemGbmGLContext::CreateContext()
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
