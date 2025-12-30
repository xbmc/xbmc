/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47.h"
#include "utils/i18n/Bcp47Formatter.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

struct TestFormatting
{
  std::string input;
  std::string english;
};

std::ostream& operator<<(std::ostream& os, const TestFormatting& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestFormatting FormattingTests[] = {
    {"en", "English"},
    {"eng", "English"},
    {"en-AU", "English (Australia)"},
    {"es-419", "Spanish (419)"}, // M.49 region - result will change with registry support
    {"zh-yue-Hant-HK", "Chinese (yue, Hant, Hong Kong)"}, // result will change with registry support
    {"yue-Hant", "yue (Hant)"}, // result will change with registry support
    // All subtags
    {"zz-ext-exz-bcde-fg-abcde-0abc-e-abcd-ef-f-ef-x-a-bcd",
     "zz (ext-exz, Bcde, FG, abcde-0abc, e-abcd-ef-f-ef, x-a-bcd)"},
    // Private use only
    {"x-a-bcd", "x-a-bcd"},
    // Irregular grandfathered - result will change with registry support
    {"i-ami", "i-ami"},
};
// clang-format on

class FormatTester : public testing::Test, public testing::WithParamInterface<TestFormatting>
{
};

TEST_P(FormatTester, FormatRaw)
{
  // Well-formed tags (as well as grandfathered tags) should round-trip except for the casing
  const auto& param = GetParam();
  const auto tag = CBcp47::ParseTag(param.input);
  EXPECT_TRUE(tag.has_value());
  if (tag.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(StringUtils::ToLower(param.input), StringUtils::ToLower(tag.value().Format()));
  }
}

TEST_P(FormatTester, FormatEnglish)
{
  const auto& param = GetParam();
  const auto tag = CBcp47::ParseTag(param.input);
  EXPECT_TRUE(tag.has_value());
  if (tag.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.english, tag.value().Format(Bcp47FormattingStyle::FORMAT_ENGLISH));
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47Formatter, FormatTester, testing::ValuesIn(FormattingTests));

struct TestRecommendedCasing
{
  std::string input;
  std::string expected;
};

std::ostream& operator<<(std::ostream& os, const TestRecommendedCasing& rhs)
{
  return os << rhs.input;
}

const TestRecommendedCasing RecommendedCasingTests[] = {
    {"Ab", "ab"},
    {"aB", "ab"},
    {"ab-ExT-eXt", "ab-ext-ext"},
    {"En-Ca-X-cA", "en-CA-x-ca"},
    {"eN-cA-x-cA", "en-CA-x-ca"},
    {"az-lAtN-x-Latn", "az-Latn-x-latn"},
    {"ab-AbCdE-bCdEfGhI", "ab-abcde-bcdefghi"},
    {"Zh-GuOyU", "zh-guoyu"},
};

class RecommendedCasingTester : public testing::Test,
                                public testing::WithParamInterface<TestRecommendedCasing>
{
};

TEST_P(RecommendedCasingTester, ParseTag)
{
  const auto& param = GetParam();
  auto actual = CBcp47::ParseTag(param.input);

  EXPECT_TRUE(actual.has_value());
  if (actual.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, actual->Format());
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47Formatter,
                         RecommendedCasingTester,
                         testing::ValuesIn(RecommendedCasingTests));
