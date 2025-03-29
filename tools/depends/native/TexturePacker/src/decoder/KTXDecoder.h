/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDecoder.h"
#include "guilib/TextureFormats.h"

#include <array>
#include <cstdint>
#include <vector>

struct TextureProperties
{
  KD_TEX_FMT textureFormat{KD_TEX_FMT_SDR_BGRA8};
  KD_TEX_ALPHA textureAlpha{KD_TEX_ALPHA_STRAIGHT};
  KD_TEX_SWIZ textureSwizzle{KD_TEX_SWIZ_RGBA};
};

class KTXDecoder : public IDecoder
{
public:
  KTXDecoder();
  ~KTXDecoder() override = default;
  bool CanDecode(const std::string& filename) override;
  bool LoadFile(const std::string& filename, DecodedFrames& frames) override;
  const char* GetImageFormatName() override { return "KTX"; }
  const char* GetDecoderName() override { return "KTX decoder"; }

private:
  bool ParseFormat(RGBAImage& image, uint32_t format);
  void ParseKeys(RGBAImage& image, std::vector<uint8_t>& keyValueData);

  static constexpr std::array<unsigned char, 12> m_magic{0xAB, 'K',  'T',  'X',  ' ',  '1',
                                                         '1',  0xBB, '\r', '\n', 0x1A, '\n'};
  static constexpr std::array<unsigned char, 4> m_endianess{0x01, 0x02, 0x03, 0x04};
};
