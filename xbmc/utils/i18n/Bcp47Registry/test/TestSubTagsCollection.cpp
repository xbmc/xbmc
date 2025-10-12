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
#include "utils/i18n/Bcp47Registry/SubTagsCollection.h"

#include <array>
#include <memory>

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

template<class T>
class CTestSubTagsCollection : public CSubTagsCollection<T>
{
public:
  bool CallEmplaceRecords(const std::vector<RegistryFileRecord>& records)
  {
    return this->EmplaceRecords(records);
  }
  void CallReset() { return this->Reset(); }
  std::size_t CallSize() { return this->Size(); }
  std::size_t CallSizeDesc() { return this->SizeDesc(); }
};

struct DescriptionSplitTest
{
  std::vector<RegistryFileRecord> m_record;
  std::vector<std::string> m_positiveDescriptions;
  std::string m_positiveSubTag;
  std::string m_positiveDescription;
  std::vector<std::string> m_negativeDescriptions;
};

const auto GrandfatheredSplitTests = std::array{
    DescriptionSplitTest{{{{{"Type", "grandfathered"}, {"Subtag", "foo"}, {"Description", "bar"}}}},
                         {"bar"},
                         "foo",
                         "bar",
                         {}},
    DescriptionSplitTest{
        {{{{"Type", "grandfathered"}, {"Subtag", "foo"}, {"Description", "bar or baz"}}}},
        {"bar", "baz"},
        "foo",
        "bar or baz",
        {"bar or baz"}},
    DescriptionSplitTest{
        {{{{"Type", "grandfathered"}, {"Subtag", "foo"}, {"Description", "bar, baz, foobar"}}}},
        {"bar", "baz", "foobar"},
        "foo",
        "bar, baz, foobar",
        {"bar, baz, foobar"}},
    DescriptionSplitTest{
        {{{{"Type", "grandfathered"}, {"Subtag", "foo"}, {"Description", "bar, baz, or foobar"}}}},
        {"bar", "baz", "foobar"},
        "foo",
        "bar, baz, or foobar",
        {"bar, baz, or foobar"}},
    DescriptionSplitTest{
        {{{{"Type", "grandfathered"}, {"Subtag", "foo"}, {"Description", "bar, baz"}}}},
        {"bar, baz"},
        "foo",
        "bar, baz",
        {"bar", "baz"}},
};

class DescGrandfatheredTests : public testing::WithParamInterface<DescriptionSplitTest>,
                               public testing::Test
{
};

TEST_P(DescGrandfatheredTests, TestValue)
{
  auto param = GetParam().m_record;
  CTestSubTagsCollection<GrandfatheredTag> registry;
  EXPECT_TRUE(registry.CallEmplaceRecords(param));

  for (const auto& desc : GetParam().m_positiveDescriptions)
  {
    auto l = registry.LookupByDescription(desc);
    EXPECT_TRUE(l.has_value());
    if (l.has_value())
    {
      EXPECT_EQ(l->m_subTag, GetParam().m_positiveSubTag);
      EXPECT_EQ(l->m_descriptions, std::vector<std::string>{GetParam().m_positiveDescription});
    }
  }

  for (const auto& desc : GetParam().m_negativeDescriptions)
  {
    auto l = registry.LookupByDescription(desc);
    EXPECT_FALSE(l.has_value());
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nSubTagsCollection,
                         DescGrandfatheredTests,
                         testing::ValuesIn(GrandfatheredSplitTests));

const auto NegativeSplitTests = std::array{
    DescriptionSplitTest{{{{{"Type", "language"}, {"Subtag", "foo"}, {"Description", "bar"}}}},
                         {"bar"},
                         "foo",
                         "bar",
                         {}},
    DescriptionSplitTest{
        {{{{"Type", "language"}, {"Subtag", "foo"}, {"Description", "bar or baz"}}}},
        {"bar or baz"},
        "foo",
        "bar or baz",
        {"bar", "baz"}},
    DescriptionSplitTest{
        {{{{"Type", "language"}, {"Subtag", "foo"}, {"Description", "bar, baz, foobar"}}}},
        {"bar, baz, foobar"},
        "foo",
        "bar, baz, foobar",
        {"bar", "baz", "foobar"}},
    DescriptionSplitTest{
        {{{{"Type", "language"}, {"Subtag", "foo"}, {"Description", "bar, baz, or foobar"}}}},
        {"bar, baz, or foobar"},
        "foo",
        "bar, baz, or foobar",
        {"bar", "baz", "foobar"}}};

class DescNegativeTests : public testing::WithParamInterface<DescriptionSplitTest>,
                          public testing::Test
{
};

TEST_P(DescNegativeTests, TestValue)
{
  auto param = GetParam().m_record;
  CTestSubTagsCollection<LanguageSubTag> registry;
  EXPECT_TRUE(registry.CallEmplaceRecords(param));

  for (const auto& desc : GetParam().m_positiveDescriptions)
  {
    auto l = registry.LookupByDescription(desc);
    EXPECT_TRUE(l.has_value());
    if (l.has_value())
    {
      EXPECT_EQ(l->m_subTag, GetParam().m_positiveSubTag);
      EXPECT_EQ(l->m_descriptions, std::vector<std::string>{GetParam().m_positiveDescription});
    }
  }

  for (const auto& desc : GetParam().m_negativeDescriptions)
  {
    auto l = registry.LookupByDescription(desc);
    EXPECT_FALSE(l.has_value());
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nSubTagsCollection,
                         DescNegativeTests,
                         testing::ValuesIn(NegativeSplitTests));

// All subtags behave the same for base attributes, group the tests
using RegistryTypes = ::testing::Types<LanguageSubTag,
                                       ExtLangSubTag,
                                       ScriptSubTag,
                                       RegionSubTag,
                                       VariantSubTag,
                                       GrandfatheredTag,
                                       RedundantTag>;
TYPED_TEST_SUITE(TestI18nSubTagsCollectionTyped, RegistryTypes);

template<typename T>
class TestI18nSubTagsCollectionTyped : public testing::Test
{
};

TYPED_TEST(TestI18nSubTagsCollectionTyped, EmplaceRecords)
{
  std::vector<RegistryFileRecord> param = {RegistryFileRecord({
      {"Type", "language"},
      {"Subtag", "foo"},
      {"Description", "bar"},
      {"Prefix", "pref"},
  })};
  // Prefix field included to satisfy the constraint of ExtLang subtags.

  CTestSubTagsCollection<TypeParam> registry;
  EXPECT_TRUE(registry.CallEmplaceRecords(param));

  auto l = registry.Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }
}

TEST(TestI18nSubTagsCollection, Reset)
{
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}, {"Description", "bar"}})};

  CTestSubTagsCollection<LanguageSubTag> registry;
  EXPECT_TRUE(registry.CallEmplaceRecords(param));

  registry.CallReset();

  EXPECT_FALSE(registry.Lookup("foo").has_value());
  EXPECT_FALSE(registry.LookupByDescription("bar").has_value());
  EXPECT_EQ(0, registry.CallSize());
  EXPECT_EQ(0, registry.CallSizeDesc());
}
