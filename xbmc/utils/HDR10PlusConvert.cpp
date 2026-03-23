/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/log.h"

#include <algorithm> 
#include <vector>
#include <cmath>
#include <cstdint>

#include "cores/VideoPlayer/DVDStreamInfo.h"

#include "HDR10PlusConvert.h"

#include "DoViRpuWriter.h"
#include "HDR10Plus.h"

// Nits to PQ
constexpr double ST2084_Y_MAX = 10000.0;
constexpr double ST2084_M1 = 2610.0 / 16384.0;
constexpr double ST2084_M2 = (2523.0 / 4096.0) * 128.0;
constexpr double ST2084_C1 = 3424.0 / 4096.0;
constexpr double ST2084_C2 = (2413.0 / 4096.0) * 32.0;
constexpr double ST2084_C3 = (2392.0 / 4096.0) * 32.0;

// Clamp Values
constexpr std::uint16_t L1_MAX_PQ_MIN_VALUE = 2081;
constexpr std::uint16_t L1_MAX_PQ_MAX_VALUE = 4095;
constexpr std::uint16_t L1_AVG_PQ_MIN_VALUE = 819;

int max_pq_to_nits(int pq) {
  if (pq < 2055) return 96;
  if (pq > 4095) return 10000;
  switch (pq) {
    case 3079: { return  1000; }
    case 3388: { return  2000; }
    case 3696: { return  4000; }
    case 4095: { return 10000; }
  }
  double pq_normalized = pq / 4095.0;
  double pq_pow = std::pow(pq_normalized, 1.0 / ST2084_M2);
  double num = std::max(pq_pow - ST2084_C1, 0.0);
  double den = ST2084_C2 - ST2084_C3 * pq_pow;
  if (std::abs(den) < std::numeric_limits<double>::epsilon()) {
    return 0;
  }
  return static_cast<int>(std::round(ST2084_Y_MAX * std::pow(num / den, 1.0 / ST2084_M1)));
}

double nits_to_pq(double nits) {
  double y = nits / ST2084_Y_MAX;
  return std::pow((ST2084_C1 + ST2084_C2 * std::pow(y, ST2084_M1)) / (1.0 + ST2084_C3 * std::pow(y, ST2084_M1)), ST2084_M2);
}

static uint16_t cast_pq(double nits) {
  return static_cast<uint16_t>(std::round(nits_to_pq(nits) * 4095.0));
}

static uint16_t maximum_pq(const Hdr10PlusMetadata& meta, const PeakBrightnessSource& source) {

  if (meta.num_windows == 0) return 0;

  switch (source) {

    case PeakBrightnessSource::HistogramPlus:
    case PeakBrightnessSource::Histogram: {

      const auto& distributions = meta.luminance[0].distribution_maxrgb;
      if (distributions.empty()) return 0;

      auto max_value_it = std::max_element(distributions.begin(), distributions.end(),
                                  [](const auto& a, const auto& b) {
                                      return a.percentile < b.percentile;
                                  });
      const auto& max_value = *max_value_it;
      return cast_pq(static_cast<double>(max_value.percentile) / 10.0);
    }

    case PeakBrightnessSource::Histogram99: {

      const auto& distributions = meta.luminance[0].distribution_maxrgb;
      if (distributions.empty()) return 0;

      return cast_pq(static_cast<double>(distributions.back().percentile) / 10.0);
    }

    case PeakBrightnessSource::MaxScl: {

      const auto& max_scl = meta.luminance[0].maxscl;
      auto max_value = *std::max_element(max_scl, max_scl + sizeof(max_scl) / sizeof(max_scl[0]));
      return cast_pq(static_cast<double>(max_value) / 10.0);
    }

    case PeakBrightnessSource::MaxSclLuminance: {

      const auto& max_scl = meta.luminance[0].maxscl;
      double r = static_cast<double>(max_scl[0]);
      double g = static_cast<double>(max_scl[1]);
      double b = static_cast<double>(max_scl[2]);
      double luminance = (0.2627 * r) + (0.678 * g) + (0.0593 * b);
      return cast_pq(luminance / 10.0);

    }
  }

  return 0;
}

static uint16_t average_pq(const Hdr10PlusMetadata& meta, const PeakBrightnessSource& source) {

  if (meta.num_windows == 0) return 0;

  if ((source == PeakBrightnessSource::HistogramPlus) &&
      (meta.luminance[0].num_distribution_maxrgb_percentiles == 9) &&
      (meta.luminance[0].distribution_maxrgb[1].percentage == 5) && 
      (meta.luminance[0].distribution_maxrgb[2].percentage == 10)) {

    double pq1 = nits_to_pq(static_cast<double>(meta.luminance[0].distribution_maxrgb[0].percentile) / 10.0);
    double pq2 = nits_to_pq(static_cast<double>(meta.luminance[0].distribution_maxrgb[3].percentile) / 10.0);
    double pq3 = nits_to_pq(static_cast<double>(meta.luminance[0].distribution_maxrgb[4].percentile) / 10.0);
    double pq4 = nits_to_pq(static_cast<double>(meta.luminance[0].distribution_maxrgb[5].percentile) / 10.0);
    double pq5 = nits_to_pq(static_cast<double>(meta.luminance[0].distribution_maxrgb[6].percentile) / 10.0);
    double pq6 = nits_to_pq(static_cast<double>(meta.luminance[0].distribution_maxrgb[7].percentile) / 10.0);
    double pq7 = nits_to_pq(static_cast<double>(meta.luminance[0].distribution_maxrgb[8].percentile) / 10.0);

    double mean_pq = (pq1 + pq2) / 2.0 * 0.2400 +
                     (pq2 + pq3) / 2.0 * 0.2500 +
                     (pq3 + pq4) / 2.0 * 0.2500 +
                     (pq4 + pq5) / 2.0 * 0.1500 +
                     (pq5 + pq6) / 2.0 * 0.0500 +
                     (pq6 + pq7) / 2.0 * 0.0498;

    return static_cast<uint16_t>(std::round(mean_pq * 4095.0)); 
  }

  return cast_pq(static_cast<double>(meta.luminance[0].average_maxrgb) / 10.0);
}

