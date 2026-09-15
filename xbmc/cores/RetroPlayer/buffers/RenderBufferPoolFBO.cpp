/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderBufferPoolFBO.h"

#include "RenderBufferFBO.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererFBO.h"
#include "rendering/RenderSystem.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

#include <string>
#include <utility>
#include <vector>

using namespace KODI;
using namespace RETRO;

namespace
{
#if defined(HAS_GLES)
constexpr EGLenum CLIENT_API = EGL_OPENGL_ES_API;
#else
constexpr EGLenum CLIENT_API = EGL_OPENGL_API;
#endif

class CRestoreEGLAPI
{
public:
  ~CRestoreEGLAPI() { eglBindAPI(m_api); }

private:
  const EGLenum m_api{eglQueryAPI()};
};
} // namespace

CRenderBufferPoolFBO::CRenderBufferPoolFBO(CRenderContext& context) : m_context(context)
{
}

CRenderBufferPoolFBO::~CRenderBufferPoolFBO()
{
  DestroyContext();
  if (m_eglContext != EGL_NO_CONTEXT)
    CLog::Log(LOGWARNING, "RetroPlayer[RENDER]: FBO context outlived its stream, leaking it");
}

bool CRenderBufferPoolFBO::SupportsHardwareRendering() const
{
  // Asked of the window system rather than the build: a build carrying this
  // pool can still be running where there is no EGL display to share.
  auto* winSystem =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());
  if (winSystem == nullptr)
    return false;

  auto* renderSystem = m_context.Rendering();
  if (!renderSystem || winSystem->GetEGLDisplay() == EGL_NO_DISPLAY)
    return false;
  unsigned int major = 0, minor = 0;
  renderSystem->GetRenderVersion(major, minor);
#if defined(HAS_GLES)
  return major >= 3;
#else
  return major > 3 || (major == 3 && minor >= 2);
#endif
}

bool CRenderBufferPoolFBO::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return CRPRendererFBO::SupportsScalingMethod(renderSettings.GetScalingMethod());
}

bool CRenderBufferPoolFBO::ConfigureInternal()
{
  // Hardware-rendered streams carry no CPU-side pixel format. Software ones
  // declare a real one and belong to the DMA and sysmem pools.
  return m_format == AV_PIX_FMT_NONE;
}

IRenderBuffer* CRenderBufferPoolFBO::CreateRenderBuffer(void* header /* = nullptr */)
{
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: No shared context; the stream must create one first");
    return nullptr;
  }

  // Framebuffer objects are not shared between contexts, so one built outside
  // the client's context would be useless to it.
  if (m_clientFrameDepth == 0 || m_clientThread != std::this_thread::get_id())
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Refusing to build a buffer outside the client's context");
    return nullptr;
  }

  auto buffer = std::make_unique<CRenderBufferFBO>(m_context, m_contextProperties.depth,
                                                   m_contextProperties.stencil,
                                                   m_contextProperties.bottomLeftOrigin);
  m_resources.emplace_back(buffer->m_resources);
  return buffer.release();
}

