/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Collation.h"

#include "language/LanguageTag.h"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

using namespace std::string_view_literals;

namespace
{

//! Norwegian and Danish place these letters at the end of the alphabet, in this order
constexpr std::array DANO_NORWEGIAN{"nb"sv, "nn"sv, "no"sv, "da"sv};

//! Swedish and Finnish place the same letters there in another order
constexpr std::array SWEDISH{"sv"sv, "fi"sv};

//! The weights of the three letters an alphabet places after z, in its order
constexpr wchar_t FIRST_AFTER_Z{L'z' + 1};
constexpr wchar_t SECOND_AFTER_Z{L'z' + 2};
constexpr wchar_t THIRD_AFTER_Z{L'z' + 3};

//! Whether a language is one of a named set
bool IsOneOf(std::span<const std::string_view> languages,
             const KODI::LANGUAGE::CLanguageTag& language)
{
  return std::ranges::any_of(languages, [&language](std::string_view subtag)
                             { return language.IsLanguage(subtag); });
}

} // unnamed namespace

namespace KODI::LANGUAGE::I18N
{

wchar_t NordicCollationWeight(const CLanguageTag& language, wchar_t codepoint) noexcept
{
  if (IsOneOf(DANO_NORWEGIAN, language))
  {
    // Norwegian/Danish alphabet order: ... x y z ae oe aa (U+00E6 U+00F8 U+00E5)
    // a-umlaut sorts with ae and o-umlaut with oe
    switch (codepoint)
    {
      case 0x00C6:
      case 0x00E6: // AE / ae
      case 0x00C4:
      case 0x00E4: // A-umlaut / a-umlaut
        return FIRST_AFTER_Z;
      case 0x00D8:
      case 0x00F8: // OE / oe
      case 0x00D6:
      case 0x00F6: // O-umlaut / o-umlaut
        return SECOND_AFTER_Z;
      case 0x00C5:
      case 0x00E5: // AA / aa
        return THIRD_AFTER_Z;
      default:
        return 0;
    }
  }

  if (IsOneOf(SWEDISH, language))
  {
    // Swedish/Finnish alphabet order: ... x y z aa a-umlaut o-umlaut (U+00E5 U+00E4 U+00F6)
    switch (codepoint)
    {
      case 0x00C5:
      case 0x00E5: // AA / aa
        return FIRST_AFTER_Z;
      case 0x00C4:
      case 0x00E4: // A-umlaut / a-umlaut
        return SECOND_AFTER_Z;
      case 0x00D6:
      case 0x00F6: // O-umlaut / o-umlaut
        return THIRD_AFTER_Z;
      default:
        return 0;
    }
  }

  return 0;
}

} // namespace KODI::LANGUAGE::I18N
