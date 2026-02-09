/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47.h"
#include "utils/i18n/Bcp47Formatter.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryManager.h"
#include "utils/i18n/test/TestI18nUtils.h"

#include <memory>

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
    {"eng", "eng"}, // eng is  not valid
    {"en-AU", "English (Australia)"},
    {"es-419", "Spanish (Latin America and the Caribbean)"},
    {"zh-yue-Hant-HK", "Chinese (Yue Chinese, Han (Traditional variant), Hong Kong)"},
    {"yue-Hant", "Yue Chinese (Han (Traditional variant))"},
    // All subtags
    {"zz-ext-exz-bcde-fg-abcde-0abc-e-abcd-ef-f-ef-x-a-bcd",
     "zz (ext-exz, Bcde, FG, abcde-0abc, e-abcd-ef-f-ef, x-a-bcd)"},
    // Private use only
    {"x-a-bcd", "x-a-bcd"},
    // Irregular grandfathered
    {"i-ami", "Amis"},
};
// clang-format on

class FormatTester : public testing::Test, public testing::WithParamInterface<TestFormatting>
{
protected:
  static CSubTagRegistryManager m_registry;

  static void SetUpTestSuite()
  {
    // Runs once before the first test in this suite

    // Use a small controlled subtags registry for more reliable tests
    // clang-format off
    std::vector<RegistryFileRecord> param = {
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "en"}, {"Description", "English"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "es"}, {"Description", "Spanish"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "qaa"}, {"Description", "longer description"}, {"Description", "short description"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "yue"}, {"Description", "Yue Chinese"}, {"Description", "Cantonese"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "zh"}, {"Description", "Chinese"}}},
        RegistryFileRecord{{{"Type", "extlang"}, {"Subtag", "yue"}, {"Description", "Yue Chinese"}, {"Description", "Cantonese"},{"Prefix", "zh"}}},
        RegistryFileRecord{{{"Type", "script"}, {"Subtag", "Hant"}, {"Description", "Han (Traditional variant)"}}},
        RegistryFileRecord{{{"Type", "region"}, {"Subtag", "AU"}, {"Description", "Australia"}}},
        RegistryFileRecord{{{"Type", "region"}, {"Subtag", "HK"}, {"Description", "Hong Kong"}}},
        RegistryFileRecord{{{"Type", "region"}, {"Subtag", "419"}, {"Description", "Latin America and the Caribbean"}}},
        RegistryFileRecord{{{"Type", "variant"}, {"Subtag", "ao1990"}, {"Description", "Portuguese Language Orthographic Agreement of 1990 (Acordo Ortografico da Lingua Portuguesa de 1990)"}}},
        RegistryFileRecord{{{"Type", "grandfathered"}, {"Subtag", "i-ami"}, {"Description", "Amis"}}},
};
    // clang-format on

    std::unique_ptr<IRegistryRecordProvider> provider =
        std::make_unique<CMemoryRecordProvider>(std::move(param));

    EXPECT_TRUE(m_registry.Initialize(std::move(provider)));
  }

  static void TearDownTestSuite() { m_registry.Deinitialize(); }
};

// Define the static member
CSubTagRegistryManager FormatTester::m_registry;

TEST_P(FormatTester, FormatRaw)
{
  // Well-formed tags (as well as grandfathered tags) should round-trip except for the casing
  const auto& param = GetParam();
  const auto tag = CBcp47::ParseTag(param.input, &m_registry);
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
  const auto tag = CBcp47::ParseTag(param.input, &m_registry);
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

struct TestDebugFormatting
{
  std::string input;
  std::string expected;
};

std::ostream& operator<<(std::ostream& os, const TestDebugFormatting& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestDebugFormatting DebugFormattingTests[] = {
    {"en", "BCP47 (well formed, valid) language: en, ext langs: {}, script: , region: , variants: {}, extensions: {}, private use: {}, grandfathered:"},
    {"zz", "BCP47 (well formed, invalid) language: zz, ext langs: {}, script: , region: , variants: {}, extensions: {}, private use: {}, grandfathered:"},
    {"zz-ext-exz-bcde-fg-abcde-0abc-e-abcd-ef-f-ef-x-a-bcd",
        "BCP47 (well formed, invalid) language: zz, ext langs: {ext, exz}, script: bcde, region: fg, "
        "variants: {abcde, 0abc}, extensions: {name: e values: {abcd, ef} name: f values: {ef}}, "
        "private use: {a, bcd}, grandfathered:"},
    // Private use only
    {"x-a-bcd", "BCP47 (private use, valid) language: , ext langs: {}, script: , region: , variants: {}, "
        "extensions: {}, private use: {a, bcd}, grandfathered:"},
    // Grandfathered
    {"i-ami", "BCP47 (grandfathered, valid) language: , ext langs: {}, script: , region: , variants: {}, "
        "extensions: {}, private use: {}, grandfathered: i-ami"},
};
// clang-format on

class DebugFormatTester : public testing::Test,
                          public testing::WithParamInterface<TestDebugFormatting>
{
};

TEST_P(DebugFormatTester, Format)
{
  const auto& param = GetParam();
  const auto tag = CBcp47::ParseTag(param.input);
  EXPECT_TRUE(tag.has_value());
  if (tag.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, tag.value().Format(Bcp47FormattingStyle::FORMAT_DEBUG));
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47Formatter,
                         DebugFormatTester,
                         testing::ValuesIn(DebugFormattingTests));
