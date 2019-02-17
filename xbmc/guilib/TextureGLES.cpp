/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureGLES.h"

#include "ServiceBroker.h"
#include "guilib/TextureManager.h"
#include "rendering/RenderSystem.h"
#include "utils/GLUtils.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

CTextureGLES::CTextureGLES(unsigned int width, unsigned int height, unsigned int format)
  : CTextureGLBase(width, height, format)
{
}

void CTextureGLES::LoadToGPU()
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
    GLenum mipmapFilter = (m_scalingMethod == TEXTURE_SCALING::NEAREST ? GL_LINEAR_MIPMAP_NEAREST
                                                                       : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapFilter);
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
    m_textureWidth = m_maxSize;
  }

  // All incoming textures are BGRA, which GLES does not necessarily support.
  // Some (most?) hardware supports BGRA textures via an extension.
  // If not, we convert to RGBA first to avoid having to swizzle in shaders.
  // Explicitly define GL_BGRA_EXT here in the case that it's not defined by
  // system headers, and trust the extension list instead.

//! @todo remove in the future (broken headers are broken)
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

  GLint internalformat;
  GLenum pixelformat;

  switch (m_format)
  {
    case XB_FMT_RGBA8:
      internalformat = pixelformat = GL_RGBA;
      break;
    case XB_FMT_RGB8:
      internalformat = pixelformat = GL_RGB;
      break;
    default:
    case XB_FMT_A8R8G8B8:
    {
      if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_texture_format_BGRA8888") ||
          CServiceBroker::GetRenderSystem()->IsExtSupported("GL_IMG_texture_format_BGRA8888"))
      {
        internalformat = pixelformat = GL_BGRA_EXT;
      }
      else if (CServiceBroker::GetRenderSystem()->IsExtSupported(
                   "GL_APPLE_texture_format_BGRA8888"))
      {
        // Apple's implementation does not conform to spec. Instead, they require
        // differing format/internalformat, more like GL.
        internalformat = GL_RGBA;
        pixelformat = GL_BGRA_EXT;
      }
      else
      {
        SwapBlueRed(m_pixels, m_textureHeight, GetPitch());
        internalformat = pixelformat = GL_RGBA;
      }
      break;
    }
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internalformat, m_textureWidth, m_textureHeight, 0, pixelformat,
               GL_UNSIGNED_BYTE, m_pixels);

  if (IsMipmapped())
  {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  VerifyGLState();

  if (!m_bCacheMemory)
  {
    KODI::MEMORY::AlignedFree(m_pixels);
    m_pixels = nullptr;
  }

  m_loadedToGPU = true;
}
