/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureGL.h"

#include "ServiceBroker.h"
#include "guilib/TextureManager.h"
#include "rendering/RenderSystem.h"
#include "settings/AdvancedSettings.h"
#include "utils/GLUtils.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

CTexture* CTexture::CreateTexture(unsigned int width, unsigned int height, unsigned int format)
{
  return new CGLTexture(width, height, format);
}

CGLTexture::CGLTexture(unsigned int width, unsigned int height, unsigned int format)
  : CTexture(width, height, format)
{
  unsigned int major, minor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
  if (major >= 3)
    m_isOglVersion3orNewer = true;
}

CGLTexture::~CGLTexture()
{
  DestroyTextureObject();
}

void CGLTexture::CreateTextureObject()
{
  glGenTextures(1, (GLuint*) &m_texture);
}

void CGLTexture::DestroyTextureObject()
{
  if (m_texture)
    CServiceBroker::GetGUI()->GetTextureManager().ReleaseHwTexture(m_texture);
}

void CGLTexture::LoadToGPU()
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

#ifndef HAS_GLES
    // Lower LOD bias equals more sharpness, but less smooth animation
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.5f);
    if (!m_isOglVersion3orNewer)
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
#endif
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  unsigned int maxSize = CServiceBroker::GetRenderSystem()->GetMaxTextureSize();
  if (m_textureHeight > maxSize)
  {
    CLog::Log(LOGERROR,
              "GL: Image height {} too big to fit into single texture unit, truncating to {}",
              m_textureHeight, maxSize);
    m_textureHeight = maxSize;
  }
  if (m_textureWidth > maxSize)
  {
    CLog::Log(LOGERROR,
              "GL: Image width {} too big to fit into single texture unit, truncating to {}",
              m_textureWidth, maxSize);
#ifndef HAS_GLES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_textureWidth);
#endif
    m_textureWidth = maxSize;
  }

#ifndef HAS_GLES
  GLenum format = GL_BGRA;
  GLint numcomponents = GL_RGBA;
  bool uncompressed = false;

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
    uncompressed = true;
    break;
  case XB_FMT_ETC1:
  case XB_FMT_ETC2_RGB:
    format = GL_COMPRESSED_RGB8_ETC2;
    break;
  case XB_FMT_ETC2_RGBA:
    format = GL_COMPRESSED_RGBA8_ETC2_EAC;
    break;
  case XB_FMT_ETC2_R:
    format = GL_COMPRESSED_R11_EAC;
    break;
  case XB_FMT_ASTC_4x4:
    format = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
    break;
  case XB_FMT_ASTC_8x8:
    format = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
    break;
  case XB_FMT_A8R8G8B8:
  default:
    uncompressed = true;
    break;
  }

  if (uncompressed)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, numcomponents,
                 m_textureWidth, m_textureHeight, 0,
                 format, GL_UNSIGNED_BYTE, m_pixels);
  }
  else
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format,
                           m_textureWidth, m_textureHeight, 0,
                           GetPitch() * GetRows(), m_pixels);
  }

  if (IsMipmapped() && m_isOglVersion3orNewer)
  {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

#else	// GLES version

  // All incoming textures are BGRA, which GLES does not necessarily support.
  // Some (most?) hardware supports BGRA textures via an extension.
  // If not, we convert to RGBA first to avoid having to swizzle in shaders.
  // Explicitly define GL_BGRA_EXT here in the case that it's not defined by
  // system headers, and trust the extension list instead.
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

  GLint internalformat;
  GLenum pixelformat;
  bool uncompressed = false;

  switch (m_format)
  {
    default:
    case XB_FMT_RGBA8:
      internalformat = pixelformat = GL_RGBA;
      uncompressed = true;
      break;
    case XB_FMT_RGB8:
      internalformat = pixelformat = GL_RGB;
      uncompressed = true;
      break;
    case XB_FMT_A8R8G8B8:
      if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_texture_format_BGRA8888") ||
          CServiceBroker::GetRenderSystem()->IsExtSupported("GL_IMG_texture_format_BGRA8888"))
      {
        internalformat = pixelformat = GL_BGRA_EXT;
      }
      else if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_APPLE_texture_format_BGRA8888"))
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
      uncompressed = true;
      break;
    case XB_FMT_ETC1:
      internalformat = GL_ETC1_RGB8_OES;
      break;
    case XB_FMT_ETC2_RGB:
      internalformat = GL_COMPRESSED_RGB8_ETC2;
      break;
    case XB_FMT_ETC2_RGBA:
      internalformat = GL_COMPRESSED_RGBA8_ETC2_EAC;
      break;
    case XB_FMT_ETC2_R:
      internalformat = GL_COMPRESSED_R11_EAC;
      break;
    case XB_FMT_ASTC_4x4:
      internalformat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
      break;
    case XB_FMT_ASTC_8x8:
      internalformat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
      break;
  }

  if (uncompressed)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, m_textureWidth, m_textureHeight, 0,
                  pixelformat, GL_UNSIGNED_BYTE, m_pixels);
  }
  else
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalformat,
                            m_textureWidth, m_textureHeight, 0,
                            GetPitch() * GetRows(), m_pixels);
  }

  if (IsMipmapped())
  {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

#endif
  VerifyGLState();

  if (!m_bCacheMemory)
  {
    KODI::MEMORY::AlignedFree(m_pixels);
    m_pixels = NULL;
  }

  m_loadedToGPU = true;
}

