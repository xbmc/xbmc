/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/ShaderPreset.h"

namespace KODI
{
namespace RETRO
{
class CRenderContext;
}

namespace SHADER
{

class CShaderPresetGL : public CShaderPreset
{
public:
  // Instance of CShaderPreset
  explicit CShaderPresetGL(RETRO::CRenderContext& context,
                           unsigned videoWidth = 0,
                           unsigned videoHeight = 0);
  ~CShaderPresetGL() override = default;

protected:
  // Implementation of CShaderPreset
  bool CreateShaders() override;
  bool CreateLayouts() override { return true; }
  bool CreateBuffers() override { return true; }
  bool CreateShaderTextures() override;
  bool CreateSamplers() override { return true; }
  void RenderShader(IShader& shader, IShaderTexture& source, IShaderTexture& target) override;
};

} // namespace SHADER
} // namespace KODI