bool CRenderBufferPoolFBO::CreateContext(const HwContextProperties& properties)
{
  std::unique_lock lock(m_contextMutex);
  if (m_eglContext != EGL_NO_CONTEXT)
    DestroyContext();
  if (m_eglContext != EGL_NO_CONTEXT || !SupportsHardwareRendering())
    return false;
#if defined(HAS_GLES)
  if (!properties.embedded || properties.versionMajor < 3)
    return false;
#else
  if (properties.embedded)
    return false;
#endif

  m_contextProperties = properties;

  auto winSystem =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());
  if (winSystem == nullptr)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Window system does not use EGL");
    return false;
  }

  m_eglDisplay = winSystem->GetEGLDisplay();

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CLog::Log(LOGERROR, "failed to get EGL display");
    return false;
  }

  CRestoreEGLAPI restoreAPI;
  if (!eglBindAPI(CLIENT_API))
    return false;

  // clang-format off

  EGLint attribs[] =
  {
#if defined(HAS_GLES)
    // ES3 rather than ES2: the framebuffer objects, the depth and stencil
    // attachments and the sampling this pool relies on are all core in ES3.
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#else
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
    // The context is only ever made current without a surface, so the surface
    // type is a formality -- but eglChooseConfig defaults it to EGL_WINDOW_BIT
    // and matches on it either way, so it has to name something the platform
    // really offers. Window is the one every platform Kodi runs on provides.
    // Pbuffer is not: on GBM, configs come from the GBM formats and advertise
    // window only, so asking for a pbuffer matches nothing at all and every
    // hardware core fails to get a context.
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE
  };

  EGLint neglconfigs;
  if (!eglChooseConfig(m_eglDisplay, attribs, &m_eglConfig, 1, &neglconfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of EGL configs");
    return false;
  }

  if (neglconfigs <= 0)
  {
    CLog::Log(LOGERROR, "No suitable EGL configs found");
    return false;
  }

  // clang-format on

  // In libretro the version a client asks for is a minimum, so try later ones
  // first and fall back to the request. ES only: its minor versions are purely
  // additive, whereas a desktop GL version interacts with the profile below.
  std::vector<std::pair<unsigned int, unsigned int>> versions;
  if (!properties.embedded && (properties.versionMajor < 3 ||
                               (properties.versionMajor == 3 && properties.versionMinor < 2)))
  {
    versions.emplace_back(3, 2);
  }
  else if (properties.versionMajor != 0)
  {
    if (properties.embedded && properties.versionMajor == 3)
    {
      for (unsigned int minor = 2; minor > properties.versionMinor; --minor)
        versions.emplace_back(3, minor);
    }
    versions.emplace_back(properties.versionMajor, properties.versionMinor);
  }
  else
  {
    // Nothing asked for, so let the driver decide
    versions.emplace_back(0, 0);
  }

  const std::string apiName = properties.embedded      ? "OpenGL ES"
                              : properties.coreProfile ? "OpenGL core profile"
                                                       : "OpenGL compatibility profile";

  // Ask the driver for each in turn rather than keeping a table of what it
  // supports. A refusal fails the stream cleanly and the client falls back.
  std::string contextName;
  for (const auto& [major, minor] : versions)
  {
    std::vector<EGLint> contextAttribs;

    if (major != 0)
    {
      contextAttribs.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
      contextAttribs.push_back(static_cast<EGLint>(major));
      contextAttribs.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
      contextAttribs.push_back(static_cast<EGLint>(minor));
    }

    if (!properties.embedded)
    {
      contextAttribs.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
      contextAttribs.push_back(properties.coreProfile
                                   ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
                                   : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR);
    }

    contextAttribs.push_back(EGL_NONE);

    contextName = apiName;
    if (major != 0)
      contextName += StringUtils::Format(" {}.{}", major, minor);

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, winSystem->GetEGLContext(),
                                    contextAttribs.data());
    if (m_eglContext != EGL_NO_CONTEXT)
    {
      CLog::Log(LOGINFO,
                "RetroPlayer[RENDER]: Created a {} context for the game client, sharing Kodi's "
                "objects",
                contextName);
      break;
    }
  }

  if (m_eglContext == EGL_NO_CONTEXT)
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Game client asked for a {} context, which this system cannot "
              "provide (EGL error {:#x})",
              contextName, eglGetError());
    return false;
  }

  // Not made current here: a binding is per-thread, and the thread that opened
  // the stream can be Kodi's own rendering thread, which would lose the window
  // surface it presents with. BeginClientFrame() binds it around the work.
  return true;
}

IRenderBuffer* CRenderBufferPoolFBO::GetBuffer(unsigned int width, unsigned int height)
{
  std::unique_lock lock(m_contextMutex, std::try_to_lock);
  if (!lock.owns_lock() || m_clientFrameDepth == 0 || m_clientThread != std::this_thread::get_id())
    return nullptr;
  CollectBuffers();
  return CBaseRenderBufferPool::GetBuffer(width, height);
}

void CRenderBufferPoolFBO::Return(IRenderBuffer* buffer)
{
  auto* fbo = static_cast<CRenderBufferFBO*>(buffer);
  {
    auto lock = fbo->Lock();
    std::unique_lock captureLock(m_captureMutex);
    if (fbo->TextureID() != 0 && !fbo->m_resources->retired &&
        fbo->TextureWidth() == m_captureWidth && fbo->TextureHeight() == m_captureHeight)
    {
      // Pool matching uses allocation size; published frame size can be smaller.
      fbo->SetSize(fbo->TextureWidth(), fbo->TextureHeight());
      CBaseRenderBufferPool::Return(buffer);
      return;
    }
  }
  delete buffer;
}

void CRenderBufferPoolFBO::CollectBuffers()
{
  for (auto it = m_resources.begin(); it != m_resources.end();)
  {
    if (it->use_count() == 1)
    {
      (*it)->Destroy();
      it = m_resources.erase(it);
    }
    else
      ++it;
  }
}