void CGLTexture::BindToUnit(unsigned int unit)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, m_texture);
}

bool CGLTexture::IsTexSupported(uint32_t textureFormat) const
{
  unsigned int renderVersionMajor;
  unsigned int renderVersionMinor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(renderVersionMajor, renderVersionMinor);
#ifndef HAS_GLES
  switch(textureFormat)
  {
    case XB_FMT_UNKNOWN:
    case XB_FMT_A8R8G8B8:
    case XB_FMT_A8:
    case XB_FMT_RGBA8:
    case XB_FMT_RGB8:
    case XB_FMT_OPAQUE:
      return true;
    case XB_FMT_ETC1:
    case XB_FMT_ETC2_R:
    case XB_FMT_ETC2_RGB:
    case XB_FMT_ETC2_RGBA:
      return (renderVersionMajor > 4 || (renderVersionMajor == 4 && renderVersionMinor >= 3));
    case XB_FMT_ASTC_4x4:
    case XB_FMT_ASTC_8x8:
      return (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_KHR_texture_compression_astc_ldr"));
    case XB_FMT_DXT1:
    case XB_FMT_DXT3:
    case XB_FMT_DXT5:
    case XB_FMT_DXT5_YCoCg:
      return (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_texture_compression_s3tc"));
    default:
      return false;
  }
#else	// GLES version
  switch(textureFormat)
  {
    case XB_FMT_UNKNOWN:
    case XB_FMT_A8R8G8B8:
    case XB_FMT_A8:
    case XB_FMT_RGBA8:
    case XB_FMT_RGB8:
    case XB_FMT_OPAQUE:
      return true;
    case XB_FMT_ETC1:
      return (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_OES_compressed_ETC1_RGB8_texture"));
    case XB_FMT_ETC2_R:
    case XB_FMT_ETC2_RGB:
    case XB_FMT_ETC2_RGBA:
      return (renderVersionMajor >= 3);
    case XB_FMT_ASTC_4x4:
    case XB_FMT_ASTC_8x8:
      return (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_KHR_texture_compression_astc_ldr") || CServiceBroker::GetRenderSystem()->IsExtSupported("GL_OES_texture_compression_astc"));
    case XB_FMT_DXT1:
    case XB_FMT_DXT3:
    case XB_FMT_DXT5:
    case XB_FMT_DXT5_YCoCg:
      return (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_texture_compression_s3tc"));
    default:
      return false;
  }
#endif
}
