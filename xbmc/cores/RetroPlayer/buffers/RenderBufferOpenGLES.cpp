/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferOpenGLES.h"

#include "cores/RetroPlayer/rendering/RenderContext.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferOpenGLES::CRenderBufferOpenGLES(CRenderContext& context,
                                             GLuint pixeltype,
                                             GLuint internalformat,
                                             GLuint pixelformat,
                                             GLuint bpp)
  : m_context(context),
    m_pixeltype(pixeltype),
    m_internalformat(internalformat),
    m_pixelformat(pixelformat),
    m_bpp(bpp)
{
}

CRenderBufferOpenGLES::~CRenderBufferOpenGLES()
{
  DeleteTexture();
}

void CRenderBufferOpenGLES::CreateTexture()
{
  gl::GenTextures(1, &m_textureId);

  gl::BindTexture(m_textureTarget, m_textureId);

  gl::TexImage2D(m_textureTarget, 0, m_internalformat, m_width, m_height, 0, m_pixelformat,
                 m_pixeltype, NULL);

  gl::TexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  gl::BindTexture(m_textureTarget, 0);
}

bool CRenderBufferOpenGLES::UploadTexture()
{
  if (!gl::IsTexture(m_textureId))
    CreateTexture();

  gl::BindTexture(m_textureTarget, m_textureId);

  const int stride = GetFrameSize() / m_height;

  gl::PixelStorei(GL_UNPACK_ALIGNMENT, m_bpp);

  if (m_bpp == 4 && m_pixelformat == GL_RGBA)
  {
    // XOR Swap RGBA -> BGRA
    // GLES 2.0 doesn't support strided textures (unless GL_UNPACK_ROW_LENGTH_EXT is supported)
    uint8_t* pixels = const_cast<uint8_t*>(m_data.data());
    for (unsigned int y = 0; y < m_height; ++y, pixels += stride)
    {
      for (int x = 0; x < stride; x += 4)
        std::swap(pixels[x], pixels[x + 2]);
      gl::TexSubImage2D(m_textureTarget, 0, 0, y, m_width, 1, m_pixelformat, m_pixeltype, pixels);
    }
  }
  else if (m_context.IsExtSupported("GL_EXT_unpack_subimage"))
  {
#ifdef GL_UNPACK_ROW_LENGTH_EXT
    gl::PixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride / m_bpp);
    gl::TexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, m_pixelformat, m_pixeltype,
                      m_data.data());
    gl::PixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
#endif
  }
  else
  {
    uint8_t* pixels = const_cast<uint8_t*>(m_data.data());
    for (unsigned int y = 0; y < m_height; ++y, pixels += stride)
      gl::TexSubImage2D(m_textureTarget, 0, 0, y, m_width, 1, m_pixelformat, m_pixeltype, pixels);
  }

  gl::BindTexture(m_textureTarget, 0);

  return true;
}

void CRenderBufferOpenGLES::DeleteTexture()
{
  if (gl::IsTexture(m_textureId))
    gl::DeleteTextures(1, &m_textureId);

  m_textureId = 0;
}
