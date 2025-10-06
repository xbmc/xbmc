/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_2.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

TEST(TestI18nIso639_2, LookupByCode1)
{
  std::optional<std::string> result;

  // first language of main table
  uint32_t longCode = StringToLongCode("aar");
  result = CIso639_2::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Afar");

  // last language of main table
  longCode = StringToLongCode("zza");
  result = CIso639_2::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Zaza");

  // value in the range reserved for private use - will never match
  longCode = StringToLongCode("qaa");
  result = CIso639_2::LookupByCode(longCode);
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso639_2, LookupByCode2)
{
  std::optional<std::string> result;

  result = CIso639_2::LookupByCode("aar");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Afar");

  result = CIso639_2::LookupByCode("zza");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Zaza");

  result = CIso639_2::LookupByCode("qaa");
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso639_2, LookupByName)
{
  std::optional<std::string> result;

  // first language of main table
  result = CIso639_2::LookupByName("Afar");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "aar");

  // last language of main table
  result = CIso639_2::LookupByName("Zaza");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "zza");

  // first language of additional names
  result = CIso639_2::LookupByName("Abkhaz");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "abk");

  // last language of additional names
  result = CIso639_2::LookupByName("Zazaki");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "zza");

  result = CIso639_2::LookupByName("Finnish");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "fin");

  result = CIso639_2::LookupByName("not a language");
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso639_2, TCodeToBCode)
{
  std::optional<std::string> result;

  result = CIso639_2::TCodeToBCode("bod");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "tib");

  // ISO 639-2 T Code that doesn't have a matching B code
  result = CIso639_2::TCodeToBCode("zha");
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso639_2, BCodeToTCode)
{
  std::optional<uint32_t> result;

  uint32_t longCode = StringToLongCode("tib");
  result = CIso639_2::BCodeToTCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, StringToLongCode("bod"));

  // ISO 639-2 T Code that doesn't have a matching B code
  longCode = StringToLongCode("zha");
  result = CIso639_2::BCodeToTCode(longCode);
  EXPECT_FALSE(result.has_value());
}
