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
#pragma once

#include "IRenderBufferPool.h"
#include "threads/CriticalSection.h"

#include <deque>
#include <memory>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class CBaseRenderBufferPool : public IRenderBufferPool
  {
  public:
    CBaseRenderBufferPool() = default;
    ~CBaseRenderBufferPool() override;

    // Partial implementation of IRenderBufferPool
    void RegisterRenderer(CRPBaseRenderer *renderer) override;
    void UnregisterRenderer(CRPBaseRenderer *renderer) override;
    bool HasVisibleRenderer() const override;
    bool Configure(AVPixelFormat format, unsigned int width, unsigned int height) override;
    bool IsConfigured() const override { return m_bConfigured; }
    IRenderBuffer *GetBuffer(size_t size) override;
    void Return(IRenderBuffer *buffer) override;
    void Prime(size_t bufferSize) override;
    void Flush() override;

    AVPixelFormat Format() const { return m_format; }
    unsigned int Width() const { return m_width; }
    unsigned int Height() const { return m_height; }
    size_t FrameSize() const { return m_frameSize; }

  protected:
    virtual IRenderBuffer *CreateRenderBuffer(void *header = nullptr) = 0;
    virtual bool ConfigureInternal() { return true; }
    virtual void *GetHeader(unsigned int timeoutMs = 0) { return nullptr; }
    virtual bool GetHeaderWithTimeout(void *&header)
    {
      header = nullptr;
      return true;
    }
    virtual bool SendBuffer(IRenderBuffer *buffer) { return false; }

    // Configuration parameters
    bool m_bConfigured = false;
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    unsigned int m_width = 0;
    unsigned int m_height = 0;

    // Stream properties
    size_t m_frameSize = 0; // Initially unknown, set on first call to GetBuffer()

  private:
    // Buffer properties
    std::deque<std::unique_ptr<IRenderBuffer>> m_free;

    std::vector<CRPBaseRenderer*> m_renderers;
    CCriticalSection m_rendererMutex;
    CCriticalSection m_bufferMutex;
  };
}
}
