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

#include "cores/RetroPlayer/buffers/BaseRenderBuffer.h"

#include <stdint.h>
#include <vector>

extern "C" {
#include "libavutil/pixfmt.h"
}

namespace KODI
{
namespace RETRO
{
  class CRenderBufferSysMem : public CBaseRenderBuffer
  {
  public:
    CRenderBufferSysMem() = default;
    virtual ~CRenderBufferSysMem() = default;

    // implementation of IRenderBuffer
    bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
    size_t GetFrameSize() const override;
    uint8_t *GetMemory() override;

    // Utility functions
    static size_t GetBufferSize(AVPixelFormat format, unsigned int width, unsigned int height);

  protected:
    std::vector<uint8_t> m_data;
  };

}
}
