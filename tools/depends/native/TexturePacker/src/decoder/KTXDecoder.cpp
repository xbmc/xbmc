/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KTXDecoder.h"

#include "SimpleFS.h"
#include "guilib/TextureFormats.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace
{
// clang-format off
static const std::map<uint32_t, TextureProperties> TextureMapping
{
  // S3TC
  {0x83F0, {KD_TEX_FMT_S3TC_RGB8, KD_TEX_ALPHA_OPAQUE}}, // GL_COMPRESSED_RGB_S3TC_DXT1_EXT
  {0x83F1, {KD_TEX_FMT_S3TC_RGB8_A1}}, // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
  {0x83F2, {KD_TEX_FMT_S3TC_RGB8_A4}}, // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
  {0x83F3, {KD_TEX_FMT_S3TC_RGBA8}}, // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
  {0x8C4C, {KD_TEX_FMT_S3TC_RGB8, KD_TEX_ALPHA_OPAQUE}}, // GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
  {0x8C4D, {KD_TEX_FMT_S3TC_RGB8_A1}}, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
  {0x8C4E, {KD_TEX_FMT_S3TC_RGB8_A4}}, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
  {0x8C4F, {KD_TEX_FMT_S3TC_RGBA8}}, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
  // ETC1
  {0x8D64, {KD_TEX_FMT_ETC1_RGB8, KD_TEX_ALPHA_OPAQUE}}, // GL_ETC1_RGB8_OES
  // RGTC
  {0x8DBB, {KD_TEX_FMT_RGTC_R11, KD_TEX_ALPHA_STRAIGHT, KD_TEX_SWIZ_111R}}, // GL_COMPRESSED_RED_RGTC1_EXT, interpret as alpha texture
  {0x8DBD, {KD_TEX_FMT_RGTC_RG11, KD_TEX_ALPHA_STRAIGHT, KD_TEX_SWIZ_RRRG}}, // GL_COMPRESSED_RED_GREEN_RGTC2_EXT
  // BPTC
  {0x8E8C, {KD_TEX_FMT_BPTC_RGBA8}}, // GL_COMPRESSED_RGBA_BPTC_UNORM
  {0x8E8D, {KD_TEX_FMT_BPTC_RGBA8}}, // GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
  {0x8E8F, {KD_TEX_FMT_BPTC_RGB16F}}, // GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
  // ETC2
  {0x9270, {KD_TEX_FMT_ETC2_R11, KD_TEX_ALPHA_STRAIGHT, KD_TEX_SWIZ_111R}}, // GL_COMPRESSED_R11_EAC, interpret as alpha texture
  {0x9272, {KD_TEX_FMT_ETC2_RG11, KD_TEX_ALPHA_STRAIGHT, KD_TEX_SWIZ_RRRG}}, // GL_COMPRESSED_RG11_EAC
  {0x9274, {KD_TEX_FMT_ETC2_RGB8, KD_TEX_ALPHA_OPAQUE}}, // GL_COMPRESSED_RGB8_ETC2
  {0x9275, {KD_TEX_FMT_ETC2_RGB8, KD_TEX_ALPHA_OPAQUE}}, // GL_COMPRESSED_SRGB8_ETC2
  {0x9276, {KD_TEX_FMT_ETC2_RGB8_A1}}, // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
  {0x9277, {KD_TEX_FMT_ETC2_RGB8_A1}}, // GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
  {0x9278, {KD_TEX_FMT_ETC2_RGBA8}}, // GL_COMPRESSED_RGBA8_ETC2_EAC
  {0x9279, {KD_TEX_FMT_ETC2_RGBA8}}, // GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
  // ASTC, just LDR for now
  {0x93B0, {KD_TEX_FMT_ASTC_LDR_4x4}}, // GL_COMPRESSED_RGBA_ASTC_4x4_KHR
  {0x93B1, {KD_TEX_FMT_ASTC_LDR_5x4}}, // GL_COMPRESSED_RGBA_ASTC_5x4_KHR
  {0x93B2, {KD_TEX_FMT_ASTC_LDR_5x5}}, // GL_COMPRESSED_RGBA_ASTC_5x5_KHR
  {0x93B3, {KD_TEX_FMT_ASTC_LDR_6x5}}, // GL_COMPRESSED_RGBA_ASTC_6x5_KHR
  {0x93B4, {KD_TEX_FMT_ASTC_LDR_6x6}}, // GL_COMPRESSED_RGBA_ASTC_6x6_KHR
  {0x93B5, {KD_TEX_FMT_ASTC_LDR_8x5}}, // GL_COMPRESSED_RGBA_ASTC_8x5_KHR
  {0x93B6, {KD_TEX_FMT_ASTC_LDR_8x6}}, // GL_COMPRESSED_RGBA_ASTC_8x6_KHR
  {0x93B7, {KD_TEX_FMT_ASTC_LDR_8x8}}, // GL_COMPRESSED_RGBA_ASTC_8x8_KHR
  {0x93B8, {KD_TEX_FMT_ASTC_LDR_10x5}}, // GL_COMPRESSED_RGBA_ASTC_10x5_KHR
  {0x93B9, {KD_TEX_FMT_ASTC_LDR_10x6}}, // GL_COMPRESSED_RGBA_ASTC_10x6_KHR
  {0x93BA, {KD_TEX_FMT_ASTC_LDR_10x8}}, // GL_COMPRESSED_RGBA_ASTC_10x8_KHR
  {0x93BB, {KD_TEX_FMT_ASTC_LDR_10x10}}, // GL_COMPRESSED_RGBA_ASTC_10x10_KHR
  {0x93BC, {KD_TEX_FMT_ASTC_LDR_12x10}}, // GL_COMPRESSED_RGBA_ASTC_12x10_KHR
  {0x93BD, {KD_TEX_FMT_ASTC_LDR_12x12}}, // GL_COMPRESSED_RGBA_ASTC_12x12_KHR
  {0x93D0, {KD_TEX_FMT_ASTC_LDR_4x4}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
  {0x93D1, {KD_TEX_FMT_ASTC_LDR_5x4}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR
  {0x93D2, {KD_TEX_FMT_ASTC_LDR_5x5}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR
  {0x93D3, {KD_TEX_FMT_ASTC_LDR_6x5}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR
  {0x93D4, {KD_TEX_FMT_ASTC_LDR_6x6}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR
  {0x93D5, {KD_TEX_FMT_ASTC_LDR_8x5}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR
  {0x93D6, {KD_TEX_FMT_ASTC_LDR_8x6}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR
  {0x93D7, {KD_TEX_FMT_ASTC_LDR_8x8}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR
  {0x93D8, {KD_TEX_FMT_ASTC_LDR_10x5}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR
  {0x93D9, {KD_TEX_FMT_ASTC_LDR_10x6}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR
  {0x93DA, {KD_TEX_FMT_ASTC_LDR_10x8}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR
  {0x93DB, {KD_TEX_FMT_ASTC_LDR_10x10}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR
  {0x93DC, {KD_TEX_FMT_ASTC_LDR_12x10}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR
  {0x93DD, {KD_TEX_FMT_ASTC_LDR_12x12}}, // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,
};

static const std::map<std::string, KD_TEX_SWIZ> SwizzleKeys
{
  {"KD_TEX_SWIZ_RGBA", KD_TEX_SWIZ_RGBA}, 
  {"KD_TEX_SWIZ_RGB1", KD_TEX_SWIZ_RGB1},
  {"KD_TEX_SWIZ_RRR1", KD_TEX_SWIZ_RRR1}, 
  {"KD_TEX_SWIZ_111R", KD_TEX_SWIZ_111R},
  {"KD_TEX_SWIZ_RRRG", KD_TEX_SWIZ_RRRG}, 
  {"KD_TEX_SWIZ_RRRR", KD_TEX_SWIZ_RRRR},
  {"KD_TEX_SWIZ_GGG1", KD_TEX_SWIZ_GGG1}, 
  {"KD_TEX_SWIZ_111G", KD_TEX_SWIZ_111G},
  {"KD_TEX_SWIZ_GGGA", KD_TEX_SWIZ_GGGA}, 
  {"KD_TEX_SWIZ_GGGG", KD_TEX_SWIZ_GGGG},
  {"KD_TEX_SWIZ_SDF", KD_TEX_SWIZ_SDF},   
  {"KD_TEX_SWIZ_RGB_SDF", KD_TEX_SWIZ_RGB_SDF},
  {"KD_TEX_SWIZ_MSDF", KD_TEX_SWIZ_MSDF},
};

static const std::map<std::string, KD_TEX_ALPHA> AlphaKeys
{
  {"KD_TEX_ALPHA_STRAIGHT", KD_TEX_ALPHA_STRAIGHT},
  {"KD_TEX_ALPHA_OPAQUE", KD_TEX_ALPHA_OPAQUE},
  {"KD_TEX_ALPHA_PREMULTIPLIED", KD_TEX_ALPHA_PREMULTIPLIED},
};
// clang-format on
} // namespace

KTXDecoder::KTXDecoder()
{
  m_extensions.emplace_back(".ktx");
  m_substitutions.emplace_back(".astc.ktx", KD_TEX_FMT_ASTC_LDR);
  m_substitutions.emplace_back(".bptc.ktx", KD_TEX_FMT_BPTC);
  m_substitutions.emplace_back(".etc1.ktx", KD_TEX_FMT_ETC1);
  m_substitutions.emplace_back(".etc2.ktx", KD_TEX_FMT_ETC2);
  m_substitutions.emplace_back(".rgtc.ktx", KD_TEX_FMT_RGTC);
  m_substitutions.emplace_back(".s3tc.ktx", KD_TEX_FMT_S3TC);
}

bool KTXDecoder::CanDecode(const std::string& filename)
{
  CFile fp;
  if (!fp.Open(filename))
    return false;

  std::array<unsigned char, 12> magic;
  if (fp.Read(&magic, sizeof(magic)) != sizeof(magic))
    return false;

  if (magic != m_magic)
    return false;

  std::array<unsigned char, 4> endianess;
  if (fp.Read(&endianess, sizeof(endianess)) != sizeof(endianess))
    return false;

  if (endianess != m_endianess)
    return false;

  return true;
}

bool KTXDecoder::LoadFile(const std::string& filename, DecodedFrames& frames)
{
  CFile fp;
  if (!fp.Open(filename))
    return false;

  if (fp.Seek(28) != 28)
    return false;

  uint32_t glInternalFormat;
  if (fp.Read(&glInternalFormat, sizeof(glInternalFormat)) != sizeof(glInternalFormat))
    return false;

  DecodedFrame frame;

  if (!ParseFormat(frame.rgbaImage, glInternalFormat))
    return false;

  uint32_t glBaseInternalFormat;
  if (fp.Read(&glBaseInternalFormat, sizeof(glBaseInternalFormat)) != sizeof(glBaseInternalFormat))
    return false;

  uint32_t width;
  if (fp.Read(&width, sizeof(width)) != sizeof(width))
    return false;

  uint32_t height;
  if (fp.Read(&height, sizeof(height)) != sizeof(height))
    return false;

  uint32_t depth;
  if (fp.Read(&depth, sizeof(depth)) != sizeof(depth))
    return false;

  uint32_t numberOfArrayElements;
  if (fp.Read(&numberOfArrayElements, sizeof(numberOfArrayElements)) !=
      sizeof(numberOfArrayElements))
    return false;

  uint32_t numberOfFaces;
  if (fp.Read(&numberOfFaces, sizeof(numberOfFaces)) != sizeof(numberOfFaces))
    return false;

  uint32_t numberOfMipmapLevels;
  if (fp.Read(&numberOfMipmapLevels, sizeof(numberOfMipmapLevels)) != sizeof(numberOfMipmapLevels))
    return false;

  uint32_t bytesOfKeyValueData;
  if (fp.Read(&bytesOfKeyValueData, sizeof(bytesOfKeyValueData)) != sizeof(bytesOfKeyValueData))
    return false;

  // sanity check
  if (bytesOfKeyValueData > 2048)
    return false;

  // pad to 4 bytes
  while (bytesOfKeyValueData % 4)
    bytesOfKeyValueData++;

  std::vector<uint8_t> keyValueData;
  keyValueData.resize(bytesOfKeyValueData);
  if (fp.Read(keyValueData.data(), keyValueData.size()) != keyValueData.size())
    return false;

  ParseKeys(frame.rgbaImage, keyValueData);

  uint32_t size;
  if (fp.Read(&size, sizeof(size)) != sizeof(size))
    return false;

  frame.rgbaImage.width = width;
  frame.rgbaImage.height = height;

  frame.rgbaImage.pixels.resize(size);
  frame.rgbaImage.size = size;

  if (fp.Read(frame.rgbaImage.pixels.data(), size) != size)
    return false;

  frames.frameList.push_back(frame);

  return true;
}

bool KTXDecoder::ParseFormat(RGBAImage& image, uint32_t format)
{
  TextureProperties glFormat{};

  const auto it = TextureMapping.find(format);
  if (it == TextureMapping.cend())
    return false;
  {
    glFormat = it->second;
  }
  image.textureFormat = it->second.textureFormat;
  image.textureAlpha = it->second.textureAlpha;
  image.textureSwizzle = it->second.textureSwizzle;

  return true;
}

void KTXDecoder::ParseKeys(RGBAImage& image, std::vector<uint8_t>& keyValueData)
{
  for (auto it = SwizzleKeys.begin(); it != SwizzleKeys.end(); ++it)
  {
    if (std::search(keyValueData.begin(), keyValueData.end(), it->first.begin(), it->first.end()) !=
        keyValueData.end())
    {
      image.textureSwizzle = it->second;
      break;
    }
  }

  for (auto it = AlphaKeys.begin(); it != AlphaKeys.end(); ++it)
  {
    if (std::search(keyValueData.begin(), keyValueData.end(), it->first.begin(), it->first.end()) !=
        keyValueData.end())
    {
      image.textureAlpha = it->second;
      break;
    }
  }
}
