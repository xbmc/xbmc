/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/i18n/RegistryRecordProvider.h"
#include "utils/i18n/SubTagLoader.h"
#include "utils/i18n/SubTagRegistryTypes.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

TEST(TestI18nSubTagLoader, BaseSubTag)
{
  RegistryFileRecord param({{"Type", "script"},
                            {"Subtag", "foo"},
                            {"Description", "bar"},
                            {"Added", "2025-11-01"},
                            {"Deprecated", "2025-11-02"},
                            {"Preferred-Value", "baz"}});

  BaseSubTag expected;
  expected.m_subTag = "foo";
  expected.m_description = std::vector<std::string>{"bar"};
  expected.m_added = "2025-11-01";
  expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "baz";

  ScriptSubTag actual;
  EXPECT_TRUE(LoadBaseSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  expected.m_subTag = "foo";
  expected.m_description = {};
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
  expected.m_description = {{"bar"}, {"baz"}};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};

  EXPECT_TRUE(LoadBaseSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  param = RegistryFileRecord({{"Type", "language"}, {"Not Subtag", "foo"}});
  EXPECT_FALSE(LoadBaseSubTag(actual, param));

  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", ""}});
  EXPECT_FALSE(LoadBaseSubTag(actual, param));

  param = RegistryFileRecord({{"Type", "language"}, {"SubTag", "foo"}});
  EXPECT_FALSE(LoadBaseSubTag(actual, param));
}

TEST(TestI18nSubTagLoader, LanguageSubTag)
{
  RegistryFileRecord param({{"Type", "language"},
                            {"Subtag", "foo"},
                            {"Description", "bar"},
                            {"Added", "2025-11-01"},
                            {"Deprecated", "2025-11-02"},
                            {"Preferred-Value", "baz"},
                            {"Suppress-Script", "suppr"},
                            {"Macrolanguage", "macro"},
                            {"Scope", "collection"}});

  LanguageSubTag expected;
  expected.m_subTag = "foo";
  expected.m_description = std::vector<std::string>{"bar"};
  expected.m_added = "2025-11-01", expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "baz";
  expected.m_suppressScript = "suppr";
  expected.m_macroLanguage = "macro";
  expected.m_scope = SubTagScope::Collection;

  LanguageSubTag actual;
  EXPECT_TRUE(LoadLanguageSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  expected.m_subTag = "foo";
  expected.m_description = {};
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
  RegistryFileRecord param({{"Type", "extlang"},
                            {"Subtag", "foo"},
                            {"Description", "bar"},
                            {"Added", "2025-11-01"},
                            {"Deprecated", "2025-11-02"},
                            {"Preferred-Value", "baz"},
                            {"Prefix", "pref"},
                            {"Suppress-Script", "suppr"},
                            {"Macrolanguage", "macro"},
                            {"Scope", "collection"}});

  ExtLangSubTag expected;
  expected.m_subTag = "foo";
  expected.m_description = std::vector<std::string>{"bar"};
  expected.m_added = "2025-11-01", expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "baz";
  expected.m_prefix = "pref";
  expected.m_suppressScript = "suppr";
  expected.m_macroLanguage = "macro";
  expected.m_scope = SubTagScope::Collection;

  ExtLangSubTag actual;
  EXPECT_TRUE(LoadExtLangSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}, {"Prefix", "pref"}});
  expected.m_subTag = "foo";
  expected.m_description = {};
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
      {{"Type", "language"}, {"Subtag", "foo"}, {"Prefix", "pref"}, {"Scope", "not a scope"}});
  expected.m_scope = SubTagScope::Unknown;

  EXPECT_TRUE(LoadExtLangSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // No prefixes
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  EXPECT_FALSE(LoadExtLangSubTag(actual, param));

  // Multiple prefixes
  param = RegistryFileRecord({
      {"Type", "language"},
      {"Subtag", "foo"},
      {"Prefix", "pref1"},
      {"Prefix", "pref2"},
  });
  EXPECT_FALSE(LoadExtLangSubTag(actual, param));
}

TEST(TestI18nSubTagLoader, DerivedBaseSubTag)
{
  // Testing one derived class is enough, they would all behave the same.
  RegistryFileRecord param({{"Type", "script"},
                            {"Subtag", "foo"},
                            {"Description", "bar"},
                            {"Added", "2025-11-01"},
                            {"Deprecated", "2025-11-02"},
                            {"Preferred-Value", "baz"}});

  ScriptSubTag expected;
  expected.m_subTag = "foo";
  expected.m_description = std::vector<std::string>{"bar"};
  expected.m_added = "2025-11-01";
  expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "baz";

  ScriptSubTag actual;
  EXPECT_TRUE(LoadBaseSubTag(actual, param));
  EXPECT_EQ(expected, actual);
}

TEST(TestI18nSubTagLoader, VariantSubTag)
{
  RegistryFileRecord param({{"Type", "variant"},
                            {"Subtag", "foo"},
                            {"Description", "bar"},
                            {"Added", "2025-11-01"},
                            {"Deprecated", "2025-11-02"},
                            {"Preferred-Value", "baz"},
                            {"Prefix", "pref"}});

  VariantSubTag expected;
  expected.m_subTag = "foo";
  expected.m_description = std::vector<std::string>{"bar"};
  expected.m_added = "2025-11-01", expected.m_deprecated = "2025-11-02";
  expected.m_preferredValue = "baz";
  expected.m_prefix = std::vector<std::string>{"pref"};

  VariantSubTag actual;
  EXPECT_TRUE(LoadVariantSubTag(actual, param));
  EXPECT_EQ(expected, actual);

  // Minimal tag
  param = RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}});
  expected.m_subTag = "foo";
  expected.m_description = {};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};
  expected.m_prefix = {};

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
  expected.m_description = {};
  expected.m_added = {};
  expected.m_deprecated = {};
  expected.m_preferredValue = {};
  expected.m_prefix = {{"pref1"}, {"pref2"}};

  EXPECT_TRUE(LoadVariantSubTag(actual, param));
  EXPECT_EQ(expected, actual);
}
