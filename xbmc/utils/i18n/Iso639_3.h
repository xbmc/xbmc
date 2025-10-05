/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace KODI::UTILS::I18N
{
class CIso639_3
{
public:
  CIso639_3(CIso639_3&) = delete;
  void operator=(const CIso639_3&) = delete;

  static CIso639_3& GetInstance();

  std::optional<std::string> LookupByCode(std::string_view code);
  std::optional<std::string> LookupByCode(uint32_t longCode);

  std::optional<std::string> LookupByName(std::string_view name);

private:
  CIso639_3();
};
} // namespace KODI::UTILS::I18N
