/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Texture.h"

#include "system_gl.h"

struct TextureFormat
{
  GLenum internalFormat{GL_FALSE};
  GLenum internalFormatSRGB{GL_FALSE};
  GLint format{GL_FALSE};
  GLenum type{GL_UNSIGNED_BYTE};
};

struct TextureSwizzle
{
  GLint r{GL_RED};
  GLint g{GL_GREEN};
  GLint b{GL_BLUE};
  GLint a{GL_ALPHA};
};

class CGLESTexture : public CTexture
{
public:
  CGLESTexture(unsigned int width = 0, unsigned int height = 0, XB_FMT format = XB_FMT_A8R8G8B8);
  ~CGLESTexture() override;

  void CreateTextureObject() override;
  void DestroyTextureObject() override;
  void LoadToGPU() override;
  void BindToUnit(unsigned int unit) override;
  bool SupportsFormat(KD_TEX_FMT textureFormat, KD_TEX_SWIZ textureSwizzle) override;

protected:
  void SetSwizzle(bool swapRB);
  void SwapBlueRedSwizzle(GLint& component);
  TextureFormat GetFormatGLES20(KD_TEX_FMT textureFormat);
  TextureFormat GetFormatGLES30(KD_TEX_FMT textureFormat);

  GLuint m_texture = 0;
  bool m_isGLESVersion30orNewer{false};
};
