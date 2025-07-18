/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderSampler.h"

#include <d3d11.h>

namespace KODI::SHADER
{

class CShaderSamplerDX : public IShaderSampler
{
public:
  CShaderSamplerDX(ID3D11SamplerState* sampler);
  ~CShaderSamplerDX() override;

private:
  ID3D11SamplerState* const m_sampler;
};

} // namespace KODI::SHADER
