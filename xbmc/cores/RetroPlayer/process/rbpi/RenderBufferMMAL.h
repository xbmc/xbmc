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

#include "cores/RetroPlayer/process/BaseRenderBuffer.h"

#include <interface/mmal/mmal.h>

#include <memory>
#include <stdint.h>

class CGPUMEM;

namespace KODI
{
namespace RETRO
{
  class CRenderBufferMMAL : public CBaseRenderBuffer
  {
  public:
    CRenderBufferMMAL(MMAL_BUFFER_HEADER_T *buffer);
    ~CRenderBufferMMAL() override;

    // Partial implementation of IRenderBuffer
    void Update() override;
    size_t GetFrameSize() const override;
    uint8_t *GetMemory() override;
    bool UploadTexture() override { return true; }
    void SetHeader(void *header) override;

    // MMAL interface
    MMAL_BUFFER_HEADER_T *GetHeader() { return m_mmal_buffer; }
    void ResetHeader();

  protected:
    // GPU interface
    virtual CGPUMEM *GetGPUBuffer() const = 0;

  private:
    // Construction parameters
    MMAL_BUFFER_HEADER_T *m_mmal_buffer = nullptr;
  };

  class CRenderBufferMMALRGB : public CRenderBufferMMAL
  {
  public:
    CRenderBufferMMALRGB(MMAL_BUFFER_HEADER_T *buffer);
    ~CRenderBufferMMALRGB() override = default;

    // Partial implementation of IRenderBuffer via CRenderBufferMMAL
    bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int size) override;

  protected:
    // implementation of CRenderBufferMMAL
    CGPUMEM *GetGPUBuffer() const override { return m_gmem.get(); }

  private:
    std::unique_ptr<CGPUMEM> m_gmem;
  };
}
}
