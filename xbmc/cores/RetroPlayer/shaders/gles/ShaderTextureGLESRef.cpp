/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderTextureGLESRef.h"

using namespace KODI::SHADER;

CShaderTextureGLESRef::CShaderTextureGLESRef(uint32_t textureWidth,
                                             uint32_t textureHeight,
                                             GLuint texture)
  : m_textureWidth(textureWidth),
    m_textureHeight(textureHeight),
    m_texture(texture)
{
}

float CShaderTextureGLESRef::GetWidth() const
{
  return static_cast<float>(m_textureWidth);
}

float CShaderTextureGLESRef::GetHeight() const
{
  return static_cast<float>(m_textureHeight);
}

void CShaderTextureGLESRef::BindToUnit(unsigned int unit)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, m_texture);
}
