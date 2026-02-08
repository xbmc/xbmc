/*
 *  Copyright (C) 2025-2026 Team Kodi
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

struct TestBcp47ValidateTag
{
  std::string input;
  bool expectedIsValid{true};
};

std::ostream& operator<<(std::ostream& os, const TestBcp47ValidateTag& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestBcp47ValidateTag ValidityBcp47Tests[] = {
  {"zz", false}, // undefined
  // Language subtags
  {"en", true}, // ISO 639-1
  {"eng", false}, // ISO 639-2 that has an equivalent ISO 639-1
  {"tib", false}, // ISO 639-2/B that has an equivalent ISO 639-1
  {"bod", false}, // ISO 639-2/T that has an equivalent ISO 639-1
  {"ady", true}, // ISO 639-2 without ISO 639-1 equivalent
  {"qaa", true}, // Private use ISO 639-2
  // Extlang subtags
  {"ar-aao", true},
  {"en-abc-def", false}, // Multiple extlangs are rejected whether they exist in registry or not
  //! @todo add test for inexistent extlang after implementation of the registry
  // Region subtags
  {"en-GB", true}, // ISO 3166-1
  {"en-AA", true}, // Private use
  {"es-419", false}, // UN M.49 - support to be added with IANA language subtags registry
  // Variant subtags
  {"en-variant", true},
  {"en-variant-variant", false},
  // Extensions subtags
  {"ab-a-bcdefghi-jk", true}, // extension with multiple segments
  {"ab-b-ab-a-cd", true}, // multiple extensions in non-alphabetical order are OK
  {"ab-a-bc-a-de", false}, // duplicate extensions not allowed
};
// clang-format on

class Bcp47ValidateTagTester : public testing::Test,
                               public testing::WithParamInterface<TestBcp47ValidateTag>
{
};

TEST_P(Bcp47ValidateTagTester, ParseTag)
{
  auto& param = GetParam();
  auto actual = CBcp47::ParseTag(param.input);

  EXPECT_TRUE(actual.has_value());
  EXPECT_EQ(param.expectedIsValid, actual.value().IsValid());
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47,
                         Bcp47ValidateTagTester,
                         testing::ValuesIn(ValidityBcp47Tests));

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
