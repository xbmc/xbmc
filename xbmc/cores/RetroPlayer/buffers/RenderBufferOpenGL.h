/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
class CRenderContext;

class CRenderBufferOpenGL : public CRenderBufferSysMem
{
public:
  CRenderBufferOpenGL(GLuint pixelType,
                      GLuint internalFormat,
                      GLuint pixelFormat,
                      GLuint bpp,
                      bool hardware = false,
                      bool depth = false,
                      bool stencil = false);
  ~CRenderBufferOpenGL() override;

  // Implementation of IRenderBuffer via CRenderBufferSysMem
  bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
  uintptr_t GetCurrentFramebuffer() override { return m_fboId; }
  bool UploadTexture() override;

  GLuint TextureID() const { return m_textureId; }

  //! \brief True if this buffer is a hardware-rendered FBO the game core draws into
  bool IsHardware() const { return m_bHardware; }

private:
  // Construction parameters
  const GLuint m_pixelType;
  const GLuint m_internalFormat;
  const GLuint m_pixelFormat;
  const GLuint m_bpp;

  // Hardware-rendering parameters
  const bool m_bHardware;
  const bool m_depth;
  const bool m_stencil;

  const GLenum m_textureTarget = GL_TEXTURE_2D; //! @todo
  GLuint m_textureId = 0;

  // Hardware-rendering resources (FBO the game core renders into)
  GLuint m_fboId = 0;
  GLuint m_depthStencilRbo = 0;

  void CreateTexture();
  void DeleteTexture();

  //! \brief Create the FBO (color + optional depth/stencil) for hardware rendering
  bool CreateFramebuffer();
  void DeleteFramebuffer();
};
} // namespace RETRO
} // namespace KODI
