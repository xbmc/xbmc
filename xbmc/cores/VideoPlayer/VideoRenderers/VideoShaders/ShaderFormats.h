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
  SHADER_Y210, // packed 4:2:2 in GL_RGBA16 (Y210 / Y212 / Y216)
  SHADER_AYUV, // packed 4:4:4 in GL_RGBA8  (AYUV / XYUV)
  SHADER_Y410, // packed 4:4:4 in GL_RGB10_A2 (Y410)
  SHADER_Y412, // packed 4:4:4 in GL_RGBA16 (Y412 / Y416)
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
      {SHADER_Y210, "Y210 packed 4:2:2"},
      {SHADER_AYUV, "AYUV packed 4:4:4"},
      {SHADER_Y410, "Y410 packed 4:4:4 10-bit"},
      {SHADER_Y412, "Y412 packed 4:4:4 12/16-bit"},
  });

  static_assert(SHADER_MAX == shaderFormatMap.size(),
                "shaderFormatMap doesn't match the size of EShaderFormat, did you forget to "
                "add/remove a mapping?");
};
