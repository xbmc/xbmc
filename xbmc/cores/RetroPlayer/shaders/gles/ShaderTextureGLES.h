/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderTexture.h"

#include <memory>

#include "system_gl.h"

class CGLESTexture;

namespace KODI::SHADER
{

class CShaderTextureGLES : public IShaderTexture
{
public:
  CShaderTextureGLES(std::shared_ptr<CGLESTexture> texture, bool sRgbFramebuffer);
  ~CShaderTextureGLES() override;

  // Implementation of IShaderTexture
  float GetWidth() const override;
  float GetHeight() const override;

  // OpenGLES interface
  CGLESTexture& GetTexture() { return *m_texture; }
  const CGLESTexture& GetTexture() const { return *m_texture; }
  bool IsSRGBFramebuffer() const { return m_sRgbFramebuffer; }

  // Frame buffer interface
  bool CreateFBO();
  bool BindFBO();
  void UnbindFBO() const;

private:
  std::shared_ptr<CGLESTexture> m_texture;
  bool m_sRgbFramebuffer{false};
  GLuint FBO{0};
};
} // namespace KODI::SHADER
