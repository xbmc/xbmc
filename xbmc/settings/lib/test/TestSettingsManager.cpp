/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

struct SettingIdentifierParseTest
{
  std::string_view settingId;
  std::string_view category;
  std::string_view setting;
};

std::ostream& operator<<(std::ostream& os, const SettingIdentifierParseTest& rhs)
{
  return os << rhs.settingId;
}

class TestParseSettingIdentifier : public testing::WithParamInterface<SettingIdentifierParseTest>,
                                   public testing::Test
{
};

SettingIdentifierParseTest SettingIdentifierParseTests[] = {
    {"categorytag.settingtag", "categorytag", "settingtag"},
    {"categorytag.settingtag.subsetting", "categorytag", "settingtag.subsetting"},
    {"test", "", "test"},
};

TEST_P(TestParseSettingIdentifier, Parse)
{
  const auto& params = GetParam();

  std::string categoryTag;
  std::string settingTag;
  EXPECT_TRUE(CSettingsManager::ParseSettingIdentifier(params.settingId, categoryTag, settingTag));
  EXPECT_EQ(params.category, categoryTag);
  EXPECT_EQ(params.setting, settingTag);
}

INSTANTIATE_TEST_SUITE_P(TestSettingsManager,
                         TestParseSettingIdentifier,
                         testing::ValuesIn(SettingIdentifierParseTests));

TEST(TestSettingsManager, ParseInvalidSettingIdentifier)
{
  std::string categoryTag;
  std::string settingTag;
  EXPECT_FALSE(CSettingsManager::ParseSettingIdentifier("", categoryTag, settingTag));
  EXPECT_FALSE(CSettingsManager::ParseSettingIdentifier(".test", categoryTag, settingTag));
}

struct LocateSettingTest
{
  std::string_view m_document;
  std::string_view m_settingId;
  // nullopt means we expect the element to not be found
  std::optional<std::string_view> m_expectedSerialization;
};

class TestLocateSetting : public testing::WithParamInterface<LocateSettingTest>,
                          public testing::Test
{
};

LocateSettingTest LocateSettingTests[] = {
    // V1 format
    {
        R"(<settings><foo><bar>value</bar></foo></settings>)",
        "foo.bar",
        R"(<bar>value</bar>)",
    },
    {
        R"(<settings><foo><bar>value</bar></foo></settings>)",
        "foo",
        R"(<foo><bar>value</bar></foo>)",
    },
    {
        R"(<settings><foo><bar.baz>value</bar.baz></foo></settings>)",
        "foo.bar.baz",
        R"(<bar.baz>value</bar.baz>)",
    },
    {
        R"(<settings><foo><bar></bar></foo></settings>)",
        "foo.bar",
        R"(<bar />)",
    },
    {
        R"(<settings><foo><bar /></foo></settings>)",
        "foo.bar",
        R"(<bar />)",
    },
    {
        R"(<settings><foo><baz>value</baz></foo></settings>)",
        "foo.bar",
        std::nullopt,
    },
    {
        R"(<settings><foo></foo></settings>)",
        "foo.bar",
        std::nullopt,
    },
    {
        R"(<settings><foo /></settings>)",
        "foo.bar",
        std::nullopt,
    },

    // V2 format
    {
        R"(<settings version="2"><setting id="foo.bar">value</setting></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar">value</setting>)",
    },
    {
        R"(<settings version="2"><setting id="foo.bar"></setting></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar" />)",
    },
    {
        R"(<settings version="2"><setting id="foo.bar" /></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar" />)",
    },
    {
        R"(<settings version="2"><setting id="foo.bar"></setting><setting id="foo.baz"></setting></settings>)",
        "foo.baz",
        R"(<setting id="foo.baz" />)",
    },
    {
        R"(<settings version="2"><setting ab="foo.bar">value</setting></settings>)",
        "foo.bar",
        std::nullopt,
    },

    // Mixed V1+V2: V2 takes priority regardless of document order
    {
        R"(<settings version="2"><setting id="foo.bar">v2</setting><foo><bar>v1</bar></foo></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar">v2</setting>)",
    },
    {
        R"(<settings version="2"><foo><bar>v1</bar></foo><setting id="foo.bar">v2</setting></settings>)",
        "foo.bar",
        R"(<setting id="foo.bar">v2</setting>)",
    },
};

TEST_P(TestLocateSetting, Locate)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string{params.m_document}));

  TiXmlElement* elem = CSettingsManager::LocateSetting(doc.RootElement(), params.m_settingId);

  if (params.m_expectedSerialization.has_value())
  {
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(params.m_expectedSerialization.value(),
              XMLUtils::NodeStringSerialization(elem, XMLUtils::SerializationFormat::COMPACT));
  }
  else
  {
    EXPECT_EQ(nullptr, elem);
  }
}

INSTANTIATE_TEST_SUITE_P(TestSettingsManager,
                         TestLocateSetting,
                         testing::ValuesIn(LocateSettingTests));

TEST(TestSettingsManager, LocateSettingNull)
{
  TiXmlElement* root = nullptr;
  TiXmlElement* elem = CSettingsManager::LocateSetting(root, "foo.bar");
  EXPECT_EQ(nullptr, elem);

  const std::string document =
      R"(<settings version="2">
           <setting id="foo.bar">v2</setting>
         </settings>)";

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(document));

  elem = CSettingsManager::LocateSetting(doc.RootElement(), "");
  EXPECT_EQ(nullptr, elem);
}
