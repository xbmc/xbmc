/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/ISettingsMigrationStep.h"
#include "settings/lib/SettingsMigration.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <stdexcept>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

struct StepSpec
{
  int version;
  bool returns{true};
};

struct UpdateXMLSettingsTest
{
  std::vector<StepSpec> steps;
  int currentVersion;
  int targetVersion;
  bool expectedReturn;
  std::vector<int> expectedCompleted;
};

const UpdateXMLSettingsTest UpdateXMLSettingsTests[]{
    // "normal" guisettings.xml cases - contiguous steps from version 2
    {{{3}}, 2, 3, true, {3}},
    {{{3}, {4}}, 2, 4, true, {3, 4}},

    // advancedsettings.xml typically doesn't have a version number => version 0
    {{{3}}, 0, 3, true, {3}},

    // steps below or above the range are skipped
    {{{3}, {4}, {5}}, 3, 4, true, {4}},
    {{{3}, {4}, {5}}, 4, 5, true, {5}},

    // gaps in the steps
    {{{3}, {5}}, 2, 5, true, {3, 5}},

    // steps exist only outside of the range
    {{{1}}, 2, 3, true, {}},
    {{{10}}, 2, 3, true, {}},

    // no steps
    {{}, 2, 3, true, {}},

    // steps provided out of order
    {{{5}, {3}, {4}}, 2, 5, true, {3, 4, 5}},

    // stop on first error
    {{{3, false}, {4}}, 2, 4, false, {3}},
    {{{3}, {4, false}}, 2, 4, false, {3, 4}},

    // no work under the minimum version LEGACY_VERSION = 2 or already on current version
    {{{0}, {1}, {2}}, 0, 2, true, {}},
    {{}, 2, 2, true, {}},
    {{{3}}, 3, 3, true, {}},
};

class TestSettingsMigrationUpdateXML : public testing::WithParamInterface<UpdateXMLSettingsTest>,
                                       public testing::Test
{
};

struct StubStep : ISettingsMigrationStep
{
  StubStep(int version, bool returnValue, std::vector<int>& completionLog)
    : m_version(version),
      m_returnValue(returnValue),
      m_log(completionLog)
  {
  }
  int TargetVersion() const override { return m_version; }
  bool Apply(TiXmlElement*) override
  {
    m_log.push_back(m_version);
    return m_returnValue;
  }

private:
  int m_version;
  bool m_returnValue{true};
  std::vector<int>& m_log;
};

TEST_P(TestSettingsMigrationUpdateXML, UpdateXMLSettings)
{
  const auto& params = GetParam();

  std::vector<int> completed;

  CSettingsMigration::StepList steps;
  for (const auto& step : params.steps)
    steps.push_back(std::make_shared<StubStep>(step.version, step.returns, completed));

  CSettingsMigration migration(std::move(steps));

  EXPECT_EQ(params.expectedReturn,
            migration.UpdateXMLSettings(nullptr, params.currentVersion, params.targetVersion));
  EXPECT_EQ(params.expectedCompleted, completed);
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationUpdateXML,
                         testing::ValuesIn(UpdateXMLSettingsTests));

TEST(TestSettingsMigration, MultipleMigrationStepsSameTarget)
{
  std::vector<int> completed;

  CSettingsMigration::StepList steps;
  steps.push_back(std::make_shared<StubStep>(3, true, completed));
  steps.push_back(std::make_shared<StubStep>(3, true, completed));

  EXPECT_THROW({ CSettingsMigration(std::move(steps)); }, std::invalid_argument);
}

class TestConversions : public ::testing::Test
{
public:
  // All tests will attempt this conversion
  std::string_view m_oldSettingId = "a.b";
  std::string_view m_newSettingId = "c.d";
};

