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

#include "cores/RetroPlayer/process/BaseRenderBufferPool.h"
#include "threads/CriticalSection.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

#include <interface/mmal/mmal.h>

namespace KODI
{
namespace RETRO
{
  class CRenderBufferMMAL;
  class IRenderBuffer;

  class CRenderBufferPoolMMAL : public CBaseRenderBufferPool
  {
  public:
    CRenderBufferPoolMMAL() = default;
    ~CRenderBufferPoolMMAL() override { Deinitialize(); }

    // implementation of IRenderBufferPool via CBaseRenderBufferPool
    bool IsCompatible(const CRenderVideoSettings &renderSettings) const override;

    // MMAL interface
    MMAL_FOURCC_T Encoding() const { return m_mmal_format; }
    unsigned int AlignedWidth() const { return m_alignedWidth; }
    unsigned int AlignedHeight() const { return m_alignedHeight; }

  protected:
    // implementation of CBaseRenderBufferPool
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;
    bool ConfigureInternal() override;
    void *GetHeader(unsigned int timeoutMs = 0) override;
    bool GetHeaderWithTimeout(void *&header) override;
    bool SendBuffer(IRenderBuffer *buffer) override;

  private:
    bool InitializeMMAL();
    void Deinitialize();

    IRenderBuffer *GetBufferWithTimeout(unsigned int timeoutMs);

    // Stream properties
    MMAL_FOURCC_T m_mmal_format = MMAL_ENCODING_UNKNOWN;
    unsigned int m_alignedWidth = 0;
    unsigned int m_alignedHeight = 0;

    // MMAL properties
    //MMALState m_state;
    bool m_input = false; //! @todo Needed?
    MMAL_POOL_T *m_mmal_pool = nullptr;
    MMAL_COMPONENT_T *m_component = nullptr;

    // Synchronization
    CCriticalSection m_mutex;
  };
}
}
