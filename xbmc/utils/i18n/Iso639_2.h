/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace KODI::UTILS::I18N
{
class CIso639_2
{
public:
  CIso639_2() = delete;

  static std::optional<std::string> LookupByCode(std::string_view tCode);
  static std::optional<std::string> LookupByCode(uint32_t longTcode);

  static std::optional<std::string> LookupByName(std::string_view name);

  static bool ListLanguages(std::map<std::string, std::string>& langMap);

  static std::optional<uint32_t> BCodeToTCode(uint32_t bCode);
  static std::optional<std::string> TCodeToBCode(std::string_view tCode);
};
} // namespace KODI::UTILS::I18N
