/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

#include "BitstreamReader.h"

struct ProcessingWindow {
  uint16_t window_upper_left_corner_x;
  uint16_t window_upper_left_corner_y;
  uint16_t window_lower_right_corner_x;
  uint16_t window_lower_right_corner_y;

  uint16_t center_of_ellipse_x;
  uint16_t center_of_ellipse_y;
  uint8_t rotation_angle;

  uint16_t semimajor_axis_internal_ellipse;
  uint16_t semimajor_axis_external_ellipse;
  uint16_t semiminor_axis_external_ellipse;

  bool overlap_process_option;
};

struct DistributionMaxRgb {
  uint8_t percentage;
  uint32_t percentile;
};

struct ActualTargetedSystemDisplay {
  uint8_t num_rows_targeted_system_display_actual_peak_luminance;
  uint8_t num_cols_targeted_system_display_actual_peak_luminance;
  std::vector<std::vector<uint8_t>> targeted_system_display_actual_peak_luminance;
};

struct ActualMasteringDisplay {
  uint8_t num_rows_mastering_display_actual_peak_luminance = 0;
  uint8_t num_cols_mastering_display_actual_peak_luminance = 0;
  std::vector<uint8_t> mastering_display_actual_peak_luminance;
};

struct BezierCurve {
  uint16_t knee_point_x = 0;
  uint16_t knee_point_y = 0;
  uint8_t num_bezier_curve_anchors = 0;
  std::vector<uint16_t> bezier_curve_anchors;
};

struct Luminance {
  uint32_t maxscl[3];
	uint32_t average_maxrgb;
	uint16_t num_distribution_maxrgb_percentiles;
  std::vector<DistributionMaxRgb> distribution_maxrgb;
	uint16_t fraction_bright_pixels;
};

struct Hdr10PlusMetadata {
  
  uint8_t itu_t_t35_country_code;
  uint16_t itu_t_t35_terminal_provider_code;
  uint16_t itu_t_t35_terminal_provider_oriented_code;

  uint8_t application_identifier;
  uint8_t application_version;

  uint8_t num_windows;
  std::vector<ProcessingWindow> processing_windows;

  uint32_t targeted_system_display_maximum_luminance;

  bool targeted_system_display_actual_peak_luminance_flag;
  ActualTargetedSystemDisplay actual_targeted_system_display;

  std::vector<Luminance> luminance;

  bool mastering_display_actual_peak_luminance_flag;
  ActualMasteringDisplay actual_mastering_display;

  bool tone_mapping_flag;
  BezierCurve bezier_curve;

  bool color_saturation_mapping_flag;
  uint8_t color_saturation_weight;
};

Hdr10PlusMetadata hdr10plus_sei_to_metadata(CBitstreamReader& br);
