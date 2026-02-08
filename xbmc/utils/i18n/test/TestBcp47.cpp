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
#include "utils/i18n/Bcp47Registry/SubTagRegistryManager.h"
#include "utils/i18n/test/TestI18nUtils.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

namespace KODI::UTILS::I18N
{
std::ostream& operator<<(std::ostream& os, const CBcp47& obj)
{
  os << obj.Format(Bcp47FormattingStyle::FORMAT_DEBUG);
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
  {"en-aao-abh", false}, // Multiple extlangs are rejected whether they exist in registry or not
  {"en-aaa", false}, // does not exist
  // Script subtags
  {"en-Afak", true}, // ISO 15924
  {"en-aaaa", false}, // does not exist
  //! @todo add test for inexistent extlang after implementation of the registry
  // Region subtags
  {"en-GB", true}, // ISO 3166-1
  {"es-419", true}, // UN M.49
  {"en-AA", true}, // Private use
  {"en-000", false}, // does not exist
  // Variant subtags
  {"en-1606nict", true},
  // as of 2026-02-07 no example in the registry of (DIGIT 3alphanum) form
  {"en-1606nict-1694acad", true},
  {"en-invalid", false},
  // Extensions subtags
  {"ab-a-bcdefghi-jk", true}, // extension with multiple segments
  {"ab-b-ab-a-cd", true}, // multiple extensions in non-alphabetical order are OK
  {"ab-a-bc-a-de", false}, // duplicate extensions not allowed
  // Private use tags
  {"x-a-bcd", true},
  // Grandfathered tags
  {"i-ami", true}, // rregular grandfathered
  {"cel-gaulish", true}, // irregular grandfathered
};
// clang-format on

class Bcp47ValidateTagTester : public testing::Test,
                               public testing::WithParamInterface<TestBcp47ValidateTag>
{
protected:
  static CSubTagRegistryManager m_registry;

  static void SetUpTestSuite()
  {
    // Runs once before the first test in this suite

    // Use a small controlled subtags registry for more reliable tests
    // clang-format off
    std::vector<RegistryFileRecord> param = {
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "ab"}, {"Description", "Abkhazian"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "ar"}, {"Description", "Arabic"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "en"}, {"Description", "English"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "es"}, {"Description", "Spanish"}}},
        RegistryFileRecord{{{"Type", "language"}, {"Subtag", "ady"}, {"Description", "Adyghe"}}},
        RegistryFileRecord{{{"Type", "extlang"}, {"Subtag", "aao"}, {"Description", "Algerian Saharan Arabic"}, {"Prefix", "ar"}}},
        RegistryFileRecord{{{"Type", "extlang"}, {"Subtag", "abh"}, {"Description", "Tajiki Arabic"}, {"Prefix", "ar"}}},
        RegistryFileRecord{{{"Type", "script"}, {"Subtag", "Afak"}, {"Description", "Afaka"}}},
        RegistryFileRecord{{{"Type", "region"}, {"Subtag", "GB"}, {"Description", "United Kingdom"}}},
        RegistryFileRecord{{{"Type", "region"}, {"Subtag", "419"}, {"Description", "Latin America and the Caribbean"}}},
        RegistryFileRecord{{{"Type", "variant"}, {"Subtag", "1606nict"}, {"Description", "Late Middle French (to 1606)"}}},
        RegistryFileRecord{{{"Type", "variant"}, {"Subtag", "1694acad"}, {"Description", "Early Modern French"}}},
        RegistryFileRecord{{{"Type", "grandfathered"}, {"Subtag", "i-ami"}, {"Description", "Amis"}}},
        RegistryFileRecord{{{"Type", "grandfathered"}, {"Subtag", "cel-gaulish"}, {"Description", "Gaulish"}}},
    };
    // clang-format on

    std::unique_ptr<IRegistryRecordProvider> provider =
        std::make_unique<CMemoryRecordProvider>(std::move(param));

    EXPECT_TRUE(m_registry.Initialize(std::move(provider)));
  }

  static void TearDownTestSuite() { m_registry.Deinitialize(); }
};

// Define the static member
CSubTagRegistryManager Bcp47ValidateTagTester::m_registry;

TEST_P(Bcp47ValidateTagTester, ParseTag)
{
  auto& param = GetParam();
  auto actual = CBcp47::ParseTag(param.input, &m_registry);

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

TEST(TestI18nBcp47, RegistryDI)
{
  // System registry
  auto tag = CBcp47::ParseTag("en");
  EXPECT_TRUE(tag.has_value());
  EXPECT_TRUE(tag->IsValid());

  // Empty registry
  CSubTagRegistryManager registry;
  tag = CBcp47::ParseTag("en", &registry);
  EXPECT_TRUE(tag.has_value());
  EXPECT_FALSE(tag->IsValid());

  // registry with subtags that wouldn't be included in the official registry
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "language"}, {"Subtag", "zz"}, {"Description", "test"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryRecordProvider>(std::move(param));
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  tag = CBcp47::ParseTag("zz", &registry);
  EXPECT_TRUE(tag.has_value());
  EXPECT_TRUE(tag->IsValid());

  tag = CBcp47::ParseTag("zz");
  EXPECT_TRUE(tag.has_value());
  EXPECT_FALSE(tag->IsValid());
}
