/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtilsDX.h"

using namespace KODI::SHADER;

D3D11_TEXTURE_ADDRESS_MODE CShaderUtilsDX::TranslateWrapType(WrapType wrapType)
{
  D3D11_TEXTURE_ADDRESS_MODE dxWrap;
  switch (wrapType)
  {
    case WrapType::EDGE:
      dxWrap = D3D11_TEXTURE_ADDRESS_CLAMP;
      break;
    case WrapType::REPEAT:
      dxWrap = D3D11_TEXTURE_ADDRESS_WRAP;
      break;
    case WrapType::MIRRORED_REPEAT:
      dxWrap = D3D11_TEXTURE_ADDRESS_MIRROR;
      break;
    case WrapType::BORDER:
    default:
      dxWrap = D3D11_TEXTURE_ADDRESS_BORDER;
      break;
  }
  return dxWrap;
}

DirectX::XMFLOAT2 CShaderUtilsDX::ToDXVector(const float2& vec)
{
  return DirectX::XMFLOAT2(static_cast<float>(vec.x), static_cast<float>(vec.y));
}
