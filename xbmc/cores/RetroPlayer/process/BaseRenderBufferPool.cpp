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

bool CBaseRenderBufferPool::Configure(AVPixelFormat format, unsigned int width, unsigned int height)
{
  m_format = format;
  m_width = width;
  m_height = height;

  if (ConfigureInternal())
    m_bConfigured = true;

  return m_bConfigured;
}

IRenderBuffer *CBaseRenderBufferPool::GetBuffer(size_t size)
{
  if (!m_bConfigured)
    return nullptr;

  // If frame size is unknown, set it now
  if (m_frameSize == 0)
    m_frameSize = size;

  // Changing sizes is not implemented
  if (m_frameSize != size)
    return nullptr;

  IRenderBuffer *renderBuffer = nullptr;

  void *header = nullptr;

  if (GetHeaderWithTimeout(header))
  {
    CSingleLock lock(m_bufferMutex);

    if (!m_free.empty())
    {
      renderBuffer = m_free.front().release();
      m_free.pop_front();
      renderBuffer->SetHeader(header);
    }
    else
    {
      std::unique_ptr<IRenderBuffer> renderBufferPtr(CreateRenderBuffer(header));
      if (renderBufferPtr->Allocate(m_format, m_width, m_height, m_frameSize))
        renderBuffer = renderBufferPtr.release();
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

  m_free.emplace_back(buffer);
}

void CBaseRenderBufferPool::Prime(size_t bufferSize)
{
  CSingleLock lock(m_bufferMutex);

  if (m_frameSize != bufferSize)
    return;

  // Allocate two buffers for double buffering
  unsigned int bufferCount = 2;

  std::vector<IRenderBuffer*> buffers;

  for (unsigned int i = 0; i < bufferCount; i++)
  {
    IRenderBuffer *buffer = GetBuffer(0);
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
}
