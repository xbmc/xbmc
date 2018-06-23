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

extern "C" {
#include "libavutil/pixfmt.h"
}

#include <memory>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
  class IRenderBufferPool;

  class IRenderBuffer
  {
  public:
    virtual ~IRenderBuffer() { }

    // Pool functions
    virtual void Acquire() = 0;
    virtual void Acquire(std::shared_ptr<IRenderBufferPool> pool) = 0;
    virtual void Release() = 0;
    virtual IRenderBufferPool *GetPool() = 0;

    // Buffer functions
    virtual bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) = 0;
    virtual void Update() { } //! @todo Remove me
    virtual size_t GetFrameSize() const = 0;
    virtual uint8_t *GetMemory() = 0;
    virtual void ReleaseMemory() { }
    virtual bool UploadTexture() = 0;
    virtual void BindToUnit(unsigned int unit) { }
    virtual void SetHeader(void *header) { }

    // Buffer properties
    AVPixelFormat GetFormat() const { return m_format; }
    unsigned int GetWidth() const { return m_width; }
    unsigned int GetHeight() const { return m_height; }
    bool IsLoaded() const { return m_bLoaded; }
    void SetLoaded(bool bLoaded) { m_bLoaded = bLoaded; }
    bool IsRendered() const { return m_bRendered; }
    void SetRendered(bool bRendered) { m_bRendered = bRendered; }

  protected:
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    bool m_bLoaded = false;
    bool m_bRendered = false;
  };
}
}