bool CRenderBufferPoolFBO::BeginClientFrame()
{
  if (!m_contextMutex.try_lock())
    return false;
  if (m_eglContext == EGL_NO_CONTEXT)
  {
    m_contextMutex.unlock();
    return false;
  }
  if (m_clientFrameDepth > 0)
  {
    ++m_clientFrameDepth;
    return true;
  }

  m_prevAPI = eglQueryAPI();
  m_prevDisplay = eglGetCurrentDisplay();
  m_prevDraw = eglGetCurrentSurface(EGL_DRAW);
  m_prevRead = eglGetCurrentSurface(EGL_READ);
  m_prevContext = eglGetCurrentContext();

  if (!eglBindAPI(CLIENT_API) ||
      !eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to bind client context (EGL error {:#x})",
              eglGetError());
    eglBindAPI(m_prevAPI);
    m_contextMutex.unlock();
    return false;
  }
  m_clientThread = std::this_thread::get_id();
  m_clientFrameDepth = 1;
  CollectBuffers();
  return true;
}

void CRenderBufferPoolFBO::EndClientFrame()
{
  if (--m_clientFrameDepth == 0)
  {
    glFlush();
    if (m_prevAPI != CLIENT_API)
      eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglBindAPI(m_prevAPI);
    const EGLBoolean restored =
        m_prevContext != EGL_NO_CONTEXT
            ? eglMakeCurrent(m_prevDisplay, m_prevDraw, m_prevRead, m_prevContext)
            : eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!restored)
    {
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to restore EGL context (error {:#x})",
                eglGetError());
      // A failed restore must not leave later Kodi draws on the client context.
      eglBindAPI(CLIENT_API);
      eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      eglBindAPI(m_prevAPI);
    }
    m_clientThread = {};
    m_prevDisplay = EGL_NO_DISPLAY;
    m_prevContext = EGL_NO_CONTEXT;
    m_prevDraw = m_prevRead = EGL_NO_SURFACE;
  }
  m_contextMutex.unlock();
}

IRenderBuffer* CRenderBufferPoolFBO::CaptureClientFrame(IRenderBuffer* clientBuffer,
                                                        unsigned int width,
                                                        unsigned int height)
{
  std::unique_lock lock(m_contextMutex);
  if (m_clientFrameDepth == 0 || m_clientThread != std::this_thread::get_id() || !clientBuffer ||
      width == 0 || height == 0)
    return nullptr;

  auto* client = static_cast<CRenderBufferFBO*>(clientBuffer);
  if (width > client->TextureWidth() || height > client->TextureHeight() ||
      client->GetCurrentFramebuffer() == 0)
    return nullptr;

  {
    std::unique_lock captureLock(m_captureMutex);
    if (width != m_captureWidth || height != m_captureHeight)
    {
      m_captureWidth = width;
      m_captureHeight = height;
      CBaseRenderBufferPool::Flush();
      Configure(AV_PIX_FMT_NONE);
    }
  }

  auto* target = static_cast<CRenderBufferFBO*>(GetBuffer(width, height));
  if (!target)
    return nullptr;

  target->PrepareForCapture();
  GLint prevRead = 0, prevDraw = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDraw);
  const GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
  glDisable(GL_SCISSOR_TEST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, client->GetCurrentFramebuffer());
  GLint prevReadBuffer = 0;
  glGetIntegerv(GL_READ_BUFFER, &prevReadBuffer);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->GetCurrentFramebuffer());
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  const bool ready = target->SetReady();
  glReadBuffer(prevReadBuffer);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, prevRead);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDraw);
  if (scissorEnabled)
    glEnable(GL_SCISSOR_TEST);

  if (!ready)
  {
    target->Release();
    return nullptr;
  }
  return target;
}

void CRenderBufferPoolFBO::DestroyContext()
{
  std::unique_lock lock(m_contextMutex);
  if (m_eglContext == EGL_NO_CONTEXT)
    return;
  if (!m_resources.empty() || m_clientFrameDepth > 0)
  {
    if (!BeginClientFrame())
    {
      CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Unable to bind context for resource destruction");
      return;
    }

    // Outstanding buffers remain valid CPU objects, but can no longer be drawn.
    for (const auto& resources : m_resources)
      resources->Destroy();
    m_resources.clear();
    while (m_clientFrameDepth > 0)
      EndClientFrame();
  }
  {
    std::unique_lock captureLock(m_captureMutex);
    m_captureWidth = m_captureHeight = 0;
    Flush();
  }

  if (!eglDestroyContext(m_eglDisplay, m_eglContext))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to destroy EGL context (error {:#x})",
              eglGetError());
    return;
  }
  m_eglContext = EGL_NO_CONTEXT;
  m_eglDisplay = EGL_NO_DISPLAY;
}
