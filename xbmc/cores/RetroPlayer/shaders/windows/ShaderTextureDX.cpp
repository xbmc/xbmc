/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderTextureDX.h"

#include "guilib/D3DResource.h"

#include <cassert>
#include <utility>

using namespace KODI::SHADER;

CShaderTextureDX::CShaderTextureDX(std::shared_ptr<CD3DTexture> texture)
  : m_texture(std::move(texture))
{
  assert(m_texture.get() != nullptr);
}

float CShaderTextureDX::GetWidth() const
{
  return static_cast<float>(m_texture->GetWidth());
}

float CShaderTextureDX::GetHeight() const
{
  return static_cast<float>(m_texture->GetHeight());
}

ID3D11ShaderResourceView* CShaderTextureDX::GetShaderResource() const
{
  return m_texture->GetShaderResource();
}
