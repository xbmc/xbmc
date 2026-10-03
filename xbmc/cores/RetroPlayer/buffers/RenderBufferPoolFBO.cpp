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
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererFBO.h"
#include "cores/RetroPlayer/rendering/contexts/IHwRenderingContext.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace RETRO;

CRenderBufferPoolFBO::CRenderBufferPoolFBO(CRenderContext& context)
  : CRenderBufferPoolFBO(context, CreateHwRenderingContext(context))
{
}

CRenderBufferPoolFBO::CRenderBufferPoolFBO(CRenderContext& context,
                                           std::unique_ptr<IHwRenderingContext> hwContext)
  : m_context(context),
    m_hwContext(std::move(hwContext))
{
}

CRenderBufferPoolFBO::~CRenderBufferPoolFBO()
{
  DestroyContext();
}

bool CRenderBufferPoolFBO::SupportsHardwareRendering() const
{
  std::unique_lock lock(m_contextMutex);
  return m_hwContext && m_hwContext->SupportsHardwareRendering();
}

bool CRenderBufferPoolFBO::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return CRPRendererFBO::SupportsScalingMethod(renderSettings.GetScalingMethod());
}

IRenderBuffer* CRenderBufferPoolFBO::CreateRenderBuffer(void* header /* = nullptr */)
{
  return CreateFBO(CRenderBufferFBO::Type::CAPTURE);
}

bool CRenderBufferPoolFBO::ConfigureInternal()
{
  // Hardware-rendered streams carry no CPU-side pixel format. Software ones
  // declare a real one and belong to the DMA and sysmem pools.
  return m_format == AV_PIX_FMT_NONE;
}

CRenderBufferFBO* CRenderBufferPoolFBO::CreateFBO(CRenderBufferFBO::Type type)
{
  if (!m_hwContext || !m_hwContext->IsCreated())
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

  const bool client = type == CRenderBufferFBO::Type::CLIENT;
  auto buffer = std::make_unique<CRenderBufferFBO>(m_context, client && m_contextProperties.depth,
                                                   client && m_contextProperties.stencil,
                                                   m_contextProperties.bottomLeftOrigin, type);
  m_resources.emplace_back(buffer->m_resources);
  return buffer.release();
}

bool CRenderBufferPoolFBO::CreateContext(const HwContextProperties& properties)
{
  std::unique_lock lock(m_contextMutex);
  if (!m_hwContext)
    return false;
  if (m_hwContext->IsCreated())
    DestroyContext();
  if (m_hwContext->IsCreated() || !SupportsHardwareRendering())
    return false;

  m_contextProperties = properties;
  if (!m_hwContext->Create(properties))
  {
    m_hwContext->Destroy();
    return false;
  }
  return true;
}

IRenderBuffer* CRenderBufferPoolFBO::GetBuffer(unsigned int width, unsigned int height)
{
  std::unique_lock lock(m_contextMutex, std::try_to_lock);
  if (!lock.owns_lock() || m_clientFrameDepth == 0 || m_clientThread != std::this_thread::get_id())
    return nullptr;
  if (!IsConfigured())
    return nullptr;
  CollectBuffers();

  // The stable client framebuffer retains alpha and never enters the capture pool.
  std::unique_ptr<CRenderBufferFBO> buffer(CreateFBO(CRenderBufferFBO::Type::CLIENT));
  if (!buffer || !buffer->Allocate(AV_PIX_FMT_NONE, width, height))
    return nullptr;
  buffer->Acquire(GetPtr());
  buffer->Update();
  return buffer.release();
}

void CRenderBufferPoolFBO::Return(IRenderBuffer* buffer)
{
  auto* fbo = static_cast<CRenderBufferFBO*>(buffer);
  {
    auto lock = fbo->Lock();
    std::unique_lock captureLock(m_captureMutex);
    if (fbo->IsCapture() && fbo->TextureID() != 0 && !fbo->m_resources->retired &&
        fbo->TextureWidth() == m_captureWidth && fbo->TextureHeight() == m_captureHeight)
    {
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

void CRenderBufferPoolFBO::Flush()
{
  std::unique_lock captureLock(m_captureMutex);
  CBaseRenderBufferPool::Flush();
}

bool CRenderBufferPoolFBO::BeginClientFrame()
{
  if (!m_contextMutex.try_lock())
    return false;
  if (!m_hwContext || !m_hwContext->IsCreated())
  {
    m_contextMutex.unlock();
    return false;
  }
  if (m_clientFrameDepth > 0)
  {
    ++m_clientFrameDepth;
    return true;
  }

  if (!m_hwContext->MakeCurrent())
  {
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
    m_hwContext->RestoreCurrent();
    m_clientThread = {};
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

  CollectBuffers();
  auto* target = static_cast<CRenderBufferFBO*>(GetCaptureBuffer(width, height));
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
  // Normalize the drawn rectangle before directional shader filters see it.
  const GLint sourceY0 = client->BottomLeftOrigin() ? height : 0;
  const GLint sourceY1 = client->BottomLeftOrigin() ? 0 : height;
  glBlitFramebuffer(0, sourceY0, width, sourceY1, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);
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

IRenderBuffer* CRenderBufferPoolFBO::GetCaptureBuffer(unsigned int width, unsigned int height)
{
  // The caller owns the context; exclude Flush() through configuration and allocation.
  std::unique_lock captureLock(m_captureMutex);
  if (width != m_captureWidth || height != m_captureHeight)
  {
    m_captureWidth = width;
    m_captureHeight = height;
    CBaseRenderBufferPool::Flush();
  }
  if (!Configure(AV_PIX_FMT_NONE))
    return nullptr;

  return CBaseRenderBufferPool::GetBuffer(width, height);
}

void CRenderBufferPoolFBO::DestroyContext()
{
  std::unique_lock lock(m_contextMutex);
  if (!m_hwContext || !m_hwContext->IsCreated())
    return;
  if (!m_resources.empty() || m_clientFrameDepth > 0)
  {
    if (!BeginClientFrame())
    {
      CLog::Log(LOGERROR,
                "RetroPlayer[RENDER]: Client context lost or unbindable; abandoning GPU resources");
      for (const auto& resources : m_resources)
        resources->Abandon();
    }
    else
    {
      // Outstanding buffers remain valid CPU objects, but can no longer be drawn.
      for (const auto& resources : m_resources)
        resources->Destroy();
      while (m_clientFrameDepth > 0)
        EndClientFrame();
    }
    m_resources.clear();
  }
  {
    std::unique_lock captureLock(m_captureMutex);
    m_captureWidth = m_captureHeight = 0;
    CBaseRenderBufferPool::Flush();
  }

  m_hwContext->Destroy();
}
