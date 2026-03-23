/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/DVDStreamInfo.h"

#include "HDR10Plus.h"

enum class PeakBrightnessSource {
  Histogram = 0,
  Histogram99,
  MaxScl,
  MaxSclLuminance,
  HistogramPlus
};

struct VdrDmData {

  VdrDmData() {} // Default constructor

  uint16_t min_pq;
  uint16_t max_pq;
  uint16_t avg_pq;

  uint16_t source_min_pq;
  uint16_t source_max_pq;

  uint16_t max_display_mastering_luminance;
  uint16_t min_display_mastering_luminance;
  uint16_t max_content_light_level;
  uint16_t max_frame_average_light_level;
};

std::vector<uint8_t> create_dovi_rpu_nalu_from_hdr10plus(
  const Hdr10PlusMetadata& meta,
  const PeakBrightnessSource& peak_source,
  const HDRStaticMetadataInfo& hdrStaticMetadataInfo);

int max_pq_to_nits(int pq);
