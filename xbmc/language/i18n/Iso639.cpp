/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso639.h"

#include "language/i18n/Iso639_2_Table.h"
#include "language/i18n/IsoCodes.h"
#include "language/i18n/TableLanguageCodes.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>

namespace
{
//! The low byte of a packed code, which is its last character
constexpr uint32_t LONG_CODE_CHAR_MASK{(1u << CHAR_BIT) - 1};
} // namespace

namespace KODI::LANGUAGE::I18N
{
std::string LongCodeToString(uint32_t code)
{
  // Build the string in reverse order since appending to a string is more efficient than inserting
  // at position 0 and shifting the existing contents
  std::string ret;
  for (std::size_t j = 0; j < LONG_CODE_LENGTH; j++)
  {
    const char c = static_cast<char>(code & LONG_CODE_CHAR_MASK);
    if (c == '\0')
      break;
    ret.push_back(c);
    code >>= CHAR_BIT;
  }
  // Reverse the string for the final result
  std::ranges::reverse(ret);
  return ret;
}

std::optional<std::string> CIso639::Alpha2ToAlpha3B(std::string_view code)
{
  if (code.length() != ALPHA2_CODE_LENGTH)
    return std::nullopt;

  const auto it = std::ranges::lower_bound(TableISO639_1, code, {}, &ISO639_1::alpha2);
  if (it != TableISO639_1.end() && it->alpha2 == code)
    return std::string{it->alpha3B};

  return std::nullopt;
}

std::optional<std::string> CIso639::Alpha3ToAlpha2(std::string_view code)
{
  if (code.length() != ALPHA3_CODE_LENGTH)
    return std::nullopt;

  // The table is keyed by the bibliographic form, so a terminological code is mapped over first
  const std::string bCode{TCodeToBCode(code).value_or(std::string{code})};

  if (const auto alpha2 = Alpha2OfAlpha3B(bCode))
    return std::string{*alpha2};

  return std::nullopt;
}

std::optional<std::string> CIso639::TCodeToBCode(std::string_view tCode)
{
  const uint32_t longCode = StringToLongCode(tCode);

  auto it =
      std::ranges::lower_bound(ISO639_2_TB_Mappings, longCode, {}, &ISO639_2_TB::terminological);
  if (it != ISO639_2_TB_Mappings.end() && it->terminological == longCode)
    return LongCodeToString(it->bibliographic);

  return std::nullopt;
}

std::optional<std::string> CIso639::BCodeToTCode(std::string_view bCode)
{
  const uint32_t longCode = StringToLongCode(bCode);

  auto it =
      std::ranges::lower_bound(ISO639_2_TB_MappingsByB, longCode, {}, &ISO639_2_TB::bibliographic);
  if (it != ISO639_2_TB_MappingsByB.end() && longCode == it->bibliographic)
    return LongCodeToString(it->terminological);

  return std::nullopt;
}
} // namespace KODI::LANGUAGE::I18N
