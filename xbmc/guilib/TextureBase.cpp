/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureBase.h"

#include "ServiceBroker.h"
#include "commons/ilog.h"
#include "guilib/TextureFormats.h"
#include "rendering/RenderSystem.h"
#include "utils/MemUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <utility>

void CTextureBase::Allocate(uint32_t width, uint32_t height, XB_FMT format)
{
  SetKDFormat(format);
  m_imageWidth = m_originalWidth = width;
  m_imageHeight = m_originalHeight = height;
  m_format = format;
  m_orientation = 0;

  m_textureWidth = m_imageWidth;
  m_textureHeight = m_imageHeight;

  if (!CServiceBroker::GetRenderSystem()->SupportsNPOT((m_textureFormat & KD_TEX_FMT_TYPE_MASK) !=
                                                       KD_TEX_FMT_S3TC))
  {
    m_textureWidth = PadPow2(m_textureWidth);
    m_textureHeight = PadPow2(m_textureHeight);
  }

  if ((m_textureFormat & KD_TEX_FMT_TYPE_MASK) == KD_TEX_FMT_S3TC)
  {
    // DXT textures must be a multiple of 4 in width and height
    m_textureWidth = ((m_textureWidth + 3) / 4) * 4;
    m_textureHeight = ((m_textureHeight + 3) / 4) * 4;
  }
  else
  {
    // align all textures so that they have an even width
    // in some circumstances when we downsize a thumbnail
    // which has an uneven number of pixels in width
    // we crash in CPicture::ScaleImage in ffmpegs swscale
    // because it tries to access beyond the source memory
    // (happens on osx and ios)
    // UPDATE: don't just update to be on an even width;
    // ffmpegs swscale relies on a 16-byte stride on some systems
    // so the textureWidth needs to be a multiple of 16. see ffmpeg
    // swscale headers for more info.
    m_textureWidth = ((m_textureWidth + 15) / 16) * 16;
  }

  // check for max texture size
  m_textureWidth = std::min(m_textureWidth, CServiceBroker::GetRenderSystem()->GetMaxTextureSize());
  m_textureHeight =
      std::min(m_textureHeight, CServiceBroker::GetRenderSystem()->GetMaxTextureSize());
  m_imageWidth = std::min(m_imageWidth, m_textureWidth);
  m_imageHeight = std::min(m_imageHeight, m_textureHeight);

  KODI::MEMORY::AlignedFree(m_pixels);
  m_pixels = NULL;

  size_t size = GetPitch() * GetRows();

  if (size == 0)
    return;

  m_pixels = static_cast<unsigned char*>(KODI::MEMORY::AlignedMalloc(size, 32));

  if (m_pixels == nullptr)
    CLog::Log(LOGERROR, "{} - Could not allocate {} bytes. Out of memory.", __FUNCTION__, size);
}

