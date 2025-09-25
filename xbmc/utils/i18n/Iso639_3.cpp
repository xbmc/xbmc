/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso639_3.h"

#include "utils/StringUtils.h"
#include "utils/i18n/Iso639_3_Table.h"
#include "utils/i18n/TableISO639.h"

#include <algorithm>

using namespace KODI::UTILS::I18N;

std::optional<std::string> CIso639_3::LookupByCode(std::string_view code)
{
  const uint32_t longCode = StringToLongCode(code);
  return LookupByCode(longCode);
}

std::optional<std::string> CIso639_3::LookupByCode(uint32_t longCode)
{
  auto it = std::ranges::lower_bound(TableISO639_3, longCode, {}, &LCENTRY::code);
  if (it != TableISO639_3.end() && longCode == it->code)
  {
    return std::string{it->name};
  }
  return std::nullopt;
}

namespace
{
bool PrepareTableByName()
{
  std::ranges::sort(
      TableISO639_3ByName, [](std::string_view a, std::string_view b)
      { return StringUtils::CompareNoCase(a, b) < 0; }, &LCENTRY::name);
  return true;
}
} // namespace

std::optional<std::string> CIso639_3::LookupByName(std::string_view name)
{
  static bool initialized = PrepareTableByName();

  auto it = std::ranges::lower_bound(
      TableISO639_3ByName, name, [](std::string_view a, std::string_view b)
      { return StringUtils::CompareNoCase(a, b) < 0; }, &LCENTRY::name);
  if (it != TableISO639_3ByName.end() && StringUtils::EqualsNoCase(it->name, name))
    return LongCodeToString(it->code);

  return std::nullopt;
}
