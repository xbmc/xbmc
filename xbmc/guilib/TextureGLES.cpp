/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureGLES.h"

#include "ServiceBroker.h"
#include "guilib/TextureFormats.h"
#include "guilib/TextureGLESFormatMap.h"
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

  TextureFormatGLES glesFormat;
  if (m_isGLESVersion30orNewer)
  {
    KD_TEX_FMT textureFormat = m_textureFormat;
    bool swapRB = false;
    // Support for BGRA is hit and miss, swizzle instead
    if (textureFormat == KD_TEX_FMT_SDR_BGRA8)
    {
      textureFormat = KD_TEX_FMT_SDR_RGBA8;
      swapRB = true;
    }
    SetSwizzle(swapRB);
    glesFormat = GetFormatGLES30(textureFormat);
  }
  else
  {
    glesFormat = GetFormatGLES20(m_textureFormat);
  }

  if (glesFormat.internalFormat == GL_FALSE)
  {
    CLog::LogF(LOGDEBUG, "Failed to load texture. Unsupported format {}", m_textureFormat);
    m_loadedToGPU = true;
    return;
  }

  if ((m_textureFormat & KD_TEX_FMT_SDR) || (m_textureFormat & KD_TEX_FMT_HDR))
  {
    glTexImage2D(GL_TEXTURE_2D, 0, glesFormat.internalFormat, m_textureWidth, m_textureHeight, 0,
                 glesFormat.format, glesFormat.type, m_pixels);
  }
  else
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, glesFormat.internalFormat, m_textureWidth,
                           m_textureHeight, 0, GetPitch() * GetRows(), m_pixels);
  }
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

void CGLESTexture::SetSwizzle(bool swapRB)
{
#if defined(GL_ES_VERSION_3_0)
  TextureSwizzleGLES swiz;

  const auto it = SwizzleMapGLES.find(m_textureSwizzle);
  if (it != SwizzleMapGLES.cend())
    swiz = it->second;

  if (swapRB)
  {
    SwapBlueRedSwizzle(swiz.r);
    SwapBlueRedSwizzle(swiz.g);
    SwapBlueRedSwizzle(swiz.b);
    SwapBlueRedSwizzle(swiz.a);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, swiz.r);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, swiz.g);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, swiz.b);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, swiz.a);
#endif
}

void CGLESTexture::SwapBlueRedSwizzle(GLint& component)
{
  if (component == GL_RED)
    component = GL_BLUE;
  else if (component == GL_BLUE)
    component = GL_RED;
}

TextureFormatGLES CGLESTexture::GetFormatGLES20(KD_TEX_FMT textureFormat)
{
  TextureFormatGLES glFormat;

  // GLES 2.0 does not support swizzling. But for some Kodi formats+swizzles,
  // we can map GLES formats (Luminance, Luminance-Alpha, BGRA). Other swizzles
  // would have to be supported in the shader, or converted before upload.

  if (m_textureFormat == KD_TEX_FMT_SDR_R8 && m_textureSwizzle == KD_TEX_SWIZ_RRR1)
  {
    glFormat.format = glFormat.internalFormat = GL_LUMINANCE;
  }
  else if (m_textureFormat == KD_TEX_FMT_SDR_RG8 && m_textureSwizzle == KD_TEX_SWIZ_RRRG)
  {
    glFormat.format = glFormat.internalFormat = GL_LUMINANCE_ALPHA;
  }
  else if (m_textureFormat == KD_TEX_FMT_SDR_BGRA8 && m_textureSwizzle == KD_TEX_SWIZ_RGBA &&
           !CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_texture_format_BGRA8888") &&
           !CServiceBroker::GetRenderSystem()->IsExtSupported("GL_IMG_texture_format_BGRA8888"))
  {
#if defined(GL_APPLE_texture_format_BGRA8888)
    if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_APPLE_texture_format_BGRA8888"))
    {
      glFormat.internalFormat = GL_RGBA;
      glFormat.format = GL_BGRA_EXT;
    }
    else
#endif
    {
      SwapBlueRed(m_pixels, m_textureHeight, GetPitch());
      glFormat.format = glFormat.internalFormat = GL_RGBA;
    }
  }
  else if (textureFormat & KD_TEX_FMT_SDR || textureFormat & KD_TEX_FMT_HDR ||
           textureFormat & KD_TEX_FMT_ETC1)
  {
    const auto it = TextureMappingGLES20.find(textureFormat);
    if (it != TextureMappingGLES20.cend())
      glFormat = it->second;
    glFormat.format = glFormat.internalFormat;
  }
  else
  {
    const auto it = TextureMappingGLESExtensions.find(textureFormat);
    if (it != TextureMappingGLESExtensions.cend())
      glFormat = it->second;
  }

  return glFormat;
}

TextureFormatGLES CGLESTexture::GetFormatGLES30(KD_TEX_FMT textureFormat)
{
  TextureFormatGLES glFormat;

  if (textureFormat & KD_TEX_FMT_SDR || textureFormat & KD_TEX_FMT_HDR)
  {
#if defined(GL_ES_VERSION_3_0)
    const auto it = TextureMappingGLES30.find(KD_TEX_FMT_SDR_RGBA8);
    if (it != TextureMappingGLES30.cend())
      glFormat = it->second;
#else
    const auto it = TextureMappingGLES20.find(textureFormat);
    if (it != TextureMappingGLES20.cend())
      glFormat = it->second;
    glFormat.format = glFormat.internalFormat;
#endif
  }
  else
  {
    const auto it = TextureMappingGLESExtensions.find(textureFormat);
    if (it != TextureMappingGLESExtensions.cend())
      glFormat = it->second;
  }

  return glFormat;
}
