/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/i18n/Bcp47Registry/RegistryRecordProvider.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"

#include <array>

#include <fmt/format.h>
#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

struct SubTagScopeTest
{
  SubTagScope m_scope;
  std::string_view m_code;
  std::string_view m_description;
};

constexpr auto SubTagScopeTests = std::array{
    SubTagScopeTest{SubTagScope::Individual, "", "individual"},
    SubTagScopeTest{SubTagScope::MacroLanguage, "macrolanguage", "macro language"},
    SubTagScopeTest{SubTagScope::Collection, "collection", "collection"},
    SubTagScopeTest{SubTagScope::Special, "special", "special"},
    SubTagScopeTest{SubTagScope::PrivateUse, "private-use", "private use"},
    SubTagScopeTest{SubTagScope::Unknown, "foo", "unknown"},
};

class ScopeValues : public testing::WithParamInterface<SubTagScopeTest>, public testing::Test
{
};

TEST_P(ScopeValues, StringToScope)
{
  EXPECT_EQ(GetParam().m_scope, FindSubTagScope(GetParam().m_code));
}

TEST_P(ScopeValues, ScopeToString)
{
  EXPECT_EQ(GetParam().m_description, fmt::format("{}", GetParam().m_scope));
}

INSTANTIATE_TEST_SUITE_P(TestI18nRegistryTypes, ScopeValues, testing::ValuesIn(SubTagScopeTests));

struct SubTagTypeTest
{
  SubTagType m_type;
  std::string_view m_code;
  std::string_view m_description = "";
};

constexpr auto SubTagTypeTests = std::array{
    SubTagTypeTest{SubTagType::Language, "language"},
    SubTagTypeTest{SubTagType::ExtLang, "extlang"},
    SubTagTypeTest{SubTagType::Script, "script"},
    SubTagTypeTest{SubTagType::Region, "region"},
    SubTagTypeTest{SubTagType::Variant, "variant"},
    SubTagTypeTest{SubTagType::Grandfathered, "grandfathered"},
    SubTagTypeTest{SubTagType::Redundant, "redundant"},
    SubTagTypeTest{SubTagType::Unknown, "foo", "unknown"},
};

class TypeValues : public testing::WithParamInterface<SubTagTypeTest>, public testing::Test
{
};

TEST_P(TypeValues, StringToType)
{
  EXPECT_EQ(GetParam().m_type, FindSubTagType(GetParam().m_code));
}

TEST_P(TypeValues, TypeToString)
{
  std::string_view description =
      GetParam().m_description.empty() ? GetParam().m_code : GetParam().m_description;
  EXPECT_EQ(description, fmt::format("{}", GetParam().m_type));
}

INSTANTIATE_TEST_SUITE_P(TestI18nRegistryTypes, TypeValues, testing::ValuesIn(SubTagTypeTests));

// Subtag types without additional attributes behave identically - group their tests
using RegistryTypes = ::testing::Types<ScriptSubTag, RegionSubTag, GrandfatheredTag, RedundantTag>;
TYPED_TEST_SUITE(TestI18nRegistryTypesTyped, RegistryTypes);

template<typename T>
class TestI18nRegistryTypesTyped : public testing::Test
{
};

TYPED_TEST(TestI18nRegistryTypesTyped, Load)
{
  RegistryFileRecord param{{
      {"Type", "redundant"},
      {"Subtag", "foo"},
  }};

  BaseSubTag expected;
  expected.m_subTag = "foo";

  TypeParam actual;
  EXPECT_TRUE(actual.Load(param));
  EXPECT_EQ(expected, actual);
}

// Subtag types with additional attributes
TEST(TestI18nRegistryTypes, LoadLanguageSubTag)
{
  RegistryFileRecord param{{
      {"Type", "language"},
      {"Subtag", "foo"},
      {"Suppress-Script", "scr"},
  }};

  LanguageSubTag expected;
  expected.m_subTag = "foo";
  expected.m_suppressScript = "scr";

  LanguageSubTag actual;
  EXPECT_TRUE(actual.Load(param));
  EXPECT_EQ(expected, actual);
}

TEST(TestI18nRegistryTypes, LoadExtLangSubTag)
{
  RegistryFileRecord param{{
      {"Type", "extlang"},
      {"Subtag", "foo"},
      {"Prefix", "prefix"},
  }};

  ExtLangSubTag expected;
  expected.m_subTag = "foo";
  expected.m_prefix = "prefix";

  ExtLangSubTag actual;
  EXPECT_TRUE(actual.Load(param));
  EXPECT_EQ(expected, actual);
}

TEST(TestI18nRegistryTypes, LoadVariantSubTag)
{
  RegistryFileRecord param{{
      {"Type", "variant"},
      {"Subtag", "foo"},
      {"Prefix", "prefix1"},
      {"Prefix", "prefix2"},
  }};

  VariantSubTag expected;
  expected.m_subTag = "foo";
  expected.m_prefixes = {"prefix1", "prefix2"};

  VariantSubTag actual;
  EXPECT_TRUE(actual.Load(param));
  EXPECT_EQ(expected, actual);
}

// Bad data should trip the validation
TEST(TestI18nRegistryTypes, LoadFailure)
{
  RegistryFileRecord param{{{"Type", "language"}}};

  // Error for missing subtag name expected
  LanguageSubTag actual;
  param = RegistryFileRecord{{{"Type", "language"}}};

  EXPECT_FALSE(actual.Load(param));
}
