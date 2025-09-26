/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace KODI::SHADER
{
using ShaderParameterMap = std::map<std::string, float, std::less<>>;

enum class FilterType
{
  NONE,
  LINEAR,
  NEAREST
};

enum class WrapType
{
  BORDER,
  EDGE,
  REPEAT,
  MIRRORED_REPEAT,
};

enum class ScaleType
{
  INPUT,
  ABSOLUTE_SCALE,
  VIEWPORT,
};

struct FboScaleAxis
{
  ScaleType scaleType{ScaleType::INPUT};
  float scale{1.0};
  unsigned int abs{1};
};

struct FboScale
{
  bool sRgbFramebuffer{false};
  bool floatFramebuffer{false};
  FboScaleAxis scaleX;
  FboScaleAxis scaleY;
};

struct ShaderLut
{
  std::string strId;
  std::string path;
  FilterType filterType{FilterType::NONE};
  WrapType wrapType{WrapType::BORDER};
  bool mipmap{false};
};

struct ShaderParameter
{
  std::string strId;
  std::string description;
  float current{0.0f};
  float minimum{0.0f};
  float initial{0.0f};
  float maximum{0.0f};
  float step{0.0f};
};

struct ShaderPass
{
  std::string sourcePath;
  std::string vertexSource;
  std::string fragmentSource;
  FilterType filterType{FilterType::NONE};
  WrapType wrapType{WrapType::BORDER};
  unsigned int frameCountMod{0};
  FboScale fbo;
  bool mipmap{false};
  std::string alias;

  std::vector<ShaderLut> luts;
  std::vector<ShaderParameter> parameters;
};

struct float2
{
  float2() : x(0.0f), y(0.0f) {}

  template<typename T>
  float2(T x_, T y_) : x(static_cast<float>(x_)),
                       y(static_cast<float>(y_))
  {
    static_assert(std::is_arithmetic_v<T>, "Not an arithmetic type");
  }

  bool operator==(const float2& rhs) const = default;

  template<typename T>
  T Max()
  {
    return static_cast<T>(std::max(x, y));
  }
  template<typename T>
  T Min()
  {
    return static_cast<T>(std::min(x, y));
  }

  float x;
  float y;
};
} // namespace KODI::SHADER
