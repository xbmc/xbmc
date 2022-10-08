/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Map.h"

#include <fmt/format.h>

enum EShaderFormat
{
  SHADER_NONE,
  SHADER_YV12,
  SHADER_YV12_9,
  SHADER_YV12_10,
  SHADER_YV12_12,
  SHADER_YV12_14,
  SHADER_YV12_16,
  SHADER_NV12,
  SHADER_YUY2,
  SHADER_UYVY,
  SHADER_NV12_RRG,
  SHADER_MAX,
};

template<>
struct fmt::formatter<EShaderFormat> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(const EShaderFormat& shaderFormat, FormatContext& ctx)
  {
    const auto it = shaderFormatMap.find(shaderFormat);
    if (it == shaderFormatMap.cend())
      throw std::range_error("no shader format string found");

    return fmt::formatter<string_view>::format(it->second, ctx);
  }

private:
  static constexpr auto shaderFormatMap = make_map<EShaderFormat, std::string_view>({
      {SHADER_NONE, "none"},
      {SHADER_YV12, "YV12"},
      {SHADER_YV12_9, "YV12 9bit"},
      {SHADER_YV12_10, "YV12 10bit"},
      {SHADER_YV12_12, "YV12 12bit"},
      {SHADER_YV12_14, "YV12 14bit"},
      {SHADER_YV12_16, "YV12 16bit"},
      {SHADER_NV12, "NV12"},
      {SHADER_YUY2, "YUY2"},
      {SHADER_UYVY, "UYVY"},
      {SHADER_NV12_RRG, "NV12 red/red/green"},
  });

  static_assert(SHADER_MAX == shaderFormatMap.size(),
                "shaderFormatMap doesn't match the size of EShaderFormat, did you forget to "
                "add/remove a mapping?");
};
