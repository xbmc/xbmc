/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "utils/EGLImage.h"
#include "utils/Geometry.h"

#include <array>

#include "system_gl.h"

class CDRMPRIMETexture
{
public:
  explicit CDRMPRIMETexture(EGLDisplay eglDisplay);
  CDRMPRIMETexture() = delete;
  ~CDRMPRIMETexture();

  bool Map(CVideoBufferDRMPRIME* buffer);
  void Unmap();
  void Init(EGLDisplay eglDisplay);

  GLenum GetTextureTarget() const { return m_textureTarget; }

  GLuint GetTextureY() { return m_texture[0]; }
  GLuint GetTextureU() { return m_texture[1]; }
  GLuint GetTextureV() { return m_texture[2]; }

  CSizeInt GetTextureSize() { return {m_texWidth, m_texHeight}; }

  int GetTextureBits() const { return m_textureBits; }

protected:
  CVideoBufferDRMPRIME* m_primebuffer{nullptr};
  std::array<std::unique_ptr<CEGLImage>, 3> m_eglImage;

  GLenum m_textureTarget{GL_TEXTURE_EXTERNAL_OES};
  std::array<GLuint, 3> m_texture{0};
  int m_texWidth{0};
  int m_texHeight{0};

  int m_textureBits{8};
};
