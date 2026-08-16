/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/SettingsMigrationPrimitive.h"
#include "utils/XMLUtils.h"

#include <string_view>

#include <gtest/gtest.h>

using namespace KODI::SETTINGS;

namespace
{
// All tests will attempt this conversion
constexpr std::string_view OLDSETTINGID = "a.b";
constexpr std::string_view NEWSETTINGID = "c.d";
} // namespace

struct ConvertSettingBoolToIntTest
{
  std::string m_originalSettings;
  SettingConversionResult m_result;
  std::string_view m_serializedOutput;
};

class TestConvertSettingBoolToInt : public ::testing::Test,
                                    public testing::WithParamInterface<ConvertSettingBoolToIntTest>
{
};

ConvertSettingBoolToIntTest ConvertSettingBoolToIntTests[] = {
    // Successful conversions
    // V2 format
    // Convert a true value
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     SettingConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     SettingConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
    // Convert one of multiple settings
    {R"(<settings version="2">
          <setting id="a.b">false</setting>
          <setting id="foo.bar">test</setting>
        </settings>)",
     SettingConversionResult::CONVERTED,
     R"(<settings version="2">)"
     R"(<setting id="foo.bar">test</setting>)"
     R"(<setting id="c.d" default="true">1</setting>)"
     R"(</settings>)"},
    // V1 format
    // Convert a true value
    {R"(<settings><a><b>true</b></a></settings>)", SettingConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings><a><b>false</b></a></settings>)", SettingConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d" default="true">1</setting></settings>)"},
    // Convert one of multiple settings
    {R"(<settings version="2">
          <a><b>false</b></a>
          <foo><bar>test</bar></foo>
        </settings>)",
     SettingConversionResult::CONVERTED,
     R"(<settings version="2">)"
     R"(<a />)"
     R"(<foo><bar>test</bar></foo>)"
     R"(<setting id="c.d" default="true">1</setting>)"
     R"(</settings>)"},

    // Invalid old setting values
    // V2 format
    {R"(<settings version="2"><setting id="a.b">notbool</setting></settings>)",
     SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b">notbool</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"> true</setting></settings>)",
     SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b"> true</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"></setting></settings>)",
     SettingConversionResult::INVALID, R"(<settings version="2"><setting id="a.b" /></settings>)"},
    {R"(<settings version="2"><setting id="a.b" /></settings>)", SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b" /></settings>)"},
    // V1 format
    {R"(<settings><a><b>notbool</b></a></settings>)", SettingConversionResult::INVALID,
     R"(<settings><a><b>notbool</b></a></settings>)"},
    {R"(<settings><a><b></b></a></settings>)", SettingConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},
    {R"(<settings><a><b /></a></settings>)", SettingConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},

    // Old setting is not present
    // V2 format
    {R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)",
     SettingConversionResult::NOT_PRESENT,
     R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)"},
    {R"(<settings version="2"></settings>)", SettingConversionResult::NOT_PRESENT,
     R"(<settings version="2" />)"},
    {R"(<settings version="2" />)", SettingConversionResult::NOT_PRESENT,
     R"(<settings version="2" />)"},
    // V1 format
    {R"(<settings><foo><bar>true</bar></foo></settings>)", SettingConversionResult::NOT_PRESENT,
     R"(<settings><foo><bar>true</bar></foo></settings>)"},
    {R"(<settings><a><bar>true</bar></a></settings>)", SettingConversionResult::NOT_PRESENT,
     R"(<settings><a><bar>true</bar></a></settings>)"},
    // the other V1 format tests are identical to the V2, except for the version attribute
    // which is not actively used by code
};

TEST_P(TestConvertSettingBoolToInt, Convert)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(params.m_originalSettings));

  auto conversionResult = ConvertSettingBoolToInt(doc.RootElement(), OLDSETTINGID, NEWSETTINGID,
                                                  {.m_default = 1, .m_false = 1, .m_true = 2});
  ASSERT_EQ(params.m_result, conversionResult);

  EXPECT_EQ(
      params.m_serializedOutput,
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationPrimitive,
                         TestConvertSettingBoolToInt,
                         testing::ValuesIn(ConvertSettingBoolToIntTests));

struct ConvertSettingBoolToIntMappingTest
{
  std::string m_originalSettings;
  SettingBoolToIntMapping m_mapping;
  std::string_view m_serializedOutput;
};

class TestConvertSettingBoolToIntMapping
  : public ::testing::Test,
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

  auto conversionResult =
      ConvertSettingBoolToInt(doc.RootElement(), OLDSETTINGID, NEWSETTINGID, params.m_mapping);
  ASSERT_EQ(SettingConversionResult::CONVERTED, conversionResult);

  EXPECT_EQ(
      params.m_serializedOutput,
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationPrimitive,
                         TestConvertSettingBoolToIntMapping,
                         testing::ValuesIn(ConvertSettingBoolToIntMappingTests));
