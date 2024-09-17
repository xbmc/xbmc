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
#include "guilib/TextureManager.h"
#include "rendering/RenderSystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

#include <memory>

namespace
{
// clang-format off
static const std::map<KD_TEX_FMT, TextureFormat> TextureMapping
{
#if defined(GL_EXT_texture_sRGB_R8) && (GL_EXT_texture_sRGB_RG8)
  {KD_TEX_FMT_SDR_R8, {GL_R8, GL_SR8_EXT, GL_RED}},
  {KD_TEX_FMT_SDR_RG8, {GL_RG8, GL_SRG8_EXT, GL_RG}},
#else
  {KD_TEX_FMT_SDR_R8, {GL_R8, GL_FALSE, GL_RED}},
  {KD_TEX_FMT_SDR_RG8, {GL_RG8, GL_FALSE, GL_RG}},
#endif
  {KD_TEX_FMT_SDR_R5G6B5, {GL_RGB565, GL_FALSE, GL_RGB, GL_UNSIGNED_SHORT_5_6_5}},
  {KD_TEX_FMT_SDR_RGB5_A1, {GL_RGB5_A1, GL_FALSE, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1}},
  {KD_TEX_FMT_SDR_RGBA4, {GL_RGBA4, GL_FALSE, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4}},
  {KD_TEX_FMT_SDR_RGB8, {GL_RGB8, GL_SRGB8, GL_RGB}},
  {KD_TEX_FMT_SDR_RGBA8, {GL_RGBA8, GL_SRGB8_ALPHA8, GL_RGBA}},
  {KD_TEX_FMT_SDR_BGRA8, {GL_RGBA8, GL_SRGB8_ALPHA8, GL_BGRA}},

#if defined(GL_VERSION_3_0)
  {KD_TEX_FMT_HDR_R16f, {GL_R16F, GL_FALSE, GL_RED, GL_HALF_FLOAT}},
  {KD_TEX_FMT_HDR_RG16f, {GL_RG16F, GL_FALSE, GL_RG, GL_HALF_FLOAT}},
  {KD_TEX_FMT_HDR_R11F_G11F_B10F, {GL_R11F_G11F_B10F, GL_FALSE, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV}},
  {KD_TEX_FMT_HDR_RGB9_E5, {GL_RGB9_E5, GL_FALSE, GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV}},
  {KD_TEX_FMT_HDR_RGB10_A2, {GL_RGB10_A2, GL_FALSE, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV}},
  {KD_TEX_FMT_HDR_RGBA16f, {GL_RGBA16F, GL_FALSE, GL_RGBA, GL_HALF_FLOAT}},
#endif

#if defined(GL_EXT_texture_compression_s3tc) && (GL_EXT_texture_sRGB)
  {KD_TEX_FMT_S3TC_RGB8, {GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT}},
  {KD_TEX_FMT_S3TC_RGB8_A1, {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT}},
  {KD_TEX_FMT_S3TC_RGB8_A4, {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT}},
  {KD_TEX_FMT_S3TC_RGBA8, {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT}},
#elif defined(GL_EXT_texture_compression_s3tc)
  {KD_TEX_FMT_S3TC_RGB8, {GL_COMPRESSED_RGB_S3TC_DXT1_EXT}},
  {KD_TEX_FMT_S3TC_RGB8_A1, {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT}},
  {KD_TEX_FMT_S3TC_RGB8_A4, {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT}},
  {KD_TEX_FMT_S3TC_RGBA8, {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT}},
#elif defined(GL_EXT_texture_compression_dxt1)
  {KD_TEX_FMT_S3TC_RGB8, {GL_COMPRESSED_RGB_S3TC_DXT1_EXT}},
  {KD_TEX_FMT_S3TC_RGB8_A1, {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT}},
#endif

#if defined(GL_EXT_texture_compression_rgtc)
  {KD_TEX_FMT_RGTC_R11, {GL_COMPRESSED_RED_RGTC1_EXT}},
  {KD_TEX_FMT_RGTC_RG11, {GL_COMPRESSED_RED_GREEN_RGTC2_EXT}},
#endif

#if defined(GL_ARB_texture_compression_bptc)
  {KD_TEX_FMT_BPTC_RGB16F, {GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB}},
  {KD_TEX_FMT_BPTC_RGBA8, {GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB}},
#endif

#if defined(GL_VERSION_4_3)
  {KD_TEX_FMT_ETC1_RGB8, {GL_COMPRESSED_RGB8_ETC2, GL_COMPRESSED_SRGB8_ETC2}},

  {KD_TEX_FMT_ETC2_R11, {GL_COMPRESSED_R11_EAC}},
  {KD_TEX_FMT_ETC2_RG11, {GL_COMPRESSED_RG11_EAC}},
  {KD_TEX_FMT_ETC2_RGB8, {GL_COMPRESSED_RGB8_ETC2, GL_COMPRESSED_SRGB8_ETC2}},
  {KD_TEX_FMT_ETC2_RGB8_A1, {GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2}},
  {KD_TEX_FMT_ETC2_RGBA8, {GL_COMPRESSED_RGBA8_ETC2_EAC, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC}},
#endif

#if defined(GL_KHR_texture_compression_astc_ldr) || (GL_KHR_texture_compression_astc_hdr)
  {KD_TEX_FMT_ASTC_LDR_4x4, {GL_COMPRESSED_RGBA_ASTC_4x4_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR}},
  {KD_TEX_FMT_ASTC_LDR_5x4, {GL_COMPRESSED_RGBA_ASTC_5x4_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR}},
  {KD_TEX_FMT_ASTC_LDR_5x5, {GL_COMPRESSED_RGBA_ASTC_5x5_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR}},
  {KD_TEX_FMT_ASTC_LDR_6x5, {GL_COMPRESSED_RGBA_ASTC_6x5_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR}},
  {KD_TEX_FMT_ASTC_LDR_6x6, {GL_COMPRESSED_RGBA_ASTC_6x6_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR}},
  {KD_TEX_FMT_ASTC_LDR_8x5, {GL_COMPRESSED_RGBA_ASTC_8x5_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR}},
  {KD_TEX_FMT_ASTC_LDR_8x6, {GL_COMPRESSED_RGBA_ASTC_8x6_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR}},
  {KD_TEX_FMT_ASTC_LDR_8x8, {GL_COMPRESSED_RGBA_ASTC_8x8_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR}},
  {KD_TEX_FMT_ASTC_LDR_10x5, {GL_COMPRESSED_RGBA_ASTC_10x5_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR}},
  {KD_TEX_FMT_ASTC_LDR_10x6, {GL_COMPRESSED_RGBA_ASTC_10x6_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR}},
  {KD_TEX_FMT_ASTC_LDR_10x8, {GL_COMPRESSED_RGBA_ASTC_10x8_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR}},
  {KD_TEX_FMT_ASTC_LDR_10x10, {GL_COMPRESSED_RGBA_ASTC_10x10_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR}},
  {KD_TEX_FMT_ASTC_LDR_12x10, {GL_COMPRESSED_RGBA_ASTC_12x10_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR}},
  {KD_TEX_FMT_ASTC_LDR_12x12, {GL_COMPRESSED_RGBA_ASTC_12x12_KHR, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR}},

  {KD_TEX_FMT_ASTC_HDR_4x4, {GL_COMPRESSED_RGBA_ASTC_4x4_KHR}},
  {KD_TEX_FMT_ASTC_HDR_5x4, {GL_COMPRESSED_RGBA_ASTC_5x4_KHR}},
  {KD_TEX_FMT_ASTC_HDR_5x5, {GL_COMPRESSED_RGBA_ASTC_5x5_KHR}},
  {KD_TEX_FMT_ASTC_HDR_6x5, {GL_COMPRESSED_RGBA_ASTC_6x5_KHR}},
  {KD_TEX_FMT_ASTC_HDR_6x6, {GL_COMPRESSED_RGBA_ASTC_6x6_KHR}},
  {KD_TEX_FMT_ASTC_HDR_8x5, {GL_COMPRESSED_RGBA_ASTC_8x5_KHR}},
  {KD_TEX_FMT_ASTC_HDR_8x6, {GL_COMPRESSED_RGBA_ASTC_8x6_KHR}},
  {KD_TEX_FMT_ASTC_HDR_8x8, {GL_COMPRESSED_RGBA_ASTC_8x8_KHR}},
  {KD_TEX_FMT_ASTC_HDR_10x5, {GL_COMPRESSED_RGBA_ASTC_10x5_KHR}},
  {KD_TEX_FMT_ASTC_HDR_10x6, {GL_COMPRESSED_RGBA_ASTC_10x6_KHR}},
  {KD_TEX_FMT_ASTC_HDR_10x8, {GL_COMPRESSED_RGBA_ASTC_10x8_KHR}},
  {KD_TEX_FMT_ASTC_HDR_10x10, {GL_COMPRESSED_RGBA_ASTC_10x10_KHR}},
  {KD_TEX_FMT_ASTC_HDR_12x10, {GL_COMPRESSED_RGBA_ASTC_12x10_KHR}},
  {KD_TEX_FMT_ASTC_HDR_12x12, {GL_COMPRESSED_RGBA_ASTC_12x12_KHR}},
#endif
};

static const std::map<KD_TEX_SWIZ, Textureswizzle> SwizzleMap
{
  {KD_TEX_SWIZ_RGBA, {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA}},
  {KD_TEX_SWIZ_RGB1, {GL_RED, GL_GREEN, GL_BLUE, GL_ONE}},
  {KD_TEX_SWIZ_RRR1, {GL_RED, GL_RED, GL_RED, GL_ONE}},
  {KD_TEX_SWIZ_111R, {GL_ONE, GL_ONE, GL_ONE, GL_RED}},
  {KD_TEX_SWIZ_RRRG, {GL_RED, GL_RED, GL_RED, GL_GREEN}},
  {KD_TEX_SWIZ_RRRR, {GL_RED, GL_RED, GL_RED, GL_RED}},
  {KD_TEX_SWIZ_GGG1, {GL_GREEN, GL_GREEN, GL_GREEN, GL_ONE}},
  {KD_TEX_SWIZ_111G, {GL_ONE, GL_ONE, GL_ONE, GL_GREEN}},
  {KD_TEX_SWIZ_GGGA, {GL_GREEN, GL_GREEN, GL_GREEN, GL_ALPHA}},
  {KD_TEX_SWIZ_GGGG, {GL_GREEN, GL_GREEN, GL_GREEN, GL_GREEN}},
};
// clang-format on
} // namespace

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

  // there might not be any padding for the following formats, so we have to
  // read one/two bytes at the time.
  if (m_textureFormat == KD_TEX_FMT_SDR_R8)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  if (m_textureFormat == KD_TEX_FMT_SDR_RG8)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);

  SetSwizzle();

  TextureFormat glFormat = GetFormatGL(m_textureFormat);

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
  if (!SwizzleMap.contains(m_textureSwizzle))
    return;

  Textureswizzle swiz = SwizzleMap.at(m_textureSwizzle);

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

TextureFormat CGLTexture::GetFormatGL(KD_TEX_FMT textureFormat)
{
  TextureFormat glFormat;

  const auto it = TextureMapping.find(textureFormat);
  if (it != TextureMapping.cend())
  {
    glFormat = it->second;
  }

  return glFormat;
}
