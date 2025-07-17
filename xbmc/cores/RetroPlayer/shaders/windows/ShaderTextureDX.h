/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderTexture.h"

#include <memory>

#include <d3d11.h>

class CD3DTexture;

namespace KODI::SHADER
{

/*!
 * \brief Shader texture that manages the lifetime of its texture object
 */
class CShaderTextureDX : public IShaderTexture
{
public:
  explicit CShaderTextureDX(std::shared_ptr<CD3DTexture> texture);
  ~CShaderTextureDX() override = default;

  // Implementation of IShaderTexture
  float GetWidth() const override;
  float GetHeight() const override;

  // DirectX interface
  CD3DTexture& GetTexture() { return *m_texture; }
  const CD3DTexture& GetTexture() const { return *m_texture; }
  ID3D11ShaderResourceView* GetShaderResource() const;

private:
  // Construction parameter
  const std::shared_ptr<CD3DTexture> m_texture;
};

} // namespace KODI::SHADER
