/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "BaseRenderBuffer.h"

#include <stdint.h>
#include <vector>

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
    bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int size) override;
    size_t GetFrameSize() const override;
    uint8_t *GetMemory() override;

  protected:
    std::vector<uint8_t> m_data;
  };

}
}
