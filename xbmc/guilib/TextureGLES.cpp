/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureGLES.h"

#include "ServiceBroker.h"
#include "guilib/TextureManager.h"
#include "rendering/RenderSystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

#include <memory>

std::unique_ptr<CTexture> CTexture::CreateTexture(unsigned int width,
                                                  unsigned int height,
                                                  XB_FMT format)
{
  return std::make_unique<CGLESTexture>(width, height, format);
}

CGLESTexture::CGLESTexture(unsigned int width, unsigned int height, XB_FMT format)
  : CTexture(width, height, format)
{
  unsigned int major, minor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
  m_isGLESVersion30orNewer = major >= 3;
}

CGLESTexture::~CGLESTexture()
{
  DestroyTextureObject();
}

void CGLESTexture::CreateTextureObject()
{
  glGenTextures(1, (GLuint*)&m_texture);
}

void CGLESTexture::DestroyTextureObject()
{
  if (m_texture)
    CServiceBroker::GetGUI()->GetTextureManager().ReleaseHwTexture(m_texture);
}

void CGLESTexture::LoadToGPU()
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

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
  if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_texture_filter_anisotropic"))
  {
    int32_t aniso =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAnisotropicFiltering;
    if (aniso > 1)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
  }
#endif

  unsigned int maxSize = CServiceBroker::GetRenderSystem()->GetMaxTextureSize();

  if (m_textureHeight > maxSize)
  {
    CLog::LogF(LOGERROR,
               "Image height {} too big to fit into single texture unit, truncating to {}",
               m_textureHeight, maxSize);
    m_textureHeight = maxSize;
  }

  if (m_textureWidth > maxSize)
  {
#if defined(GL_PACK_ROW_LENGTH)
    if (m_isGLESVersion30orNewer)
    {
      CLog::LogF(LOGERROR,
                 "Image width {} too big to fit into single texture unit, truncating to {}",
                 m_textureWidth, maxSize);

      glPixelStorei(GL_UNPACK_ROW_LENGTH, m_textureWidth);

      m_textureWidth = maxSize;
    }
    else
#endif
    {
      CLog::LogF(LOGERROR, "Image width {} too big, upload to GPU will fail", m_textureWidth);
    }
  }

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

  glTexImage2D(GL_TEXTURE_2D, 0, internalformat, m_textureWidth, m_textureHeight, 0, pixelformat,
               GL_UNSIGNED_BYTE, m_pixels);

  if (IsMipmapped())
  {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

#if defined(GL_UNPACK_ROW_LENGTH)
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

  VerifyGLState();

  if (!m_bCacheMemory)
  {
    KODI::MEMORY::AlignedFree(m_pixels);
    m_pixels = NULL;
  }

  m_loadedToGPU = true;
}

void CGLESTexture::BindToUnit(unsigned int unit)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, m_texture);
}
