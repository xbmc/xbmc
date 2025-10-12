/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/i18n/Bcp47Registry/RegistryRecordProvider.h"
#include "utils/i18n/Bcp47Registry/SubTagLoader.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

TEST(TestI18nSubTagLoader, BaseSubTag)
{
  RegistryFileRecord param({
      {"Type", "language"},
      {"Subtag", "FoO"},
      {"Description", "BaR"},
      {"Added", "2025-11-01"},
      {"Deprecated", "2025-11-02"},
      {"Preferred-Value", "bAz"},
  });

  BaseSubTag expected;
  expected.m_subTag = "FoO";
  expected.m_descriptions = std::vector<std::string>{"BaR"};
  expected.m_added = "2025-11-01";
  expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "bAz";

  ScriptSubTag actual;
  EXPECT_TRUE(LoadBaseSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  expected.m_subTag = "foo";
  expected.m_descriptions = {};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};

  EXPECT_TRUE(LoadBaseSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Multiple descriptions
  param = RegistryFileRecord({
      {"Type", "language"},
      {"Subtag", "foo"},
      {"Description", "bar"},
      {"Description", "baz"},
  });
  expected.m_subTag = "foo";
  expected.m_descriptions = {{"bar"}, {"baz"}};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};

  EXPECT_TRUE(LoadBaseSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Missing information
  param = RegistryFileRecord({{"Type", "language"}, {"Not Subtag", "foo"}});
  EXPECT_FALSE(LoadBaseSubTag(actual, param));

  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", ""}});
  EXPECT_FALSE(LoadBaseSubTag(actual, param));

  param = RegistryFileRecord({{"Type", "language"}, {"SubTag", "foo"}});
  EXPECT_FALSE(LoadBaseSubTag(actual, param));
}

TEST(TestI18nSubTagLoader, LanguageSubTag)
{
  RegistryFileRecord param({
      {"Type", "language"},
      {"Subtag", "FoO"},
      {"Description", "BaR"},
      {"Added", "2025-11-01"},
      {"Deprecated", "2025-11-02"},
      {"Preferred-Value", "bAz"},
      {"Suppress-Script", "SuPpR"},
      {"Macrolanguage", "mAcRo"},
      {"Scope", "collection"},
  });

  LanguageSubTag expected;
  expected.m_subTag = "FoO";
  expected.m_descriptions = std::vector<std::string>{"BaR"};
  expected.m_added = "2025-11-01";
  expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "bAz";
  expected.m_suppressScript = "SuPpR";
  expected.m_macroLanguage = "mAcRo";
  expected.m_scope = SubTagScope::Collection;

  LanguageSubTag actual;
  EXPECT_TRUE(LoadLanguageSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  expected.m_subTag = "foo";
  expected.m_descriptions = {};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};
  expected.m_suppressScript = {};
  expected.m_macroLanguage = {};
  expected.m_scope = SubTagScope::Individual;

  EXPECT_TRUE(LoadLanguageSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Unknown scope
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}, {"Scope", "not a scope"}});
  expected.m_scope = SubTagScope::Unknown;

  EXPECT_TRUE(LoadLanguageSubTag(actual, param));
  EXPECT_EQ(expected, actual);
}

TEST(TestI18nSubTagLoader, ExtLangSubTag)
{
  RegistryFileRecord param({
      {"Type", "extlang"},
      {"Subtag", "FoO"},
      {"Description", "BaR"},
      {"Added", "2025-11-01"},
      {"Deprecated", "2025-11-02"},
      {"Preferred-Value", "bAz"},
      {"Prefix", "PrEf"},
      {"Suppress-Script", "SuPpR"},
      {"Macrolanguage", "mAcRo"},
      {"Scope", "collection"},
  });

  ExtLangSubTag expected;
  expected.m_subTag = "FoO";
  expected.m_descriptions = std::vector<std::string>{"BaR"};
  expected.m_added = "2025-11-01";
  expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "bAz";
  expected.m_prefix = "PrEf";
  expected.m_suppressScript = "SuPpR";
  expected.m_macroLanguage = "mAcRo";
  expected.m_scope = SubTagScope::Collection;

  ExtLangSubTag actual;
  EXPECT_TRUE(LoadExtLangSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "extlang"}, {"Subtag", "foo"}, {"Prefix", "pref"}});
  expected.m_subTag = "foo";
  expected.m_descriptions = {};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};
  expected.m_prefix = "pref";
  expected.m_suppressScript = {};
  expected.m_macroLanguage = {};
  expected.m_scope = SubTagScope::Individual;

  EXPECT_TRUE(LoadExtLangSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Unknown scope
  param = RegistryFileRecord(
      {{"Type", "extlang"}, {"Subtag", "foo"}, {"Prefix", "pref"}, {"Scope", "not a scope"}});
  expected.m_scope = SubTagScope::Unknown;

  EXPECT_TRUE(LoadExtLangSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // No prefixes
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  EXPECT_FALSE(LoadExtLangSubTag(actual, param));

  // Multiple prefixes
  param = RegistryFileRecord({
      {"Type", "extlang"},
      {"Subtag", "foo"},
      {"Prefix", "pref1"},
      {"Prefix", "pref2"},
  });
  EXPECT_FALSE(LoadExtLangSubTag(actual, param));
}

TEST(TestI18nSubTagLoader, DerivedBaseSubTag)
{
  RegistryFileRecord param({
      {"Type", "script"},
      {"Subtag", "FoO"},
      {"Description", "bAr"},
      {"Added", "2025-11-01"},
      {"Deprecated", "2025-11-02"},
      {"Preferred-Value", "BaZ"},
  });

  ScriptSubTag expected;
  expected.m_subTag = "FoO";
  expected.m_descriptions = std::vector<std::string>{"bAr"};
  expected.m_added = "2025-11-01";
  expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "BaZ";

  ScriptSubTag actual;
  EXPECT_TRUE(LoadBaseSubTag(actual, param));
  EXPECT_EQ(expected, actual);
}

TEST(TestI18nSubTagLoader, VariantSubTag)
{
  RegistryFileRecord param({
      {"Type", "variant"},
      {"Subtag", "FoO"},
      {"Description", "bAr"},
      {"Added", "2025-11-01"},
      {"Deprecated", "2025-11-02"},
      {"Preferred-Value", "BaZ"},
      {"Prefix", "PrEf"},
  });

  VariantSubTag expected;
  expected.m_subTag = "FoO";
  expected.m_descriptions = std::vector<std::string>{"bAr"};
  expected.m_added = "2025-11-01";
  expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "BaZ";
  expected.m_prefixes = std::vector<std::string>{"PrEf"};

  VariantSubTag actual;
  EXPECT_TRUE(LoadVariantSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  expected.m_subTag = "foo";
  expected.m_descriptions = {};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};
  expected.m_prefixes = {};

  EXPECT_TRUE(LoadVariantSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Multiple descriptions
  param = RegistryFileRecord({
      {"Type", "language"},
      {"Subtag", "foo"},
      {"Prefix", "pref1"},
      {"Prefix", "pref2"},
  });
  expected.m_subTag = "foo";
  expected.m_descriptions = {};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};
  expected.m_prefixes = {{"pref1"}, {"pref2"}};

  EXPECT_TRUE(LoadVariantSubTag(actual, param));
  EXPECT_EQ(expected, actual);
}
