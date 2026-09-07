/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso639.h"

#include "language/i18n/Iso639_2.h"
#include "language/i18n/IsoCodes.h"
#include "language/i18n/TableLanguageCodes.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>

namespace
{
//! The low byte of a packed code, which is its last character
constexpr uint32_t LONG_CODE_CHAR_MASK{(1u << CHAR_BIT) - 1};

std::string Code(std::string_view text)
{
  std::string code{StringUtils::ToLower(text)};
  StringUtils::Trim(code);
  return code;
}
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

std::optional<std::string> CIso639::Alpha2ToAlpha3B(std::string_view alpha2)
{
  const std::string code{Code(alpha2)};
  if (code.length() != ALPHA2_CODE_LENGTH)
    return std::nullopt;

  const auto it = std::ranges::lower_bound(LanguageCodes, code, {}, &ISO639::iso639_1);
  if (it != LanguageCodes.end() && it->iso639_1 == code)
    return std::string{it->iso639_2b};

  const auto deprecated =
      std::ranges::lower_bound(DeprecatedLanguageCodes, code, {}, &ISO639::iso639_1);
  if (deprecated != DeprecatedLanguageCodes.end() && deprecated->iso639_1 == code)
    return std::string{deprecated->iso639_2b};

  return std::nullopt;
}

std::optional<std::string> CIso639::Alpha3ToAlpha2(std::string_view alpha3)
{
  const std::string code{Code(alpha3)};
  if (code.length() != ALPHA3_CODE_LENGTH)
    return std::nullopt;

  // The table is keyed by the bibliographic form, so a terminological code is mapped over first
  const std::string bCode{CIso639_2::TCodeToBCode(code).value_or(code)};

  const auto it = std::ranges::lower_bound(LanguageCodesByIso639_2b, bCode, {}, &ISO639::iso639_2b);
  if (it != LanguageCodesByIso639_2b.end() && it->iso639_2b == bCode && !it->iso639_1.empty())
    return std::string{it->iso639_1};

  return std::nullopt;
}
} // namespace KODI::LANGUAGE::I18N
