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

  class CRenderBufferOpenGLES : public CRenderBufferSysMem
  {
  public:
    CRenderBufferOpenGLES(CRenderContext &context,
                          GLuint pixeltype,
                          GLuint internalformat,
                          GLuint pixelformat,
                          GLuint bpp);
    ~CRenderBufferOpenGLES() override;

    // implementation of IRenderBuffer via CRenderBufferSysMem
    bool UploadTexture() override;

    GLuint TextureID() const { return m_textureId; }

  private:
    // Construction parameters
    CRenderContext &m_context;
    const GLuint m_pixeltype;
    const GLuint m_internalformat;
    const GLuint m_pixelformat;
    const GLuint m_bpp;

    const GLenum m_textureTarget = GL_TEXTURE_2D; //! @todo
    GLuint m_textureId = 0;

    void CreateTexture();
    void DeleteTexture();
  };
}
}
