/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/ShaderPreset.h"

#include <d3d11.h>

namespace KODI
{
namespace RETRO
{
class CRenderContext;
}

namespace SHADER
{
class IShader;
class IShaderTexture;

class CShaderPresetDX : public CShaderPreset
{
public:
  // Instance of CShaderPreset
  explicit CShaderPresetDX(RETRO::CRenderContext& context,
                           unsigned videoWidth = 0,
                           unsigned videoHeight = 0);
  ~CShaderPresetDX() override = default;

protected:
  // Implementation of CShaderPreset
  bool CreateShaders() override;
  bool CreateLayouts() override;
  bool CreateBuffers() override;
  bool CreateShaderTextures() override;
  bool CreateSamplers() override;
  void RenderShader(IShader& shader, IShaderTexture& source, IShaderTexture& target) override;

private:
  // Point/nearest neighbor sampler
  ID3D11SamplerState* m_pSampNearest = nullptr;

  // Linear sampler
  ID3D11SamplerState* m_pSampLinear = nullptr;
};

} // namespace SHADER
} // namespace KODI
