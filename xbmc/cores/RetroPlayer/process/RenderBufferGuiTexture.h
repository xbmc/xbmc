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
#include "cores/IPlayer.h"
#include "guilib/Texture.h"
#include "guilib/TextureFormats.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
  class CRenderBufferGuiTexture : public CBaseRenderBuffer
  {
  public:
    CRenderBufferGuiTexture(ESCALINGMETHOD scalingMethod);
    virtual ~CRenderBufferGuiTexture() = default;

    // implementation of IRenderBuffer via CBaseRenderBuffer
    bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int size) override;
    size_t GetFrameSize() const override;
    uint8_t *GetMemory() override;
    bool UploadTexture() override;
    void BindToUnit(unsigned int unit) override;

    // GUI texture interface
    CTexture *GetTexture() { return m_texture.get(); }

  protected:
    AVPixelFormat TranslateFormat(unsigned int textureFormat);
    TEXTURE_SCALING TranslateScalingMethod(ESCALINGMETHOD scalingMethod);

    // Texture parameters
    ESCALINGMETHOD m_scalingMethod;
    unsigned int m_textureFormat = XB_FMT_UNKNOWN;
    std::unique_ptr<CTexture> m_texture;
  };

}
}
