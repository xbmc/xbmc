/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/Iso639.h"
#include "language/i18n/Iso639_2.h"

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE::I18N;

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
