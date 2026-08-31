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

TEST(TestI18nCollation, NordicCollationWeight)
{
  // Norwegian/Danish alphabet order: ... x y z æ ø å
  for (const std::string& code : {"nor", "nob", "nno", "dan"})
  {
    const CLanguageTag lang{CLanguageTag::Parse(code)};
    const wchar_t ae = NordicCollationWeight(lang, L'æ'); // æ
    const wchar_t oe = NordicCollationWeight(lang, L'ø'); // ø
    const wchar_t aa = NordicCollationWeight(lang, L'å'); // å
    EXPECT_GT(ae, L'z');
    EXPECT_LT(ae, oe);
    EXPECT_LT(oe, aa);
    // Upper and lower case must weigh the same
    EXPECT_EQ(ae, NordicCollationWeight(lang, L'Æ')); // Æ
    EXPECT_EQ(oe, NordicCollationWeight(lang, L'Ø')); // Ø
    EXPECT_EQ(aa, NordicCollationWeight(lang, L'Å')); // Å
    // Swedish/Finnish ä/ö sort with æ/ø rather than folding to a/o
    EXPECT_EQ(ae, NordicCollationWeight(lang, L'ä')); // ä
    EXPECT_EQ(ae, NordicCollationWeight(lang, L'Ä')); // Ä
    EXPECT_EQ(oe, NordicCollationWeight(lang, L'ö')); // ö
    EXPECT_EQ(oe, NordicCollationWeight(lang, L'Ö')); // Ö
  }

  // Swedish/Finnish alphabet order: ... x y z å ä ö
  for (const std::string& code : {"swe", "fin"})
  {
    const CLanguageTag lang{CLanguageTag::Parse(code)};
    const wchar_t aa = NordicCollationWeight(lang, L'å'); // å
    const wchar_t ao = NordicCollationWeight(lang, L'ä'); // ä
    const wchar_t oe = NordicCollationWeight(lang, L'ö'); // ö
    EXPECT_GT(aa, L'z');
    EXPECT_LT(aa, ao);
    EXPECT_LT(ao, oe);
  }

  // No override for other languages or unrelated codepoints
  EXPECT_EQ(NordicCollationWeight(CLanguageTag::Parse("eng"),
                                                  L'å'),
            0); // å
  EXPECT_EQ(NordicCollationWeight(CLanguageTag::Parse("nob"),
                                                  L'é'),
            0); // é
}