struct ConvertSettingBoolToIntTest
{
  std::string m_originalSettings;
  CSettingsMigration::SettingConversionResult m_result;
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
     CSettingsMigration::SettingConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     CSettingsMigration::SettingConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
    // Convert one of multiple settings
    {R"(<settings version="2">
          <setting id="a.b">false</setting>
          <setting id="foo.bar">test</setting>
        </settings>)",
     CSettingsMigration::SettingConversionResult::CONVERTED,
     R"(<settings version="2">)"
     R"(<setting id="foo.bar">test</setting>)"
     R"(<setting id="c.d" default="true">1</setting>)"
     R"(</settings>)"},
    // V1 format
    // Convert a true value
    {R"(<settings><a><b>true</b></a></settings>)",
     CSettingsMigration::SettingConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings><a><b>false</b></a></settings>)",
     CSettingsMigration::SettingConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d" default="true">1</setting></settings>)"},
    // Convert one of multiple settings
    {R"(<settings version="2">
          <a><b>false</b></a>
          <foo><bar>test</bar></foo>
        </settings>)",
     CSettingsMigration::SettingConversionResult::CONVERTED,
     R"(<settings version="2">)"
     R"(<a />)"
     R"(<foo><bar>test</bar></foo>)"
     R"(<setting id="c.d" default="true">1</setting>)"
     R"(</settings>)"},

    // Invalid old setting values
    // V2 format
    {R"(<settings version="2"><setting id="a.b">notbool</setting></settings>)",
     CSettingsMigration::SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b">notbool</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"> true</setting></settings>)",
     CSettingsMigration::SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b"> true</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"></setting></settings>)",
     CSettingsMigration::SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b" /></settings>)"},
    {R"(<settings version="2"><setting id="a.b" /></settings>)",
     CSettingsMigration::SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b" /></settings>)"},
    // V1 format
    {R"(<settings><a><b>notbool</b></a></settings>)",
     CSettingsMigration::SettingConversionResult::INVALID,
     R"(<settings><a><b>notbool</b></a></settings>)"},
    {R"(<settings><a><b></b></a></settings>)", CSettingsMigration::SettingConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},
    {R"(<settings><a><b /></a></settings>)", CSettingsMigration::SettingConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},

    // Old setting is not present
    // V2 format
    {R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)",
     CSettingsMigration::SettingConversionResult::NOT_PRESENT,
     R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)"},
    {R"(<settings version="2"></settings>)",
     CSettingsMigration::SettingConversionResult::NOT_PRESENT, R"(<settings version="2" />)"},
    {R"(<settings version="2" />)", CSettingsMigration::SettingConversionResult::NOT_PRESENT,
     R"(<settings version="2" />)"},
    // V1 format
    {R"(<settings><foo><bar>true</bar></foo></settings>)",
     CSettingsMigration::SettingConversionResult::NOT_PRESENT,
     R"(<settings><foo><bar>true</bar></foo></settings>)"},
    {R"(<settings><a><bar>true</bar></a></settings>)",
     CSettingsMigration::SettingConversionResult::NOT_PRESENT,
     R"(<settings><a><bar>true</bar></a></settings>)"},
    // the other V1 format tests are identical to the V2, except for the version attribute
    // which is not actively used by code
};

TEST_P(TestConvertSettingBoolToInt, Convert)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(params.m_originalSettings));

  auto conversionResult =
      CSettingsMigration::ConvertSettingBoolToInt(doc.RootElement(), m_oldSettingId, m_newSettingId,
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
  CSettingsMigration::SettingBoolToIntMapping m_mapping;
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

  auto conversionResult = CSettingsMigration::ConvertSettingBoolToInt(
      doc.RootElement(), m_oldSettingId, m_newSettingId, params.m_mapping);
  ASSERT_EQ(CSettingsMigration::SettingConversionResult::CONVERTED, conversionResult);

  EXPECT_EQ(
      params.m_serializedOutput,
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestConvertSettingBoolToIntMapping,
                         testing::ValuesIn(ConvertSettingBoolToIntMappingTests));
