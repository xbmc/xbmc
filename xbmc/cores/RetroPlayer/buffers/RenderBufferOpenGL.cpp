/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferOpenGL.h"

#include "cores/RetroPlayer/rendering/RenderContext.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferOpenGL::CRenderBufferOpenGL(GLuint pixeltype,
                                         GLuint internalformat,
                                         GLuint pixelformat,
                                         GLuint bpp)
  : m_pixeltype(pixeltype), m_internalformat(internalformat), m_pixelformat(pixelformat), m_bpp(bpp)
{
}

CRenderBufferOpenGL::~CRenderBufferOpenGL()
{
  DeleteTexture();
}

void CRenderBufferOpenGL::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);

  glTexImage2D(m_textureTarget, 0, m_internalformat, m_width, m_height, 0, m_pixelformat,
               m_pixeltype, NULL);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(m_textureTarget, 0);
}

bool CRenderBufferOpenGL::UploadTexture()
{
  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  const int stride = GetFrameSize() / m_height;

  glPixelStorei(GL_UNPACK_ALIGNMENT, m_bpp);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / m_bpp);

  //! @todo This is subject to change:
  //! We want to use PBO's instead of glTexSubImage2D!
  //! This code has been borrowed from OpenGL ES in order
  //! to remove GL dependencies on GLES.
  glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, m_pixelformat, m_pixeltype,
                  m_data.data());
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  return true;
}

void CRenderBufferOpenGL::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}
