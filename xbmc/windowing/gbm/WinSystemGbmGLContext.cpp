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
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererDMAOpenGL.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "rendering/gl/GuiCompositeShaderGL.h"
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
{
}

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
  RETRO::CRPProcessInfoGbm::RegisterRendererFactory(new RETRO::CRendererFactoryDMAOpenGL);
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

bool CWinSystemGbmGLContext::SetFullScreen(bool fullScreen,
                                           RESOLUTION_INFO& res,
                                           bool blankOtherDisplays)
{
  if (res.iWidth != m_nWidth || res.iHeight != m_nHeight)
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
    bool async = !videoLayer && m_eglFence;
    if (rendered)
    {
#if defined(EGL_ANDROID_native_fence_sync) && defined(EGL_KHR_fence_sync)
      if (async)
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
      if (async)
      {
        int fd = m_eglFence->FlushFence();
        m_DRM->SetInFenceFd(fd);

        m_eglFence->WaitSyncCPU();
      }
#endif
    }

    CWinSystemGbm::FlipPage(rendered, videoLayer, async);
  }
  else
  {
    KODI::TIME::Sleep(10ms);
  }

  if (m_dispReset && m_dispResetTimer.IsTimePast())
  {
    CLog::Log(LOGDEBUG, "CWinSystemGbmGLContext::{} - Sending display reset to all clients",
              __FUNCTION__);
    m_dispReset = false;
    std::unique_lock lock(m_resourceSection);

    for (auto resource : m_resources)
      resource->OnResetDisplay();
  }
}

bool CWinSystemGbmGLContext::CreateContext()
{
  const EGLint glMajor = 3;
  const EGLint glMinor = 2;

  CEGLAttributesVec contextAttribs;
  contextAttribs.Add(
      {{EGL_CONTEXT_MAJOR_VERSION_KHR, glMajor},
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
      CLog::Log(LOGWARNING,
                "Your OpenGL drivers do not support OpenGL {}.{} core profile. Kodi will run in "
                "compatibility mode, but performance may suffer.",
                glMajor, glMinor);
    }
  }

  return true;
}

bool CWinSystemGbmGLContext::SetGuiCompositing(int colorTransfer)
{
  // TODO: add CreateLUTs(colorTransfer) support to GL composite shader to match
  // GLES (WinSystemGbmGLESContext). Currently GL only handles PQ via pow shader;
  // HLG and LUT-based PQ are not yet implemented.
  m_guiCompositing = (colorTransfer != 0);

  if (m_guiCompositing)
  {
    if (!m_compositeShader)
    {
      m_compositeShader = std::make_unique<CGuiCompositeShaderGL>();
      if (!m_compositeShader->CompileAndLink())
      {
        CLog::Log(LOGERROR, "CWinSystemGbmGLContext: failed to compile GUI composite shader");
        m_compositeShader.reset();
        m_guiCompositing = false;
      }
    }
  }
  else
  {
    m_guiFbo.Cleanup();
    m_guiFboWidth = 0;
    m_guiFboHeight = 0;
    m_compositeShader.reset();
  }

  return m_guiCompositing;
}

bool CWinSystemGbmGLContext::BeginGuiComposite()
{
  if (!m_guiCompositing)
    return false;

  int width = m_nWidth;
  int height = m_nHeight;

  if (!m_guiFbo.IsValid() || m_guiFboWidth != width || m_guiFboHeight != height)
  {
    m_guiFbo.Cleanup();

    if (!m_guiFbo.Initialize())
    {
      CLog::Log(LOGERROR, "CWinSystemGbmGLContext: failed to initialize GUI FBO");
      return false;
    }

    if (!m_guiFbo.CreateAndBindToTexture(GL_TEXTURE_2D, width, height, GL_RGBA))
    {
      CLog::Log(LOGERROR, "CWinSystemGbmGLContext: failed to create GUI FBO texture {}x{}", width,
                height);
      m_guiFbo.Cleanup();
      return false;
    }

    m_guiFboWidth = width;
    m_guiFboHeight = height;
    CLog::Log(LOGDEBUG, "CWinSystemGbmGLContext: created GUI FBO {}x{}", width, height);
  }

  if (!m_guiFbo.BeginRender())
    return false;

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  return true;
}

void CWinSystemGbmGLContext::EndGuiComposite()
{
  m_guiFbo.EndRender();

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

// CompositeGui is the last GL operation in the frame (called just before EndRender).
// GL state (blend mode, active texture, vertex arrays) is not restored afterward;
// the next frame's rendering sets its own state.
void CWinSystemGbmGLContext::CompositeGui()
{
  if (!m_guiFbo.IsValid() || !m_guiFbo.IsBound() || !m_compositeShader)
    return;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_guiFbo.Texture());

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // set up orthographic projection (screen coords, Y-down)
  float w = static_cast<float>(m_guiFboWidth);
  float h = static_cast<float>(m_guiFboHeight);

  GLfloat proj[16] = {2.0f / w, 0, 0, 0, 0, -2.0f / h, 0, 0, 0, 0, -1, 0, -1.0f, 1.0f, 0, 1};

  m_compositeShader->SetProjection(proj);
  m_compositeShader->SetSdrPeak(100.0f / 10000.0f);
  m_compositeShader->Enable();

  GLint posLoc = m_compositeShader->GetPosLoc();
  GLint texLoc = m_compositeShader->GetTexLoc();

  GLfloat vert[4][2] = {{0, 0}, {w, 0}, {w, h}, {0, h}};
  GLfloat tex[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
  GLubyte idx[4] = {0, 1, 3, 2};

  GLuint vbo[3];
  glGenBuffers(3, vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STREAM_DRAW);
  glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(posLoc);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tex), tex, GL_STREAM_DRAW);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(texLoc);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STREAM_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(3, vbo);

  m_compositeShader->Disable();
}