static uint16_t clamp16(uint16_t d, uint16_t min, uint16_t max) {
  uint16_t t = d < min ? min : d;
  return t > max ? max : t;
}

static std::vector<uint8_t> last_rpu;
static VdrDmData last_vdr_dm_data = {};

std::vector<uint8_t> create_dovi_rpu_nalu_from_hdr10plus(
  const Hdr10PlusMetadata& meta,
  const PeakBrightnessSource& peak_source,
  const HDRStaticMetadataInfo& hdrStaticMetadataInfo) 
{

  uint16_t min_pq = 0;
  // if (hdrStaticMetadataInfo.min_lum <= 10) {
  //  min_pq = 7;
  // } else if (hdrStaticMetadataInfo.min_lum == 50) {
  //  min_pq = 62;
  // }
  if (hdrStaticMetadataInfo.min_lum == 0)
    min_pq = 0;
  else if (hdrStaticMetadataInfo.min_lum < 2)
    min_pq = 7;  // 0.0001
  else if (hdrStaticMetadataInfo.min_lum < 5)
    min_pq = 10; // 0.0002
  else if (hdrStaticMetadataInfo.min_lum < 10)
    min_pq = 17; // 0.0005
  else if (hdrStaticMetadataInfo.min_lum < 20)
    min_pq = 26; // 0.001
  else if (hdrStaticMetadataInfo.min_lum < 50)
    min_pq = 38; // 0.002
  else
    min_pq = 62; // 0.005

  uint16_t source_min_pq = min_pq;
  uint16_t source_max_pq = 3079;
  switch (hdrStaticMetadataInfo.max_lum) {
      case 0:     { source_max_pq = 3079; break; }
      case 1000:  { source_max_pq = 3079; break; }
      case 2000:  { source_max_pq = 3388; break; }
      case 4000:  { source_max_pq = 3696; break; }
      case 10000: { source_max_pq = 4095; break; }
      default:    { source_max_pq = cast_pq(hdrStaticMetadataInfo.max_lum); break; }
  }
  // uint16_t source_max_pq = cast_pq(hdrStaticMetadataInfo.max_lum);

  uint16_t max_pq = maximum_pq(meta, peak_source);
  if (max_pq == 0) {
    switch (hdrStaticMetadataInfo.max_lum) {
      case 1000:  { max_pq = 3079; break; }
      case 2000:  { max_pq = 3388; break; }
      case 4000:  { max_pq = 3696; break; }
      case 10000: { max_pq = 4095; break; }
      default:    { max_pq = 3079; break; }
    }
  }

  uint16_t avg_pq = average_pq(meta, peak_source);

  VdrDmData vdr_dm_data = {};
  vdr_dm_data.source_min_pq = source_min_pq;
  vdr_dm_data.source_max_pq = source_max_pq;
  vdr_dm_data.min_pq = min_pq;
  vdr_dm_data.max_pq = clamp16(max_pq, L1_MAX_PQ_MIN_VALUE, L1_MAX_PQ_MAX_VALUE);
  vdr_dm_data.avg_pq = clamp16(avg_pq, L1_AVG_PQ_MIN_VALUE, (vdr_dm_data.max_pq - 1));

  vdr_dm_data.max_display_mastering_luminance = hdrStaticMetadataInfo.max_lum;
  vdr_dm_data.min_display_mastering_luminance = hdrStaticMetadataInfo.min_lum;
  vdr_dm_data.max_content_light_level = hdrStaticMetadataInfo.max_cll;
  vdr_dm_data.max_frame_average_light_level = hdrStaticMetadataInfo.max_fall;

  if ((last_vdr_dm_data.source_min_pq != vdr_dm_data.source_min_pq) ||
      (last_vdr_dm_data.source_max_pq != vdr_dm_data.source_max_pq) ||
      (last_vdr_dm_data.min_pq != vdr_dm_data.min_pq) ||
      (last_vdr_dm_data.max_pq != vdr_dm_data.max_pq) ||
      (last_vdr_dm_data.avg_pq != vdr_dm_data.avg_pq) ||
      (last_vdr_dm_data.max_display_mastering_luminance != vdr_dm_data.max_display_mastering_luminance) ||
      (last_vdr_dm_data.min_display_mastering_luminance != vdr_dm_data.min_display_mastering_luminance) ||
      (last_vdr_dm_data.max_content_light_level != vdr_dm_data.max_content_light_level) ||
      (last_vdr_dm_data.max_frame_average_light_level != vdr_dm_data.max_frame_average_light_level)) {

    last_rpu = create_dovi_rpu_nalu(vdr_dm_data);
    last_vdr_dm_data = vdr_dm_data;

    logM(LOGDEBUG, "HDR10PlusConvert", "min_pq [{}] max_pq [{}] avg_pq [{}] mdml max [{}] mdml min [{}] cll [{}] fall [{}]",
      vdr_dm_data.min_pq,
      vdr_dm_data.max_pq,
      vdr_dm_data.avg_pq,
      vdr_dm_data.max_display_mastering_luminance,
      vdr_dm_data.min_display_mastering_luminance,
      vdr_dm_data.max_content_light_level,
      vdr_dm_data.max_frame_average_light_level);
  }

  return last_rpu;
}
