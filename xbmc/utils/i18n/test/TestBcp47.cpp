/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47.h"
#include "utils/i18n/Bcp47Parser.h"
#include "utils/i18n/test/TestI18nUtils.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

namespace KODI::UTILS::I18N
{
std::ostream& operator<<(std::ostream& os, const CBcp47& obj)
{
  if (obj.GetType() == Bcp47TagType::GRANDFATHERED) [[unlikely]]
  {
    os << "BCP47 (grandfathered: " << obj.GetGrandfathered() << ")";
    return os;
  }

  os << "BCP47 (language: " << obj.GetLanguage() << ", extended languages: {"
     << StringUtils::Join(obj.GetExtLangs(), ",") << "}, script: " << obj.GetScript()
     << ", region: " << obj.GetRegion() << ", variants: {"
     << StringUtils::Join(obj.GetVariants(), ",") << "}, extensions: {";

  for (const auto& ext : obj.GetExtensions())
    os << ext << " ";

  os << "}, private use: {" << StringUtils::Join(obj.GetPrivateUse(), ", ") << "}, ";
  os << "grandfathered: " << obj.GetGrandfathered() << ")";

  return os;
}
} // namespace KODI::UTILS::I18N

struct TestBcp47ParseTag
{
  std::string input;
  ParsedBcp47Tag expected;
  bool status{true};
};

std::ostream& operator<<(std::ostream& os, const TestBcp47ParseTag& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestBcp47ParseTag ParseBcp47Tests[] = {
    {"ab", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {}, {}, {}, ""}},
    // invalid, more than 8 letters
    {"montenegro", {}, false},
    {"", {}, false},
    // Combine all subtags
    {"ab-ext-bcde-fg-abcde-0abc-1def-e-abcd-ef-f-ef-x-a-bcd",
      {Bcp47TagType::WELL_FORMED, "ab", {"ext"}, "bcde", "fg", {{"abcde"}, {"0abc"}, {"1def"}}, {{'e', {{"abcd"}, {"ef"}}},{'f', {{"ef"}}}}, {"a", "bcd"}, ""}},
    // Just a private use subtag
    {"x-a-bcd", {Bcp47TagType::PRIVATE_USE, "", {}, "", "", {}, {}, {"a", "bcd"}, ""}},
    // Irregular grandfathered
    {"i-ami", {Bcp47TagType::GRANDFATHERED, "", {}, "", "", {}, {}, {}, "i-ami"}},
    // Regular grandfathered
    {"cel-gaulish", {Bcp47TagType::GRANDFATHERED, "", {}, "", "", {}, {}, {}, "cel-gaulish"}},
};
// clang-format on

class Bcp47ParseTagTester : public testing::Test,
                            public testing::WithParamInterface<TestBcp47ParseTag>
{
};

TEST_P(Bcp47ParseTagTester, ParseTag)
{
  auto& param = GetParam();
  auto actual = CBcp47::ParseTag(param.input);

  EXPECT_EQ(param.status, actual.has_value());
  if (param.status && actual.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, actual.value());
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47, Bcp47ParseTagTester, testing::ValuesIn(ParseBcp47Tests));

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

TEST(TestI18nBcp47, ValidateLanguage)
{
  // ISO 639-1
  auto tag = CBcp47::ParseTag("en");
  EXPECT_TRUE(tag->IsValid());

  // ISO 639-2 that has an equivalent ISO 639-1
  tag = CBcp47::ParseTag("eng");
  EXPECT_FALSE(tag->IsValid());

  // ISO 639-2/B that has an equivalent ISO 639-1
  tag = CBcp47::ParseTag("tib");
  EXPECT_FALSE(tag->IsValid());

  // ISO 639-2/T that has an equivalent ISO 639-1
  tag = CBcp47::ParseTag("bod");
  EXPECT_FALSE(tag->IsValid());

  // ISO 639-2 without ISO 639-1 equivalent
  tag = CBcp47::ParseTag("ady");
  EXPECT_TRUE(tag->IsValid());

  // Private use ISO 639-2
  tag = CBcp47::ParseTag("qaa");
  EXPECT_TRUE(tag->IsValid());
}

TEST(TestI18nBcp47, ValidateRegion)
{
  // ISO 3166-1
  auto tag = CBcp47::ParseTag("en-GB");
  EXPECT_TRUE(tag->IsValid());

  // Private use
  tag = CBcp47::ParseTag("en-AA");
  EXPECT_TRUE(tag->IsValid());

  // UN M.49 - support to be added with IANA language subtags registry
  tag = CBcp47::ParseTag("es-419");
  EXPECT_FALSE(tag->IsValid());
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

TEST(TestI18nBcp47, ValidateExtLang)
{
  auto tag = CBcp47::ParseTag("ar-aao");
  EXPECT_TRUE(tag->IsValid());

  // Multiple extlangs are rejected whether they exist in registry or not
  tag = CBcp47::ParseTag("en-abc-def");
  EXPECT_FALSE(tag->IsValid());

  //! @todo add test for inexistent extlang after implementation of the registry
}
