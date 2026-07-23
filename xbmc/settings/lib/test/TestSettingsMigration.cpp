/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/SettingsMigration.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <string_view>

#include <gtest/gtest.h>

class TestConversions : public ::testing::Test
{
public:
  enum class TestConversionResult
  {
    NOT_PRESENT,
    INVALID,
    CONVERTED,
    ALREADY_EXISTS,
    UNKNOWN, // extra value for test
  };

  struct TestSettingBoolToIntMapping
  {
    int m_default;
    int m_false;
    int m_true;
  };

  // All tests will attempt this conversion
  std::string_view m_oldSettingId = "a.b";
  std::string_view m_newSettingId = "c.d";

protected:
  // Proxy enum/functions/accessors as the fixture functions are not part of the class and don't have
  // access to inherited private members.

  CSettingsMigration::SettingBoolToIntMapping ConvertMapping(
      const TestSettingBoolToIntMapping& mapping)
  {
    return {.m_default = mapping.m_default, .m_false = mapping.m_false, .m_true = mapping.m_true};
  }

  TestConversionResult ConvertResult(CSettingsMigration::SettingConversionResult result)
  {
    if (result == CSettingsMigration::SettingConversionResult::NOT_PRESENT)
      return TestConversionResult::NOT_PRESENT;
    if (result == CSettingsMigration::SettingConversionResult::INVALID)
      return TestConversionResult::INVALID;
    if (result == CSettingsMigration::SettingConversionResult::CONVERTED)
      return TestConversionResult::CONVERTED;
    if (result == CSettingsMigration::SettingConversionResult::ALREADY_EXISTS)
      return TestConversionResult::ALREADY_EXISTS;
    return TestConversionResult::UNKNOWN;
  }

  TestConversionResult CallConvertSettingBoolToInt(TiXmlElement* root,
                                                   std::string_view oldId,
                                                   std::string_view newId,
                                                   const TestSettingBoolToIntMapping& mapping)
  {
    CSettingsMigration::SettingConversionResult result =
        CSettingsMigration::ConvertSettingBoolToInt(root, oldId, newId, ConvertMapping(mapping));

    return ConvertResult(result);
  }
};

struct ConvertSettingBoolToIntTest
{
  std::string m_originalSettings;
  TestConversions::TestConversionResult m_result;
  std::string_view m_serializedOutput;
};

class TestConvertSettingBoolToInt : public TestConversions,
                                    public testing::WithParamInterface<ConvertSettingBoolToIntTest>
{
};

