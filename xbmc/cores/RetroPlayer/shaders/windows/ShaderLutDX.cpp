/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderLutDX.h"

#include "ShaderUtilsDX.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "guilib/TextureDX.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"

#include <utility>

using namespace KODI::SHADER;

CShaderLutDX::CShaderLutDX(std::string id, std::string path)
  : IShaderLut(std::move(id), std::move(path))
{
}

CShaderLutDX::~CShaderLutDX() = default;

bool CShaderLutDX::Create(const ShaderLut& lut)
{
  std::unique_ptr<CTexture> lutTexture{CreateLUTexture(lut)};
  if (!lutTexture)
  {
    CLog::LogF(LOGWARNING, "CShaderLutDX::Create: Couldn't create texture for LUT: {}", lut.strId);
    return false;
  }

  m_texture = std::move(lutTexture);
  return true;
}

std::unique_ptr<CTexture> CShaderLutDX::CreateLUTexture(const ShaderLut& lut)
{
  std::unique_ptr<CTexture> texture = CTexture::LoadFromFile(lut.path);
  auto* textureDX = static_cast<CDXTexture*>(texture.get());

  if (textureDX == nullptr)
  {
    CLog::Log(LOGERROR, "CShaderLutDX::CreateLUTexture: Couldn't open LUT: {}", lut.path);
    return std::unique_ptr<CTexture>();
  }

  if (lut.mipmap)
    textureDX->SetMipmapping();

  textureDX->SetScalingMethod(lut.filterType == FilterType::LINEAR ? TEXTURE_SCALING::LINEAR
                                                                   : TEXTURE_SCALING::NEAREST);
  textureDX->LoadToGPU();

  //! @todo Set LUT wrap type
  //D3D11_TEXTURE_ADDRESS_MODE wrapType = CShaderUtilsDX::TranslateWrapType(lut.wrapType);

  return texture;
}
