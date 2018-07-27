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


CRenderBufferOpenGL::CRenderBufferOpenGL(CRenderContext &context,
                                         GLuint pixeltype,
                                         GLuint internalformat,
                                         GLuint pixelformat,
                                         GLuint bpp) :
  CRenderBufferOpenGLES(context,
                        pixeltype,
                        internalformat,
                        pixelformat,
                        bpp)
{
}

bool CRenderBufferOpenGL::UploadTexture()
{
  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  const int stride = GetFrameSize() / m_height;

  glPixelStorei(GL_UNPACK_ALIGNMENT, m_bpp);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / m_bpp);
  glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, m_pixelformat, m_pixeltype, m_data.data());
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  return true;
}
