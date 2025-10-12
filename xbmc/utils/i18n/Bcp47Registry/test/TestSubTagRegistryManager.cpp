/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/i18n/Bcp47Registry/RegistryRecordProvider.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryFile.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryManager.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

class CMemoryProvider : public IRegistryRecordProvider
{
public:
  CMemoryProvider(std::vector<RegistryFileRecord>& records) : m_records(records) {}
  CMemoryProvider(std::vector<RegistryFileRecord>&& records) : m_records(std::move(records)) {}

  bool Load() override { return !m_records.empty(); }
  const std::vector<RegistryFileRecord>& GetRecords() const override { return m_records; }

private:
  std::vector<RegistryFileRecord> m_records;
};

TEST(TestI18nRegistryManager, LanguageSubTag)
{
  // Minimal tags to test availability in registry
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "language"}, {"Subtag", "foo"}, {"Description", "bar"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetLanguageSubTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.GetLanguageSubTags().LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  registry.Deinitialize();
  l = registry.GetLanguageSubTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, ExtLangSubTag)
{
  // Minimal tags to test availability in registry
  std::vector<RegistryFileRecord> param = {RegistryFileRecord(
      {{"Type", "extlang"}, {"Subtag", "foo"}, {"Description", "bar"}, {"Prefix", "pref"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetExtLangSubTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.GetExtLangSubTags().LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  registry.Deinitialize();
  l = registry.GetExtLangSubTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, ScriptSubTag)
{
  // Minimal tags to test availability in registry
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "script"}, {"Subtag", "foo"}, {"Description", "bar"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetScriptSubTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.GetScriptSubTags().LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  registry.Deinitialize();
  l = registry.GetScriptSubTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, RegionSubTag)
{
  // Minimal tags to test availability in registry
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "region"}, {"Subtag", "foo"}, {"Description", "bar"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetRegionSubTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.GetRegionSubTags().LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  registry.Deinitialize();
  l = registry.GetRegionSubTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, VariantSubTag)
{
  // Minimal tags to test availability in registry
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "variant"}, {"Subtag", "foo"}, {"Description", "bar"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetVariantSubTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.GetVariantSubTags().LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  registry.Deinitialize();
  l = registry.GetVariantSubTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, GrandfatheredTag)
{
  // Minimal tags to test availability in registry
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "grandfathered"}, {"Subtag", "foo"}, {"Description", "bar"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetGrandfatheredTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.GetGrandfatheredTags().LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  registry.Deinitialize();
  l = registry.GetGrandfatheredTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, RedundantTag)
{
  // Minimal tags to test availability in registry
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord({{"Type", "redundant"}, {"Subtag", "foo"}, {"Description", "bar"}})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetRedundantTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  l = registry.GetRedundantTags().LookupByDescription("bar");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }

  registry.Deinitialize();
  l = registry.GetRedundantTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, InvalidSubTag1)
{
  std::vector<RegistryFileRecord> param = {RegistryFileRecord({})};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_FALSE(registry.Initialize(std::move(provider)));

  registry.Deinitialize();

  param = {RegistryFileRecord({{"Type", "language"}, {"Description", "bar"}})};

  provider = std::make_unique<CMemoryProvider>(std::move(param));
  EXPECT_FALSE(registry.Initialize(std::move(provider)));
}

TEST(TestI18nRegistryManager, InvalidSubTag2)
{
  // The Extlang subtag requires one Prefix field
  std::vector<RegistryFileRecord> param = {RegistryFileRecord({
      {"Type", "extlang"},
      {"Subtag", "foo"},
  })};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_FALSE(registry.Initialize(std::move(provider)));
  auto l = registry.GetExtLangSubTags().Lookup("foo");
  EXPECT_FALSE(l.has_value());
}

TEST(TestI18nRegistryManager, DuplicateSubTag)
{
  std::vector<RegistryFileRecord> param = {
      RegistryFileRecord{{{"Type", "language"}, {"Subtag", "foo"}, {"Description", "bar"}}},
      RegistryFileRecord{{{"Type", "language"}, {"Subtag", "foo"}, {"Description", "baz"}}}};

  std::unique_ptr<IRegistryRecordProvider> provider =
      std::make_unique<CMemoryProvider>(std::move(param));
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));

  auto l = registry.GetLanguageSubTags().Lookup("foo");
  EXPECT_TRUE(l.has_value());
  if (l.has_value())
  {
    EXPECT_EQ(l->m_subTag, "foo");
    EXPECT_EQ(l->m_descriptions, std::vector<std::string>{"bar"});
  }
}

TEST(TestI18nRegistryManager, LoadFile)
{
  const std::string filePath =
      XBMC_REF_FILE_PATH("xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry.txt");
  auto provider = std::make_unique<CRegistryFile>(filePath);
  CSubTagRegistryManager registry;
  EXPECT_TRUE(registry.Initialize(std::move(provider)));
  EXPECT_TRUE(registry.GetLanguageSubTags().Lookup("ab").has_value());
  EXPECT_TRUE(registry.GetExtLangSubTags().Lookup("aao").has_value());
  EXPECT_TRUE(registry.GetScriptSubTags().Lookup("adlm").has_value());
  EXPECT_TRUE(registry.GetRegionSubTags().Lookup("ac").has_value());
  EXPECT_TRUE(registry.GetVariantSubTags().Lookup("1994").has_value());
  EXPECT_TRUE(registry.GetGrandfatheredTags().Lookup("art-lojban").has_value());
  EXPECT_TRUE(registry.GetRedundantTags().Lookup("az-arab").has_value());
}
