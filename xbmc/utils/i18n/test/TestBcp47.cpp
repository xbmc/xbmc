/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

TEST(TestI18nBcp47, Canonicalize)
{
  // ISO 639-1 code
  auto tag = CBcp47::ParseTag("en");
  tag->Canonicalize();
  EXPECT_EQ(tag->Format(), "en");

  // unsorted extensions
  tag = CBcp47::ParseTag("ab-d-ef-g-hi-a-bc");
  tag->Canonicalize();
  EXPECT_EQ(tag->Format(), "ab-a-bc-d-ef-g-hi");
}

TEST(TestI18nBcp47, ValidateVariants)
{
  auto tag = CBcp47::ParseTag("en-variant");
  EXPECT_TRUE(tag->IsValid());

  tag = CBcp47::ParseTag("en");
  EXPECT_TRUE(tag->IsValid());

  tag = CBcp47::ParseTag("en-variant-variant");
  EXPECT_FALSE(tag->IsValid());
}

TEST(TestI18nBcp47, ValidateExtensions)
{
  // extension with multiple segments
  auto tag = CBcp47::ParseTag("ab-a-bcdefghi-jk");
  EXPECT_TRUE(tag->IsValid());

  // multiple extensions in non-alphabetical order are OK
  tag = CBcp47::ParseTag("ab-b-ab-a-cd");
  EXPECT_TRUE(tag->IsValid());

  // no extension
  tag = CBcp47::ParseTag("ab");
  EXPECT_TRUE(tag->IsValid());

  // duplicate extensions not allowed
  tag = CBcp47::ParseTag("ab-a-bc-a-de");
  EXPECT_FALSE(tag->IsValid());
}

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
  auto& param = GetParam();

  auto actual = CBcp47::ParseTag(param.input);

  EXPECT_TRUE(actual.has_value());

  if (actual.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, actual->Format());
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47,
                         RecommendedCasingTester,
                         testing::ValuesIn(RecommendedCasingTests));

TEST(TestI18nBcp47, ToAudioLanguageTag)
{
  std::string input = "en";
  std::string expected = "en";

  auto actual = CBcp47::ParseTag(input);
  EXPECT_TRUE(actual.has_value());
  if (actual.has_value())
  {
    EXPECT_EQ(expected, actual->ToAudioLanguageTag().Format());
  }

  // with script
  input = "zh-Hant";
  expected = "zh";

  actual = CBcp47::ParseTag(input);
  EXPECT_TRUE(actual.has_value());
  if (actual.has_value())
  {
    EXPECT_EQ(expected, actual->ToAudioLanguageTag().Format());
  }

  // all possible subtags
  input = "ab-ext-bcde-fg-abcde-e-abcd-x-a";
  expected = "ab-ext-FG-abcde-e-abcd-x-a";

  actual = CBcp47::ParseTag(input);
  EXPECT_TRUE(actual.has_value());
  if (actual.has_value())
  {
    EXPECT_EQ(expected, actual->ToAudioLanguageTag().Format());
  }

  // grandfathered
  input = "zh-guoyu";
  expected = "zh-guoyu";

  actual = CBcp47::ParseTag(input);
  EXPECT_TRUE(actual.has_value());
  if (actual.has_value())
  {
    CBcp47 audio = actual->ToAudioLanguageTag();
    EXPECT_TRUE(audio.IsGrandfathered());
    EXPECT_EQ(expected, audio.Format());
  }
}
