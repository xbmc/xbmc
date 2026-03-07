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

class CShaderTextureGL : public IShaderTexture
{
public:
  CShaderTextureGL(uint32_t textureWidth,
                   uint32_t textureHeight,
                   GLuint pixelType,
                   GLuint internalFormat,
                   GLuint pixelFormat,
                   bool bUseAlpha);
  ~CShaderTextureGL() override;

  // Disallow copy and move (this object owns raw GL IDs)
  CShaderTextureGL(const CShaderTextureGL&) = delete;
  CShaderTextureGL& operator=(const CShaderTextureGL&) = delete;
  CShaderTextureGL(CShaderTextureGL&&) = delete;
  CShaderTextureGL& operator=(CShaderTextureGL&&) = delete;

  // Implementation of IShaderTexture
  float GetWidth() const override;
  float GetHeight() const override;

  // OpenGL interface
  void CreateTexture();
  void DeleteTexture();
  void BindToUnit(unsigned int unit);
  GLuint GetTextureID() const { return m_texture; }

  void SetSRGBFramebuffer() { m_sRgbFramebuffer = true; }
  bool IsSRGBFramebuffer() const { return m_sRgbFramebuffer; }

  void SetMipmapping() { m_mipmapping = true; }
  bool IsMipmapped() const { return m_mipmapping; }

  // Frame buffer interface
  void CreateFBO();
  void DeleteFBO();
  bool BindFBO();
  void UnbindFBO() const;

private:
  // Construction parameters
  const uint32_t m_textureWidth;
  const uint32_t m_textureHeight;
  const GLuint m_pixelType;
  const GLuint m_internalFormat;
  const GLuint m_pixelFormat;
  const bool m_useAlpha;

  const GLenum m_textureTarget = GL_TEXTURE_2D; //! @todo
  GLuint m_texture{0};
  GLuint m_FBO{0};
  bool m_sRgbFramebuffer{false};
  bool m_mipmapping{false};
};
} // namespace KODI::SHADER
