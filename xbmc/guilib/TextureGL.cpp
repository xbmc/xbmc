/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureGL.h"

#include "ServiceBroker.h"
#include "guilib/TextureFormats.h"
#include "guilib/TextureGLFormatMap.h"
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
  return std::make_unique<CGLTexture>(width, height, format);
}

CGLTexture::CGLTexture(unsigned int width, unsigned int height, XB_FMT format)
  : CTexture(width, height, format)
{
  unsigned int major, minor;
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
  if (major >= 3)
    m_isOglVersion3orNewer = true;
  if (major > 3 || (major == 3 && minor >= 3))
    m_isOglVersion33orNewer = true;
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

    // Lower LOD bias equals more sharpness, but less smooth animation
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.5f);
    if (!m_isOglVersion3orNewer)
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
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

    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_textureWidth);

    m_textureWidth = maxSize;
  }

  SetSwizzle();

  TextureFormatGL glFormat = GetFormatGL(m_textureFormat);

  if (glFormat.format == GL_FALSE)
  {
    CLog::LogF(LOGDEBUG, "Failed to load texture. Unsupported format {}", m_textureFormat);
    m_loadedToGPU = true;
    return;
  }

  if ((m_textureFormat & KD_TEX_FMT_SDR) || (m_textureFormat & KD_TEX_FMT_HDR))
  {
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat.internalFormat, m_textureWidth, m_textureHeight, 0,
                 glFormat.format, glFormat.type, m_pixels);
  }
  else
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFormat.internalFormat, m_textureWidth,
                           m_textureHeight, 0, GetPitch() * GetRows(), m_pixels);
  }

  if (IsMipmapped() && m_isOglVersion3orNewer)
    glGenerateMipmap(GL_TEXTURE_2D);

  VerifyGLState();

  if (!m_bCacheMemory)
  {
    KODI::MEMORY::AlignedFree(m_pixels);
    m_pixels = NULL;
  }

  m_loadedToGPU = true;
}

void CGLTexture::SyncGPU()
{
  glFinish();
}

void CGLTexture::BindToUnit(unsigned int unit)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, m_texture);
}

void CGLTexture::SetSwizzle()
{
  if (!SwizzleMapGl.contains(m_textureSwizzle))
    return;

  TextureswizzleGL swiz = SwizzleMapGl.at(m_textureSwizzle);

  // GL_TEXTURE_SWIZZLE_RGBA and GL_TEXTURE_SWIZZLE_RGBA_EXT should be the same
  // token, but just to be sure...
#if defined(GL_VERSION_3_3) || (GL_ARB_texture_swizzle)
  if (m_isOglVersion33orNewer ||
      CServiceBroker::GetRenderSystem()->IsExtSupported("GL_ARB_texture_swizzle"))
  {
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, &swiz.r);
    return;
  }
#endif
#if defined(GL_EXT_texture_swizzle)
  if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_texture_swizzle"))
  {
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA_EXT, &swiz.r);
    return;
  }
#endif
}

TextureFormatGL CGLTexture::GetFormatGL(KD_TEX_FMT textureFormat)
{
  TextureFormatGL glFormat;

  const auto it = TextureMappingGL.find(textureFormat);
  if (it != TextureMappingGL.cend())
  {
    glFormat = it->second;
  }

  return glFormat;
}