ConvertSettingBoolToIntTest ConvertSettingBoolToIntTests[] = {
    // Successful conversions
    // V2 format
    // Convert a true value
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     TestConversions::TestConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     TestConversions::TestConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
    // Convert one of multiple settings
    {R"(<settings version="2">
          <setting id="a.b">false</setting>
          <setting id="foo.bar">test</setting>
        </settings>)",
     TestConversions::TestConversionResult::CONVERTED,
     R"(<settings version="2">)"
     R"(<setting id="foo.bar">test</setting>)"
     R"(<setting id="c.d" default="true">1</setting>)"
     R"(</settings>)"},
    // V1 format
    // Convert a true value
    {R"(<settings><a><b>true</b></a></settings>)", TestConversions::TestConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings><a><b>false</b></a></settings>)",
     TestConversions::TestConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d" default="true">1</setting></settings>)"},
    // Convert one of multiple settings
    {R"(<settings version="2">
          <a><b>false</b></a>
          <foo><bar>test</bar></foo>
        </settings>)",
     TestConversions::TestConversionResult::CONVERTED,
     R"(<settings version="2">)"
     R"(<a />)"
     R"(<foo><bar>test</bar></foo>)"
     R"(<setting id="c.d" default="true">1</setting>)"
     R"(</settings>)"},

    // Invalid old setting values
    // V2 format
    {R"(<settings version="2"><setting id="a.b">notbool</setting></settings>)",
     TestConversions::TestConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b">notbool</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"> true</setting></settings>)",
     TestConversions::TestConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b"> true</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"></setting></settings>)",
     TestConversions::TestConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b" /></settings>)"},
    {R"(<settings version="2"><setting id="a.b" /></settings>)",
     TestConversions::TestConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b" /></settings>)"},
    // V1 format
    {R"(<settings><a><b>notbool</b></a></settings>)",
     TestConversions::TestConversionResult::INVALID,
     R"(<settings><a><b>notbool</b></a></settings>)"},
    {R"(<settings><a><b></b></a></settings>)", TestConversions::TestConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},
    {R"(<settings><a><b /></a></settings>)", TestConversions::TestConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},

    // Old setting is not present
    // V2 format
    {R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)",
     TestConversions::TestConversionResult::NOT_PRESENT,
     R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)"},
    {R"(<settings version="2"></settings>)", TestConversions::TestConversionResult::NOT_PRESENT,
     R"(<settings version="2" />)"},
    {R"(<settings version="2" />)", TestConversions::TestConversionResult::NOT_PRESENT,
     R"(<settings version="2" />)"},
    // V1 format
    {R"(<settings><foo><bar>true</bar></foo></settings>)",
     TestConversions::TestConversionResult::NOT_PRESENT,
     R"(<settings><foo><bar>true</bar></foo></settings>)"},
    {R"(<settings><a><bar>true</bar></a></settings>)",
     TestConversions::TestConversionResult::NOT_PRESENT,
     R"(<settings><a><bar>true</bar></a></settings>)"},
    // the other V1 format tests are identical to the V2, except for the version attribute
    // which is not actively used by code

    // New setting is already there
    // V2 format
    {R"(<settings version="2"><setting id="c.d">true</setting></settings>)",
     TestConversions::TestConversionResult::ALREADY_EXISTS,
     R"(<settings version="2"><setting id="c.d">true</setting></settings>)"},
    // V1 format
    {R"(<settings><c><d>notbool</d></c></settings>)",
     TestConversions::TestConversionResult::ALREADY_EXISTS,
     R"(<settings><c><d>notbool</d></c></settings>)"},
};

TEST_P(TestConvertSettingBoolToInt, Convert)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(params.m_originalSettings));

  auto conversionResult =
      CallConvertSettingBoolToInt(doc.RootElement(), m_oldSettingId, m_newSettingId,
                                  {.m_default = 1, .m_false = 1, .m_true = 2});
  ASSERT_EQ(params.m_result, conversionResult);

  EXPECT_EQ(
      params.m_serializedOutput,
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestConvertSettingBoolToInt,
                         testing::ValuesIn(ConvertSettingBoolToIntTests));

struct ConvertSettingBoolToIntMappingTest
{
  std::string m_originalSettings;
  TestConversions::TestSettingBoolToIntMapping m_mapping;
  std::string_view m_serializedOutput;
};

class TestConvertSettingBoolToIntMapping
  : public TestConversions,
    public testing::WithParamInterface<ConvertSettingBoolToIntMappingTest>
{
};

ConvertSettingBoolToIntMappingTest ConvertSettingBoolToIntMappingTests[] = {
    // True value
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 1, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 2, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d" default="true">2</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 1, .m_false = 2, .m_true = 2},
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 2, .m_false = 2, .m_true = 2},
     R"(<settings version="2"><setting id="c.d" default="true">2</setting></settings>)"},

    // False value
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 1, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 2, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 2, .m_false = 1, .m_true = 1},
     R"(<settings version="2"><setting id="c.d">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 1, .m_false = 1, .m_true = 1},
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
};

TEST_P(TestConvertSettingBoolToIntMapping, Convert)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(params.m_originalSettings));

  auto conversionResult = CallConvertSettingBoolToInt(doc.RootElement(), m_oldSettingId,
                                                      m_newSettingId, params.m_mapping);
  ASSERT_EQ(TestConversions::TestConversionResult::CONVERTED, conversionResult);

  EXPECT_EQ(
      params.m_serializedOutput,
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestConvertSettingBoolToIntMapping,
                         testing::ValuesIn(ConvertSettingBoolToIntMappingTests));
