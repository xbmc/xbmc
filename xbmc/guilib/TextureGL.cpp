/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureGL.h"

#include "ServiceBroker.h"
#include "Texture.h"
#include "guilib/TextureManager.h"
#include "rendering/RenderSystem.h"
#include "settings/AdvancedSettings.h"
#include "utils/GLUtils.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

CTextureGL::CTextureGL(unsigned int width, unsigned int height, unsigned int format)
  : CTextureGLBase(width, height, format)
{
  unsigned int major, minor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
  if (major >= 3)
  {
    m_isOglVersion3orNewer = true;
  }
}

void CTextureGL::LoadToGPU()
{
  if (!m_pixels)
  {
    // nothing to load - probably same image (no change)
    return;
  }
  if (m_texture == 0)
  {
    // Have OpenGL generate a texture object handle for us
    // this happens only one time - the first time the texture is loaded
    CreateTextureObject();
  }

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, m_texture);

  GLenum filter = (m_scalingMethod == TEXTURE_SCALING::NEAREST ? GL_NEAREST : GL_LINEAR);

  // Set the texture's stretching properties
  if (IsMipmapped())
  {
    GLenum mipmapFilter = (m_scalingMethod == TEXTURE_SCALING::NEAREST ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapFilter);

    // Lower LOD bias equals more sharpness, but less smooth animation
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.5f);
    if (!m_isOglVersion3orNewer)
    {
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (m_textureHeight > m_maxSize)
  {
    CLog::Log(LOGERROR,
              "GL: Image height %d too big to fit into single texture unit, truncating to %u",
              m_textureHeight, m_maxSize);
    m_textureHeight = m_maxSize;
  }

  if (m_textureWidth > m_maxSize)
  {
    CLog::Log(LOGERROR,
              "GL: Image width %d too big to fit into single texture unit, truncating to %u",
              m_textureWidth, m_maxSize);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_textureWidth);
    m_textureWidth = m_maxSize;
  }

  GLenum format = GL_BGRA;
  GLint numcomponents = GL_RGBA;

  switch (m_format)
  {
  case XB_FMT_DXT1:
    format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    break;
  case XB_FMT_DXT3:
    format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    break;
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    break;
  case XB_FMT_RGB8:
    format = GL_RGB;
    numcomponents = GL_RGB;
    break;
  case XB_FMT_A8R8G8B8:
  default:
    break;
  }

  if ((m_format & XB_FMT_DXT_MASK) == 0)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, numcomponents, m_textureWidth, m_textureHeight, 0, format,
                 GL_UNSIGNED_BYTE, m_pixels);
  }
  else
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, m_textureWidth, m_textureHeight, 0,
                           GetPitch() * GetRows(), m_pixels);
  }

  if (IsMipmapped() && m_isOglVersion3orNewer)
  {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  VerifyGLState();

  if (!m_bCacheMemory)
  {
    KODI::MEMORY::AlignedFree(m_pixels);
    m_pixels = NULL;
  }

  m_loadedToGPU = true;
}
