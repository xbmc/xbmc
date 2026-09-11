/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/geometry/ContentBarDetector.h"

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace KODI::VIDEO::GEOMETRY::TEST
{

/*!
 * \brief An in-memory YUV frame for detector tests.
 *
 * All sample values are given in native code values for the frame's bit depth, so a test
 * for 10-bit limited range says 64 for black rather than relying on a helper to scale it.
 * That is the point: the failure this class exists to catch is an 8-bit constant reaching
 * a 10-bit plane.
 */
class CSyntheticFrame
{
public:
  CSyntheticFrame(unsigned int width,
                  unsigned int height,
                  unsigned int bitDepth = 8,
                  ColorRange range = ColorRange::Limited,
                  ChromaSubsampling subsampling = ChromaSubsampling::YUV420)
    : m_width(width),
      m_height(height),
      m_bitDepth(bitDepth),
      m_range(range),
      m_subsampling(subsampling),
      m_bytes(bitDepth > 8 ? 2 : 1)
  {
    m_chromaWidth = subsampling == ChromaSubsampling::YUV444 ? width : (width + 1) / 2;
    m_chromaHeight = subsampling == ChromaSubsampling::YUV420 ? (height + 1) / 2 : height;

    m_lumaStride = static_cast<int>(width * m_bytes);
    m_chromaStride = static_cast<int>(m_chromaWidth * m_bytes);

    m_y.assign(static_cast<size_t>(m_lumaStride) * height, 0);
    m_u.assign(static_cast<size_t>(m_chromaStride) * m_chromaHeight, 0);
    m_v.assign(static_cast<size_t>(m_chromaStride) * m_chromaHeight, 0);

    Fill(Black(), Neutral(), Neutral());
  }

  //! \brief Reference black for this frame's range and depth.
  unsigned int Black() const
  {
    return m_range == ColorRange::Full ? 0u : (16u << (m_bitDepth - 8));
  }
  //! \brief Achromatic Cb/Cr for this frame's depth. Mid scale in both ranges.
  unsigned int Neutral() const { return 128u << (m_bitDepth - 8); }
  //! \brief Nominal white for this frame's range and depth.
  unsigned int White() const
  {
    return m_range == ColorRange::Full ? ((1u << m_bitDepth) - 1u) : (235u << (m_bitDepth - 8));
  }
  //! \brief Scale an 8-bit equivalent quantity to this frame's depth.
  unsigned int Scaled(unsigned int value8) const { return value8 << (m_bitDepth - 8); }

  void Fill(unsigned int luma, unsigned int cb, unsigned int cr)
  {
    FillRect(0, 0, m_width, m_height, luma, cb, cr);
  }

  //! \brief Fill a half-open luma rectangle, and the chroma samples it covers.
  void FillRect(unsigned int x0,
                unsigned int y0,
                unsigned int x1,
                unsigned int y1,
                unsigned int luma,
                unsigned int cb,
                unsigned int cr)
  {
    for (unsigned int y = y0; y < y1 && y < m_height; ++y)
    {
      for (unsigned int x = x0; x < x1 && x < m_width; ++x)
        Store(m_y, m_lumaStride, x, y, luma);
    }

    const unsigned int sx = m_subsampling == ChromaSubsampling::YUV444 ? 0 : 1;
    const unsigned int sy = m_subsampling == ChromaSubsampling::YUV420 ? 1 : 0;

    for (unsigned int y = y0 >> sy; y < ((y1 + (1u << sy) - 1) >> sy) && y < m_chromaHeight; ++y)
    {
      for (unsigned int x = x0 >> sx; x < ((x1 + (1u << sx) - 1) >> sx) && x < m_chromaWidth; ++x)
      {
        Store(m_u, m_chromaStride, x, y, cb);
        Store(m_v, m_chromaStride, x, y, cr);
      }
    }
  }

  void FillRows(
      unsigned int y0, unsigned int y1, unsigned int luma, unsigned int cb, unsigned int cr)
  {
    FillRect(0, y0, m_width, y1, luma, cb, cr);
  }

  //! \brief Write chroma only, over the chroma lines the given luma rows map to.
  void FillChromaRows(unsigned int y0, unsigned int y1, unsigned int cb, unsigned int cr)
  {
    const unsigned int sy = m_subsampling == ChromaSubsampling::YUV420 ? 1 : 0;
    for (unsigned int y = y0 >> sy; y < ((y1 + (1u << sy) - 1) >> sy) && y < m_chromaHeight; ++y)
    {
      for (unsigned int x = 0; x < m_chromaWidth; ++x)
      {
        Store(m_u, m_chromaStride, x, y, cb);
        Store(m_v, m_chromaStride, x, y, cr);
      }
    }
  }

  /*!
   * \brief Add zero-mean luma noise of the given standard deviation, in native code values.
   *
   * Only std::mt19937's output sequence is specified by the standard; the distribution
   * classes are not, and differ between libstdc++, libc++ and MSVC. The noise is therefore
   * built from raw engine output so the corpus is identical on every platform.
   */
  void AddLumaGrain(unsigned int sigma, uint32_t seed, unsigned int y0 = 0, unsigned int y1 = ~0u)
  {
    std::mt19937 rng(seed);
    const unsigned int last = y1 == ~0u ? m_height : (y1 < m_height ? y1 : m_height);
    for (unsigned int y = y0; y < last; ++y)
    {
      for (unsigned int x = 0; x < m_width; ++x)
      {
        const int value = static_cast<int>(Load(m_y, m_lumaStride, x, y)) + Noise(rng, sigma);
        Store(m_y, m_lumaStride, x, y, Clamp(value));
      }
    }
  }

  /*!
   * \brief Fill a luma rectangle with a horizontal gradient plus grain: plausible picture.
   */
  void FillPicture(unsigned int y0,
                   unsigned int y1,
                   unsigned int low,
                   unsigned int high,
                   uint32_t seed,
                   unsigned int sigma = 0)
  {
    FillRows(y0, y1, low, Neutral(), Neutral());
    const unsigned int span = m_width > 1 ? m_width - 1 : 1;
    for (unsigned int y = y0; y < y1 && y < m_height; ++y)
    {
      for (unsigned int x = 0; x < m_width; ++x)
        Store(m_y, m_lumaStride, x, y, low + (high - low) * x / span);
    }
    if (sigma > 0)
      AddLumaGrain(sigma, seed, y0, y1);
  }

  void SetLuma(unsigned int x, unsigned int y, unsigned int value)
  {
    Store(m_y, m_lumaStride, x, y, value);
  }

  unsigned int Luma(unsigned int x, unsigned int y) const { return Load(m_y, m_lumaStride, x, y); }

  //! \brief Drop the chroma planes, as a caller with luma only would.
  void DropChroma() { m_chromaPresent = false; }

  FrameRef Ref() const
  {
    FrameRef frame;
    frame.width = m_width;
    frame.height = m_height;
    frame.bitDepth = m_bitDepth;
    frame.subsampling = m_subsampling;
    frame.range = m_range;
    frame.y = {m_y.data(), m_lumaStride};
    if (m_chromaPresent)
    {
      frame.u = {m_u.data(), m_chromaStride};
      frame.v = {m_v.data(), m_chromaStride};
    }
    return frame;
  }

private:
  static int Noise(std::mt19937& rng, unsigned int sigma)
  {
    // Three uniforms on [-1,1] sum to unit variance, so the scale factor is sigma itself.
    double sum = 0.0;
    for (int i = 0; i < 3; ++i)
      sum += static_cast<double>(rng() % 2048u) / 1024.0 - 1.0;
    return static_cast<int>(std::lround(sum * sigma));
  }

  unsigned int Clamp(int value) const
  {
    const int maximum = static_cast<int>((1u << m_bitDepth) - 1u);
    if (value < 0)
      return 0;
    if (value > maximum)
      return static_cast<unsigned int>(maximum);
    return static_cast<unsigned int>(value);
  }

  void Store(
      std::vector<uint8_t>& plane, int stride, unsigned int x, unsigned int y, unsigned int value)
  {
    const size_t offset = static_cast<size_t>(y) * stride + static_cast<size_t>(x) * m_bytes;
    if (m_bytes == 1)
    {
      plane[offset] = static_cast<uint8_t>(value);
    }
    else
    {
      plane[offset] = static_cast<uint8_t>(value & 0xFF);
      plane[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    }
  }

  unsigned int Load(const std::vector<uint8_t>& plane,
                    int stride,
                    unsigned int x,
                    unsigned int y) const
  {
    const size_t offset = static_cast<size_t>(y) * stride + static_cast<size_t>(x) * m_bytes;
    if (m_bytes == 1)
      return plane[offset];
    return plane[offset] | (static_cast<unsigned int>(plane[offset + 1]) << 8);
  }

  unsigned int m_width;
  unsigned int m_height;
  unsigned int m_bitDepth;
  ColorRange m_range;
  ChromaSubsampling m_subsampling;
  unsigned int m_bytes;
  unsigned int m_chromaWidth{0};
  unsigned int m_chromaHeight{0};
  int m_lumaStride{0};
  int m_chromaStride{0};
  bool m_chromaPresent{true};
  std::vector<uint8_t> m_y;
  std::vector<uint8_t> m_u;
  std::vector<uint8_t> m_v;
};

} // namespace KODI::VIDEO::GEOMETRY::TEST
