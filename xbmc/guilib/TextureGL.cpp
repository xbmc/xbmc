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

#include <memory>

#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#endif
#if defined(HAS_GLES)
#include "rendering/gles/RenderSystemGLES.h"
#endif

std::unique_ptr<CTexture> CTexture::CreateTexture(unsigned int width,
                                                  unsigned int height,
                                                  unsigned int format)
{
  return std::make_unique<CGLTexture>(width, height, format);
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
  gl::GenTextures(1, (GLuint*)&m_texture);
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
  gl::BindTexture(GL_TEXTURE_2D, m_texture);

  GLenum filter = (m_scalingMethod == TEXTURE_SCALING::NEAREST ? GL_NEAREST : GL_LINEAR);

#ifdef HAS_GL
  auto renderSystemGL = dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem());
#endif

  // Set the texture's stretching properties
  if (IsMipmapped())
  {
    GLenum mipmapFilter = (m_scalingMethod == TEXTURE_SCALING::NEAREST ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapFilter);

#ifdef HAS_GL
    if (renderSystemGL)
    {
      // Lower LOD bias equals more sharpness, but less smooth animation
      gl::TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.5f);
      if (!m_isOglVersion3orNewer)
        gl::TexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }
#endif
  }
  else
  {
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  }

  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
#if HAS_GL
    if (renderSystemGL)
      gl::PixelStorei(GL_UNPACK_ROW_LENGTH, m_textureWidth);
#endif
    m_textureWidth = maxSize;
  }

#ifdef HAS_GL
  if (renderSystemGL)
  {
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
      gl::TexImage2D(GL_TEXTURE_2D, 0, numcomponents, m_textureWidth, m_textureHeight, 0, format,
                     GL_UNSIGNED_BYTE, m_pixels);
    }
    else
    {
      gl::CompressedTexImage2D(GL_TEXTURE_2D, 0, format, m_textureWidth, m_textureHeight, 0,
                               GetPitch() * GetRows(), m_pixels);
    }

    if (IsMipmapped() && m_isOglVersion3orNewer)
    {
      gl::GenerateMipmap(GL_TEXTURE_2D);
    }

    gl::PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  }
#endif
#ifdef HAS_GLES // GLES version

  // All incoming textures are BGRA, which GLES does not necessarily support.
  // Some (most?) hardware supports BGRA textures via an extension.
  // If not, we convert to RGBA first to avoid having to swizzle in shaders.
  // Explicitly define GL_BGRA_EXT here in the case that it's not defined by
  // system headers, and trust the extension list instead.
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

  auto renderSystemGLES = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());

  if (renderSystemGLES)
  {
    GLint internalformat;
    GLenum pixelformat;

    switch (m_format)
    {
      default:
      case XB_FMT_RGBA8:
        internalformat = pixelformat = GL_RGBA;
        break;
      case XB_FMT_RGB8:
        internalformat = pixelformat = GL_RGB;
        break;
      case XB_FMT_A8R8G8B8:
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
    gl::TexImage2D(GL_TEXTURE_2D, 0, internalformat, m_textureWidth, m_textureHeight, 0,
                   pixelformat, GL_UNSIGNED_BYTE, m_pixels);

    if (IsMipmapped())
    {
      gl::GenerateMipmap(GL_TEXTURE_2D);
    }
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
  gl::ActiveTexture(GL_TEXTURE0 + unit);
  gl::BindTexture(GL_TEXTURE_2D, m_texture);
}

