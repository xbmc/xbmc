/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso3166_1.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

TEST(TestI18nIso3166_1, Alpha2ToAlpha3)
{
  std::optional<std::string> result;

  result = CIso3166_1::Alpha2ToAlpha3("ai");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "aia");

  // zz is reserved for private use and won't ever be allocated
  result = CIso3166_1::Alpha2ToAlpha3("zz");
  EXPECT_FALSE(result.has_value());

  // alpha-2 code is expected in lower case
  result = CIso3166_1::Alpha2ToAlpha3("AI");
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso3166_1, Alpha3ToAlpha2)
{
  std::optional<std::string> result;

  result = CIso3166_1::Alpha3ToAlpha2("aia");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "ai");

  // zzz is reserved for private use and won't ever be allocated
  result = CIso3166_1::Alpha3ToAlpha2("zzz");
  EXPECT_FALSE(result.has_value());
}

TEST(TestI18nIso3166_1, ContainsAlpha3)
{
  EXPECT_TRUE(CIso3166_1::ContainsAlpha3("aia"));
  EXPECT_FALSE(CIso3166_1::ContainsAlpha3("zzz"));
}
