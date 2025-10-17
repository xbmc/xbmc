/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso639_1.h"

#include "utils/StringUtils.h"
#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_1_Table.h"

#include <algorithm>

using namespace KODI::UTILS::I18N;

std::optional<std::string> CIso639_1::LookupByCode(std::string_view code)
{
  const uint32_t longCode = StringToLongCode(code);
  return LookupByCode(longCode);
}

std::optional<std::string> CIso639_1::LookupByCode(uint32_t longCode)
{
  {
    auto it = std::ranges::lower_bound(TableISO639_1ByCode, longCode, {}, &LCENTRY::code);
    if (it != TableISO639_1ByCode.end() && longCode == it->code)
      return std::string{it->name};
  }
  {
    auto it = std::ranges::lower_bound(TableISO639_1_DeprByCode, longCode, {}, &LCENTRY::code);
    if (it != TableISO639_1_DeprByCode.end() && longCode == it->code)
      return std::string{it->name};
  }
  return std::nullopt;
}

std::optional<std::string> CIso639_1::LookupByName(std::string_view name)
{
  {
    auto it = std::ranges::lower_bound(
        TableISO639_1ByName, name, [](std::string_view a, std::string_view b)
        { return StringUtils::CompareNoCase(a, b) < 0; }, &LCENTRY::name);
    if (it != TableISO639_1ByName.end() && StringUtils::EqualsNoCase(it->name, name))
      return LongCodeToString(it->code);
  }
  {
    auto it = std::ranges::lower_bound(
        TableISO639_1_DeprByName, name, [](std::string_view a, std::string_view b)
        { return StringUtils::CompareNoCase(a, b) < 0; }, &LCENTRY::name);
    if (it != TableISO639_1_DeprByName.end() && StringUtils::EqualsNoCase(it->name, name))
      return LongCodeToString(it->code);
  }
  return std::nullopt;
}

bool CIso639_1::ListLanguages(std::map<std::string, std::string>& langMap)
{
  std::ranges::transform(TableISO639_1ByName, std::inserter(langMap, langMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(LongCodeToString(e.code), std::string{e.name}); });

  std::ranges::transform(TableISO639_1_DeprByName, std::inserter(langMap, langMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(LongCodeToString(e.code), std::string{e.name}); });

  return true;
}
