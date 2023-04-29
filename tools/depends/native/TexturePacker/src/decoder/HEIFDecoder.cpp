/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HEIFDecoder.h"

#include <algorithm>
#include <map>

#include <libheif/heif_cxx.h>

HEIFDecoder::HEIFDecoder()
{
  heif_init(nullptr);

  m_extensions.emplace_back(".avif");

  m_ctx = std::make_unique<heif::Context>();
}

HEIFDecoder::~HEIFDecoder()
{
  m_ctx.reset();

  heif_deinit();
}

bool HEIFDecoder::CanDecode(const std::string& filename)
{
  try
  {
    m_ctx->read_from_file(filename);
  }
  catch (heif::Error e)
  {
    return false;
  }

  return true;
}

bool HEIFDecoder::LoadFile(const std::string& filename, DecodedFrames& frames)
{
  for (const heif_item_id id : m_ctx->get_list_of_top_level_image_IDs())
  {
    heif::ImageHandle handle = m_ctx->get_image_handle(id);

    heif::Image image = handle.decode_image(heif_colorspace_RGB, heif_chroma_interleaved_RGBA);

    int width = image.get_width(heif_channel_interleaved);
    int height = image.get_height(heif_channel_interleaved);
    int bpp = image.get_bits_per_pixel(heif_channel_interleaved);

    const int channels = 4;

    int stride;
    uint8_t* data = image.get_plane(heif_channel_interleaved, &stride);

    DecodedFrame frame;

    frame.rgbaImage.pixels.resize(height * width * channels);

    enum class RGBA
    {
      R = 0,
      G = 1,
      B = 2,
      A = 3,
    };

    enum class BGRA
    {
      B = 0,
      G = 1,
      R = 2,
      A = 3,
    };

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < (width * channels); x += channels)
      {
        frame.rgbaImage.pixels[y * (width * channels) + x + static_cast<int>(BGRA::B)] =
            data[y * stride + x + static_cast<int>(RGBA::B)];
        frame.rgbaImage.pixels[y * (width * channels) + x + static_cast<int>(BGRA::G)] =
            data[y * stride + x + static_cast<int>(RGBA::G)];
        frame.rgbaImage.pixels[y * (width * channels) + x + static_cast<int>(BGRA::R)] =
            data[y * stride + x + static_cast<int>(RGBA::R)];
        frame.rgbaImage.pixels[y * (width * channels) + x + static_cast<int>(BGRA::A)] =
            data[y * stride + x + static_cast<int>(RGBA::A)];
      }
    }

    frame.rgbaImage.width = width;
    frame.rgbaImage.height = height;
    frame.rgbaImage.bbp = bpp;
    frame.rgbaImage.pitch = (width * channels);

    frames.frameList.push_back(frame);
  }

  return true;
}
