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

#include "BaseRenderBufferPool.h"
#include "IRenderBuffer.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CBaseRenderBufferPool::~CBaseRenderBufferPool()
{
  Flush();
}

void CBaseRenderBufferPool::RegisterRenderer(CRPBaseRenderer *renderer)
{
  CSingleLock lock(m_rendererMutex);

  m_renderers.push_back(renderer);
}

void CBaseRenderBufferPool::UnregisterRenderer(CRPBaseRenderer *renderer)
{
  CSingleLock lock(m_rendererMutex);

  m_renderers.erase(std::remove(m_renderers.begin(), m_renderers.end(), renderer), m_renderers.end());
}

bool CBaseRenderBufferPool::HasVisibleRenderer() const
{
  CSingleLock lock(m_rendererMutex);

  for (auto renderer : m_renderers)
  {
    if (renderer->IsVisible())
      return true;
  }

  return false;
}

bool CBaseRenderBufferPool::Configure(AVPixelFormat format)
{
  m_format = format;

  if (ConfigureInternal())
    m_bConfigured = true;

  return m_bConfigured;
}

IRenderBuffer *CBaseRenderBufferPool::GetBuffer(unsigned int width, unsigned int height)
{
  if (!m_bConfigured)
    return nullptr;

  IRenderBuffer *renderBuffer = nullptr;

  void *header = nullptr;

  if (GetHeaderWithTimeout(header))
  {
    CSingleLock lock(m_bufferMutex);

    while (!m_free.empty())
    {
      renderBuffer = m_free.front().release();
      m_free.pop_front();

      // Only return buffers of the same dimensions
      const unsigned int bufferWidth = renderBuffer->GetWidth();
      const unsigned int bufferHeight = renderBuffer->GetHeight();

      if (bufferWidth == width && bufferHeight == height)
      {
        renderBuffer->SetHeader(header);
        break;
      }
      else
      {
        CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Discarding render buffer of size %ux%u",
                  bufferWidth,
                  bufferHeight);

        std::unique_ptr<IRenderBuffer> bufferPtr(renderBuffer);
        renderBuffer = nullptr;
      }
    }

    if (renderBuffer == nullptr)
    {
      CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Creating render buffer of size %ux%u for buffer pool",
                width,
                height);

      std::unique_ptr<IRenderBuffer> renderBufferPtr(CreateRenderBuffer(header));
      if (renderBufferPtr->Allocate(m_format, width, height))
        renderBuffer = renderBufferPtr.release();
      else
        CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to allocate render buffer");
    }

    if (renderBuffer != nullptr)
    {
      renderBuffer->Acquire(GetPtr());
      renderBuffer->Update();
    }
  }

  return renderBuffer;
}

void CBaseRenderBufferPool::Return(IRenderBuffer *buffer)
{
  CSingleLock lock(m_bufferMutex);

  buffer->SetLoaded(false);
  buffer->SetRendered(false);

  std::unique_ptr<IRenderBuffer> bufferPtr(buffer);
  m_free.emplace_back(std::move(bufferPtr));
}

void CBaseRenderBufferPool::Prime(unsigned int width, unsigned int height)
{
  CSingleLock lock(m_bufferMutex);

  // Allocate two buffers for double buffering
  unsigned int bufferCount = 2;

  std::vector<IRenderBuffer*> buffers;

  for (unsigned int i = 0; i < bufferCount; i++)
  {
    IRenderBuffer *buffer = GetBuffer(width, height);
    if (buffer == nullptr)
      break;

    if (!SendBuffer(buffer))
      buffers.emplace_back(buffer);
  }

  for (auto buffer : buffers)
    buffer->Release();
}

void CBaseRenderBufferPool::Flush()
{
  CSingleLock lock(m_bufferMutex);

  m_free.clear();
  m_bConfigured = false;
}
