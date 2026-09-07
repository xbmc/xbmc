/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/LanguageTag.h"
#include "language/i18n/Collation.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE::I18N;
using KODI::LANGUAGE::CLanguageTag;

namespace
{
constexpr wchar_t AE_LOWER = 0x00E6;
constexpr wchar_t AE_UPPER = 0x00C6;
constexpr wchar_t OE_LOWER = 0x00F8;
constexpr wchar_t OE_UPPER = 0x00D8;
constexpr wchar_t AA_LOWER = 0x00E5;
constexpr wchar_t AA_UPPER = 0x00C5;
constexpr wchar_t A_UMLAUT_LOWER = 0x00E4;
constexpr wchar_t A_UMLAUT_UPPER = 0x00C4;
constexpr wchar_t O_UMLAUT_LOWER = 0x00F6;
constexpr wchar_t O_UMLAUT_UPPER = 0x00D6;
constexpr wchar_t E_ACUTE_LOWER = 0x00E9;
} // namespace

TEST(TestI18nCollation, NordicCollationWeight)
{
  // Norwegian/Danish alphabet order: ... x y z ae oe aa
  for (const std::string& code : {"nor", "nob", "nno", "dan"})
  {
    const CLanguageTag lang{CLanguageTag::Parse(code)};
    const wchar_t ae = NordicCollationWeight(lang, AE_LOWER);
    const wchar_t oe = NordicCollationWeight(lang, OE_LOWER);
    const wchar_t aa = NordicCollationWeight(lang, AA_LOWER);
    EXPECT_GT(ae, L'z');
    EXPECT_LT(ae, oe);
    EXPECT_LT(oe, aa);
    // Upper and lower case must weigh the same
    EXPECT_EQ(ae, NordicCollationWeight(lang, AE_UPPER));
    EXPECT_EQ(oe, NordicCollationWeight(lang, OE_UPPER));
    EXPECT_EQ(aa, NordicCollationWeight(lang, AA_UPPER));
    // Swedish/Finnish a-umlaut and o-umlaut sort with ae and oe rather than folding to a/o
    EXPECT_EQ(ae, NordicCollationWeight(lang, A_UMLAUT_LOWER));
    EXPECT_EQ(ae, NordicCollationWeight(lang, A_UMLAUT_UPPER));
    EXPECT_EQ(oe, NordicCollationWeight(lang, O_UMLAUT_LOWER));
    EXPECT_EQ(oe, NordicCollationWeight(lang, O_UMLAUT_UPPER));
  }

  // Swedish/Finnish alphabet order: ... x y z aa a-umlaut o-umlaut
  for (const std::string& code : {"swe", "fin"})
  {
    const CLanguageTag lang{CLanguageTag::Parse(code)};
    const wchar_t aa = NordicCollationWeight(lang, AA_LOWER);
    const wchar_t ao = NordicCollationWeight(lang, A_UMLAUT_LOWER);
    const wchar_t oe = NordicCollationWeight(lang, O_UMLAUT_LOWER);
    EXPECT_GT(aa, L'z');
    EXPECT_LT(aa, ao);
    EXPECT_LT(ao, oe);
  }

  // No override for other languages or unrelated codepoints
  EXPECT_EQ(NordicCollationWeight(CLanguageTag::Parse("eng"), AA_LOWER), 0);
  EXPECT_EQ(NordicCollationWeight(CLanguageTag::Parse("nob"), E_ACUTE_LOWER), 0);
}
