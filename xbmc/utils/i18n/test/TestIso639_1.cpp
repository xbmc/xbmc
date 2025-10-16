/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_1.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

TEST(TestI18nIso639_1, LookupByCode1)
{
  uint32_t longCode = StringToLongCode("aa");
  std::optional<std::string> result;
  result = CIso639_1::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Afar");

  longCode = StringToLongCode("zu");
  result = CIso639_1::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Zulu");

  longCode = StringToLongCode("pb");
  result = CIso639_1::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Portuguese (Brazil)");

  longCode = StringToLongCode("jw");
  result = CIso639_1::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Javanese");

  // there are no values reserved for private use - zz is not allocated at this time
  longCode = StringToLongCode("zz");
  result = CIso639_1::LookupByCode(longCode);
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso639_1, LookupByCode2)
{
  std::optional<std::string> result;

  result = CIso639_1::LookupByCode("aa");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Afar");

  result = CIso639_1::LookupByCode("zu");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Zulu");

  // there are no values reserved for private use - zz is not allocated at this time
  result = CIso639_1::LookupByCode("zz");
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso639_1, LookupByName)
{
  std::optional<std::string> result;

  result = CIso639_1::LookupByName("Afar");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "aa");

  result = CIso639_1::LookupByName("Zulu");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "zu");

  result = CIso639_1::LookupByName("Moldavian");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "mo");

  result = CIso639_1::LookupByName("Indonesian");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "id");

  result = CIso639_1::LookupByName("not a language");
  EXPECT_FALSE(result.has_value());
}
