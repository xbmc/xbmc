/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace KODI::VIDEO::GEOMETRY
{

inline constexpr unsigned int BUCKET_COUNT = 256;

//! \brief A luma or chroma histogram, always in 8-bit equivalent buckets whatever the source
//! depth, so that every threshold is expressed in the units the histogram is.
class CHistogram
{
public:
  void Reset()
  {
    m_bins.fill(0);
    m_total = 0;
  }

  void Add(unsigned int bucket)
  {
    ++m_bins[bucket];
    ++m_total;
  }

  uint32_t Total() const { return m_total; }

  unsigned int Percentile(double quantile) const
  {
    if (m_total == 0)
      return 0;

    const uint32_t target = static_cast<uint32_t>(quantile * m_total);
    uint32_t accumulated = 0;
    for (unsigned int bucket = 0; bucket < BUCKET_COUNT; ++bucket)
    {
      accumulated += m_bins[bucket];
      if (accumulated > target)
        return bucket;
    }
    return BUCKET_COUNT - 1;
  }

  /*!
   * \brief Several quantiles in one walk of the buckets, answering exactly as Percentile() does.
   *        Classifying a line needs four, and a walk covers all 256 buckets either way.
   *
   * \param quantiles ascending; a precondition rather than something this sorts, as it runs per
   *        line of every frame
   */
  template<size_t N>
  std::array<unsigned int, N> Percentiles(const std::array<double, N>& quantiles) const
  {
    assert(std::is_sorted(quantiles.begin(), quantiles.end()));

    std::array<unsigned int, N> values{};
    if (m_total == 0)
      return values;

    std::array<uint32_t, N> targets{};
    for (size_t i = 0; i < N; ++i)
      targets[i] = static_cast<uint32_t>(quantiles[i] * m_total);

    size_t next = 0;
    uint32_t accumulated = 0;
    for (unsigned int bucket = 0; bucket < BUCKET_COUNT && next < N; ++bucket)
    {
      accumulated += m_bins[bucket];
      while (next < N && accumulated > targets[next])
      {
        values[next] = bucket;
        ++next;
      }
    }

    for (; next < N; ++next)
      values[next] = BUCKET_COUNT - 1;

    return values;
  }

private:
  std::array<uint32_t, BUCKET_COUNT> m_bins{};
  uint32_t m_total{0};
};

} // namespace KODI::VIDEO::GEOMETRY
