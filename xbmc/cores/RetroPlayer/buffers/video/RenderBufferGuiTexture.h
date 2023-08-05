/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "cores/RetroPlayer/buffers/BaseRenderBuffer.h"
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
  CRenderBufferGuiTexture(SCALINGMETHOD scalingMethod);
  ~CRenderBufferGuiTexture() override = default;

  // implementation of IRenderBuffer via CBaseRenderBuffer
  bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
  size_t GetFrameSize() const override;
  uint8_t* GetMemory() override;
  bool UploadTexture() override;
  void BindToUnit(unsigned int unit) override;

  // GUI texture interface
  CTexture* GetTexture() { return m_texture.get(); }

protected:
  AVPixelFormat TranslateFormat(XB_FMT textureFormat);
  TEXTURE_SCALING TranslateScalingMethod(SCALINGMETHOD scalingMethod);

  // Texture parameters
  SCALINGMETHOD m_scalingMethod;
  XB_FMT m_textureFormat = XB_FMT_UNKNOWN;
  std::unique_ptr<CTexture> m_texture;
};

} // namespace RETRO
} // namespace KODI