uint32_t CTextureBase::PadPow2(uint32_t x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

bool CTextureBase::SwapBlueRed(
    uint8_t* pixels, uint32_t height, uint32_t pitch, uint32_t elements, uint32_t offset)
{
  if (!pixels)
    return false;
  uint8_t* dst = pixels;
  for (uint32_t y = 0; y < height; y++)
  {
    dst = pixels + (y * pitch);
    for (uint32_t x = 0; x < pitch; x += elements)
      std::swap(dst[x + offset], dst[x + 2 + offset]);
  }
  return true;
}

void CTextureBase::ClampToEdge()
{
  if (m_pixels == nullptr)
    return;

  uint32_t imagePitch = GetPitch(m_imageWidth);
  uint32_t imageRows = GetRows(m_imageHeight);
  uint32_t texturePitch = GetPitch(m_textureWidth);
  uint32_t textureRows = GetRows(m_textureHeight);

  if (imagePitch < texturePitch)
  {
    uint32_t blockSize = GetBlockSize();
    unsigned char* src = m_pixels + imagePitch - blockSize;
    unsigned char* dst = m_pixels;
    for (uint32_t y = 0; y < imageRows; y++)
    {
      for (uint32_t x = imagePitch; x < texturePitch; x += blockSize)
        memcpy(dst + x, src, blockSize);
      dst += texturePitch;
    }
  }

  if (imageRows < textureRows)
  {
    unsigned char* dst = m_pixels + imageRows * texturePitch;
    for (uint32_t y = imageRows; y < textureRows; y++)
    {
      memcpy(dst, dst - texturePitch, texturePitch);
      dst += texturePitch;
    }
  }
}

uint32_t CTextureBase::GetPitch(uint32_t width) const
{
  uint32_t blockWidth = GetBlockWidth();
  uint32_t pitch = ((width + blockWidth - 1) / blockWidth) * GetBlockSize();

  // For the GPU, RGB8 needs to be aligned to 32 bit
  if (m_textureFormat == KD_TEX_FMT_SDR_RGB8)
    pitch = ((pitch + 3) / 4) * 4;

  return pitch;
}

uint32_t CTextureBase::GetRows(uint32_t height) const
{
  uint32_t blockHeight = GetBlockHeight();
  return (height + blockHeight - 1) / blockHeight;
}

uint32_t CTextureBase::GetBlockWidth() const
{
  switch (m_textureFormat)
  {
    case KD_TEX_FMT_SDR_R8:
    case KD_TEX_FMT_SDR_RG8:
    case KD_TEX_FMT_SDR_R5G6B5:
    case KD_TEX_FMT_SDR_RGB5_A1:
    case KD_TEX_FMT_SDR_RGBA4:
    case KD_TEX_FMT_SDR_RGB8:
    case KD_TEX_FMT_SDR_RGBA8:
    case KD_TEX_FMT_SDR_BGRA8:
    case KD_TEX_FMT_HDR_R16f:
    case KD_TEX_FMT_HDR_RG16f:
    case KD_TEX_FMT_HDR_R11F_G11F_B10F:
    case KD_TEX_FMT_HDR_RGB9_E5:
    case KD_TEX_FMT_HDR_RGB10_A2:
    case KD_TEX_FMT_HDR_RGBA16f:
      return 1;
    case KD_TEX_FMT_YUV_YUYV8:
      return 2;
    case KD_TEX_FMT_S3TC_RGB8:
    case KD_TEX_FMT_S3TC_RGB8_A1:
    case KD_TEX_FMT_S3TC_RGB8_A4:
    case KD_TEX_FMT_S3TC_RGBA8:
    case KD_TEX_FMT_RGTC_R11:
    case KD_TEX_FMT_RGTC_RG11:
    case KD_TEX_FMT_BPTC_RGB16F:
    case KD_TEX_FMT_BPTC_RGBA8:
    case KD_TEX_FMT_ETC1:
    case KD_TEX_FMT_ETC2_RGB8:
    case KD_TEX_FMT_ETC2_RGB8_A1:
    case KD_TEX_FMT_ETC2_RGBA8:
    case KD_TEX_FMT_ETC2_R11:
    case KD_TEX_FMT_ETC2_RG11:
    case KD_TEX_FMT_ASTC_LDR_4x4:
    case KD_TEX_FMT_ASTC_HDR_4x4:
      return 4;
    case KD_TEX_FMT_ASTC_LDR_5x4:
    case KD_TEX_FMT_ASTC_LDR_5x5:
    case KD_TEX_FMT_ASTC_HDR_5x4:
    case KD_TEX_FMT_ASTC_HDR_5x5:
      return 5;
    case KD_TEX_FMT_ASTC_LDR_6x5:
    case KD_TEX_FMT_ASTC_LDR_6x6:
    case KD_TEX_FMT_ASTC_HDR_6x5:
    case KD_TEX_FMT_ASTC_HDR_6x6:
      return 6;
    case KD_TEX_FMT_ASTC_LDR_8x5:
    case KD_TEX_FMT_ASTC_LDR_8x6:
    case KD_TEX_FMT_ASTC_LDR_8x8:
    case KD_TEX_FMT_ASTC_HDR_8x5:
    case KD_TEX_FMT_ASTC_HDR_8x6:
    case KD_TEX_FMT_ASTC_HDR_8x8:
      return 8;
    case KD_TEX_FMT_ASTC_LDR_10x5:
    case KD_TEX_FMT_ASTC_LDR_10x6:
    case KD_TEX_FMT_ASTC_LDR_10x8:
    case KD_TEX_FMT_ASTC_LDR_10x10:
    case KD_TEX_FMT_ASTC_HDR_10x5:
    case KD_TEX_FMT_ASTC_HDR_10x6:
    case KD_TEX_FMT_ASTC_HDR_10x8:
    case KD_TEX_FMT_ASTC_HDR_10x10:
      return 10;
    case KD_TEX_FMT_ASTC_LDR_12x10:
    case KD_TEX_FMT_ASTC_LDR_12x12:
    case KD_TEX_FMT_ASTC_HDR_12x10:
    case KD_TEX_FMT_ASTC_HDR_12x12:
      return 12;
    default:
      return 1;
  }
}

uint32_t CTextureBase::GetBlockHeight() const
{
  switch (m_textureFormat)
  {
    case KD_TEX_FMT_SDR_R8:
    case KD_TEX_FMT_SDR_RG8:
    case KD_TEX_FMT_SDR_R5G6B5:
    case KD_TEX_FMT_SDR_RGB5_A1:
    case KD_TEX_FMT_SDR_RGBA4:
    case KD_TEX_FMT_SDR_RGB8:
    case KD_TEX_FMT_SDR_RGBA8:
    case KD_TEX_FMT_SDR_BGRA8:
    case KD_TEX_FMT_HDR_R16f:
    case KD_TEX_FMT_HDR_RG16f:
    case KD_TEX_FMT_HDR_R11F_G11F_B10F:
    case KD_TEX_FMT_HDR_RGB9_E5:
    case KD_TEX_FMT_HDR_RGB10_A2:
    case KD_TEX_FMT_HDR_RGBA16f:
    case KD_TEX_FMT_YUV_YUYV8:
      return 1;
    case KD_TEX_FMT_S3TC_RGB8:
    case KD_TEX_FMT_S3TC_RGB8_A1:
    case KD_TEX_FMT_S3TC_RGB8_A4:
    case KD_TEX_FMT_S3TC_RGBA8:
    case KD_TEX_FMT_RGTC_R11:
    case KD_TEX_FMT_RGTC_RG11:
    case KD_TEX_FMT_BPTC_RGB16F:
    case KD_TEX_FMT_BPTC_RGBA8:
    case KD_TEX_FMT_ETC1:
    case KD_TEX_FMT_ETC2_RGB8:
    case KD_TEX_FMT_ETC2_RGB8_A1:
    case KD_TEX_FMT_ETC2_RGBA8:
    case KD_TEX_FMT_ETC2_R11:
    case KD_TEX_FMT_ETC2_RG11:
    case KD_TEX_FMT_ASTC_LDR_4x4:
    case KD_TEX_FMT_ASTC_LDR_5x4:
    case KD_TEX_FMT_ASTC_HDR_4x4:
    case KD_TEX_FMT_ASTC_HDR_5x4:
      return 4;
    case KD_TEX_FMT_ASTC_LDR_5x5:
    case KD_TEX_FMT_ASTC_LDR_6x5:
    case KD_TEX_FMT_ASTC_LDR_8x5:
    case KD_TEX_FMT_ASTC_LDR_10x5:
    case KD_TEX_FMT_ASTC_HDR_5x5:
    case KD_TEX_FMT_ASTC_HDR_6x5:
    case KD_TEX_FMT_ASTC_HDR_8x5:
    case KD_TEX_FMT_ASTC_HDR_10x5:
      return 5;
    case KD_TEX_FMT_ASTC_LDR_6x6:
    case KD_TEX_FMT_ASTC_LDR_8x6:
    case KD_TEX_FMT_ASTC_LDR_10x6:
    case KD_TEX_FMT_ASTC_HDR_6x6:
    case KD_TEX_FMT_ASTC_HDR_8x6:
    case KD_TEX_FMT_ASTC_HDR_10x6:
      return 6;
    case KD_TEX_FMT_ASTC_LDR_8x8:
    case KD_TEX_FMT_ASTC_LDR_10x8:
    case KD_TEX_FMT_ASTC_HDR_8x8:
    case KD_TEX_FMT_ASTC_HDR_10x8:
      return 8;
    case KD_TEX_FMT_ASTC_LDR_10x10:
    case KD_TEX_FMT_ASTC_LDR_12x10:
    case KD_TEX_FMT_ASTC_HDR_10x10:
    case KD_TEX_FMT_ASTC_HDR_12x10:
      return 10;
    case KD_TEX_FMT_ASTC_LDR_12x12:
    case KD_TEX_FMT_ASTC_HDR_12x12:
      return 12;
    default:
      return 4;
  }
}

uint32_t CTextureBase::GetBlockSize() const
{
  switch (m_textureFormat)
  {
    case KD_TEX_FMT_SDR_R8:
      return 1;
    case KD_TEX_FMT_SDR_RG8:
    case KD_TEX_FMT_SDR_R5G6B5:
    case KD_TEX_FMT_SDR_RGB5_A1:
    case KD_TEX_FMT_SDR_RGBA4:
    case KD_TEX_FMT_HDR_R16f:
      return 2;
    case KD_TEX_FMT_SDR_RGB8:
      return 3;
    case KD_TEX_FMT_SDR_RGBA8:
    case KD_TEX_FMT_SDR_BGRA8:
    case KD_TEX_FMT_HDR_RG16f:
    case KD_TEX_FMT_HDR_R11F_G11F_B10F:
    case KD_TEX_FMT_HDR_RGB9_E5:
    case KD_TEX_FMT_HDR_RGB10_A2:
    case KD_TEX_FMT_YUV_YUYV8:
      return 4;
    case KD_TEX_FMT_HDR_RGBA16f:
    case KD_TEX_FMT_S3TC_RGB8:
    case KD_TEX_FMT_S3TC_RGB8_A1:
    case KD_TEX_FMT_RGTC_R11:
    case KD_TEX_FMT_ETC1:
    case KD_TEX_FMT_ETC2_RGB8:
    case KD_TEX_FMT_ETC2_RGB8_A1:
    case KD_TEX_FMT_ETC2_R11:
      return 8;
    case KD_TEX_FMT_S3TC_RGB8_A4:
    case KD_TEX_FMT_S3TC_RGBA8:
    case KD_TEX_FMT_RGTC_RG11:
    case KD_TEX_FMT_BPTC_RGB16F:
    case KD_TEX_FMT_BPTC_RGBA8:
    case KD_TEX_FMT_ETC2_RG11:
    case KD_TEX_FMT_ETC2_RGBA8:
    case KD_TEX_FMT_ASTC_LDR_4x4:
    case KD_TEX_FMT_ASTC_LDR_5x4:
    case KD_TEX_FMT_ASTC_LDR_5x5:
    case KD_TEX_FMT_ASTC_LDR_6x5:
    case KD_TEX_FMT_ASTC_LDR_6x6:
    case KD_TEX_FMT_ASTC_LDR_8x5:
    case KD_TEX_FMT_ASTC_LDR_8x6:
    case KD_TEX_FMT_ASTC_LDR_8x8:
    case KD_TEX_FMT_ASTC_LDR_10x5:
    case KD_TEX_FMT_ASTC_LDR_10x6:
    case KD_TEX_FMT_ASTC_LDR_10x8:
    case KD_TEX_FMT_ASTC_LDR_10x10:
    case KD_TEX_FMT_ASTC_LDR_12x10:
    case KD_TEX_FMT_ASTC_LDR_12x12:
    case KD_TEX_FMT_ASTC_HDR_4x4:
    case KD_TEX_FMT_ASTC_HDR_5x4:
    case KD_TEX_FMT_ASTC_HDR_5x5:
    case KD_TEX_FMT_ASTC_HDR_6x5:
    case KD_TEX_FMT_ASTC_HDR_6x6:
    case KD_TEX_FMT_ASTC_HDR_8x5:
    case KD_TEX_FMT_ASTC_HDR_8x6:
    case KD_TEX_FMT_ASTC_HDR_8x8:
    case KD_TEX_FMT_ASTC_HDR_10x5:
    case KD_TEX_FMT_ASTC_HDR_10x6:
    case KD_TEX_FMT_ASTC_HDR_10x8:
    case KD_TEX_FMT_ASTC_HDR_10x10:
    case KD_TEX_FMT_ASTC_HDR_12x10:
    case KD_TEX_FMT_ASTC_HDR_12x12:
      return 16;
    default:
      return 4;
  }
}

void CTextureBase::SetKDFormat(XB_FMT xbFMT)
{
  switch (xbFMT)
  {
    case XB_FMT_DXT1:
      m_textureFormat = KD_TEX_FMT_S3TC_RGB8;
      return;
    case XB_FMT_DXT3:
      m_textureFormat = KD_TEX_FMT_S3TC_RGB8_A4;
      return;
    case XB_FMT_DXT5:
      m_textureFormat = KD_TEX_FMT_S3TC_RGBA8;
      return;
    case XB_FMT_A8R8G8B8:
      m_textureFormat = KD_TEX_FMT_SDR_BGRA8;
      return;
    case XB_FMT_A8:
      m_textureFormat = KD_TEX_FMT_SDR_R8;
      m_textureSwizzle = KD_TEX_SWIZ_111R;
      return;
    case XB_FMT_RGBA8:
      m_textureFormat = KD_TEX_FMT_SDR_RGBA8;
      return;
    case XB_FMT_RGB8:
      m_textureFormat = KD_TEX_FMT_SDR_RGB8;
      return;
    case XB_FMT_UNKNOWN:
    case XB_FMT_DXT5_YCoCg:
    default:
      m_textureFormat = KD_TEX_FMT_UNKNOWN;
      return;
  }
}
