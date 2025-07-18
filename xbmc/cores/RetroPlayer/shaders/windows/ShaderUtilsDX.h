/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/ShaderTypes.h"

#include <DirectXMath.h>
#include <d3d11.h>

namespace KODI::SHADER
{
class CShaderUtilsDX
{
public:
  static D3D11_TEXTURE_ADDRESS_MODE TranslateWrapType(WrapType wrapType);
  static DirectX::XMFLOAT2 ToDXVector(const float2& vec);
};
} // namespace KODI::SHADER
