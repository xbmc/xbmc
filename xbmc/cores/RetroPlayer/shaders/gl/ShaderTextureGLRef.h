/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderTexture.h"

#include "system_gl.h"

namespace KODI::SHADER
{

/*!
 * \brief Shader texture that wraps an external texture object
 *
 * NOTE: The lifetime of the external texture object must outlast this class.
 */
class CShaderTextureGLRef : public IShaderTexture
{
public:
  CShaderTextureGLRef(uint32_t textureWidth, uint32_t textureHeight, GLuint texture = 0);
  ~CShaderTextureGLRef() override = default;

  // Implementation of IShaderTexture
  float GetWidth() const override;
  float GetHeight() const override;

  // OpenGL interface
  void BindToUnit(unsigned int unit);
  GLuint GetTextureID() const { return m_texture; }

private:
  uint32_t m_textureWidth{0};
  uint32_t m_textureHeight{0};
  GLuint m_texture{0};
};
} // namespace KODI::SHADER
