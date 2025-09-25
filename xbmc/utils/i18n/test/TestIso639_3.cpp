/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso639_3.h"
#include "utils/i18n/TableISO639.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

TEST(TestIso639_3, LookupByCode1)
{
  uint32_t longCode = StringToLongCode("aaa");
  std::optional<std::string> result;
  result = CIso639_3::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Ghotuo");

  longCode = StringToLongCode("boh");
  result = CIso639_3::LookupByCode(longCode);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Boma");

  // value in the range reserved for private use - will never match
  longCode = StringToLongCode("qaa");
  result = CIso639_3::LookupByCode(longCode);
  EXPECT_FALSE(result.has_value());
}

TEST(TestIso639_3, LookupByCode2)
{
  std::optional<std::string> result;

  result = CIso639_3::LookupByCode("aaa");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Ghotuo");

  result = CIso639_3::LookupByCode("boh");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Boma");

  // value in the range reserved for private use - will never match
  result = CIso639_3::LookupByCode("qaa");
  EXPECT_FALSE(result.has_value());
}

TEST(TestIso639_3, LookupByName)
{
  std::optional<std::string> result;

  result = CIso639_3::LookupByName("Puyuma");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(*result, "pyu");

  result = CIso639_3::LookupByName("not a language");
  EXPECT_FALSE(result.has_value());
}
