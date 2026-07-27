/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/TextureFormats.h"

#include <cstdint>
#include <string>
#include <vector>

/* forward declarations */

class DecodedFrame;
class DecodedFrames;

class IDecoder
{
  public:
    virtual ~IDecoder() = default;
    virtual bool CanDecode(const std::string &filename) = 0;
    virtual bool LoadFile(const std::string& filename, DecodedFrames& frames) = 0;
    virtual const char* GetImageFormatName() = 0;
    virtual const char* GetDecoderName() = 0;

    const std::vector<std::string>& GetSupportedExtensions() const { return m_extensions; }
    const std::vector<std::pair<std::string, KD_TEX_FMT>>& GetSupportedSubstitutions() const
    {
      return m_substitutions;
    }

  protected:
    std::vector<std::string> m_extensions;
    std::vector<std::pair<std::string, KD_TEX_FMT>> m_substitutions;
};

class RGBAImage
{
public:
  RGBAImage() = default;

  std::vector<uint8_t> pixels;
  int width = 0; // width
  int height = 0; // height
  int bbp = 32; // bits per pixel
  int pitch = 0; // rowsize in bytes
  size_t size = 0; // image size in bytes
  KD_TEX_FMT textureFormat{KD_TEX_FMT_SDR_BGRA8};
  KD_TEX_ALPHA textureAlpha{KD_TEX_ALPHA_STRAIGHT};
  KD_TEX_SWIZ textureSwizzle{KD_TEX_SWIZ_RGBA};
};

class DecodedFrame
{
public:
  DecodedFrame() = default;
  RGBAImage rgbaImage; /* rgbaimage for this frame */
  int delay = 0; /* Frame delay in ms */
};

class DecodedFrames
{
  public:
    DecodedFrames() = default;
    std::vector<DecodedFrame> frameList;
};
