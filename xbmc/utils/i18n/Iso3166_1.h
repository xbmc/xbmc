/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace KODI::UTILS::I18N
{
class CIso3166_1
{
public:
  CIso3166_1() = delete;

  static std::optional<std::string> Alpha2ToAlpha3(std::string_view code);
  static std::optional<std::string> Alpha3ToAlpha2(std::string_view code);
  static bool ContainsAlpha3(std::string_view code);
};
} // namespace KODI::UTILS::I18N
