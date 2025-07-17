/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderLut.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"

#include <memory>
#include <string>

#include "system_gl.h"

namespace KODI::SHADER
{
class CTextureBase;
struct ShaderLut;

class CShaderLutGLES : public IShaderLut
{
public:
  CShaderLutGLES(std::string id, std::string path);
  ~CShaderLutGLES() override = default;

  // Implementation of IShaderLut
  bool Create(const ShaderLut& lut) override;
  CTexture* GetTexture() override { return m_texture.get(); }

private:
  static std::unique_ptr<CTexture> CreateLUTTexture(const ShaderLut& lut);

  std::unique_ptr<CTexture> m_texture;
};

} // namespace KODI::SHADER
