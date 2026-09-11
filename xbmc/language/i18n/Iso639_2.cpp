/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso639_2.h"

#include "utils/StringUtils.h"
#include "language/i18n/Iso639.h"
#include "language/i18n/Iso639_2_Table.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <mutex>

namespace KODI::LANGUAGE::I18N
{
namespace
{
// Array containing main + additional names, sorted by case-insensitive name
std::array<LCENTRY, ISO639_2_COUNT + ISO639_2_ADDL_NAMES_COUNT> g_TableISO639_2AllNames;

// Ensure initialization thread-safety
std::once_flag g_initializeOnce;

class CIso639_2_Initializer
{
public:
  CIso639_2_Initializer() { std::call_once(g_initializeOnce, Initialize); }

private:
  static void Initialize()
  {
    // Prepare TableISO639_2AllNames at runtime - exceeds constexpr step limits on some compilers.
    // Concatenation of main + additional names, sorted by name
    std::ranges::copy(TableISO639_2ByCode, g_TableISO639_2AllNames.begin());
    std::ranges::copy(TableISO639_2_Names, g_TableISO639_2AllNames.begin() + ISO639_2_COUNT);

    std::ranges::sort(g_TableISO639_2AllNames, {},
                      [](const auto& elem) { return StringUtils::ToLower(elem.name); });

    // Check that names are unique
    assert(std::ranges::adjacent_find(g_TableISO639_2AllNames, {}, [](const auto& elem)
                                      { return StringUtils::ToLower(elem.name); }) ==
           g_TableISO639_2AllNames.end());
  }
};
// Ensure initialization of the table when the program starts before any class function can run.
CIso639_2_Initializer g_initializer;
} // namespace
} // namespace KODI::LANGUAGE::I18N

using namespace KODI::LANGUAGE::I18N;

bool CIso639_2::ListLanguages(std::map<std::string, std::string>& langMap)
{
  // ISO 639-2/T codes, under the name the standard gives as the language's own rather than
  // whichever of its alternative names happens to sort first
  std::ranges::transform(TableISO639_2ByCode, std::inserter(langMap, langMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(LongCodeToString(e.code), std::string{e.name}); });

  // ISO 639-2/B codes that are different from the T code.
  for (const ISO639_2_TB& tb : ISO639_2_TB_Mappings)
  {
    const std::string bCode = LongCodeToString(tb.bibliographic);

    // Lookup the 639-2/T code
    //! @todo Maybe could be avoided by building a constexpr 639-2/B to name array at compile time.
    //! Is it worth the effort and memory though.
    auto it = std::ranges::lower_bound(TableISO639_2ByCode, tb.terminological, {}, &LCENTRY::code);
    if (it != TableISO639_2ByCode.end() && tb.terminological == it->code)
      langMap[bCode] = std::string(it->name);
  }

  return true;
}

bool CIso639_2::ListLanguageNames(std::map<std::string, std::string>& nameMap)
{
  std::ranges::transform(g_TableISO639_2AllNames, std::inserter(nameMap, nameMap.end()),
                         [](const LCENTRY& e)
                         { return std::make_pair(std::string{e.name}, LongCodeToString(e.code)); });

  return true;
}

std::optional<uint32_t> CIso639_2::BCodeToTCode(uint32_t bCode)
{
  auto it =
      std::ranges::lower_bound(ISO639_2_TB_MappingsByB, bCode, {}, &ISO639_2_TB::bibliographic);
  if (it != ISO639_2_TB_MappingsByB.end() && bCode == it->bibliographic)
    return it->terminological;

  return std::nullopt;
}

std::optional<std::string> CIso639_2::TCodeToBCode(std::string_view tCode)
{
  const uint32_t longCode = StringToLongCode(tCode);

  auto it =
      std::ranges::lower_bound(ISO639_2_TB_Mappings, longCode, {}, &ISO639_2_TB::terminological);
  if (it != ISO639_2_TB_Mappings.end() && it->terminological == longCode)
    return LongCodeToString(it->bibliographic);

  return std::nullopt;
}
