/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/ISettingsMigrationStep.h"
#include "settings/lib/SettingsManager.h"
#include "settings/lib/SettingsMigration.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::SETTINGS;

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

  MigrationStepList steps;
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

  MigrationStepList steps;
  steps.push_back(std::make_shared<StubStep>(3, true, completed));
  steps.push_back(std::make_shared<StubStep>(3, true, completed));

  EXPECT_THROW({ CSettingsMigration(std::move(steps)); }, std::invalid_argument);
}

namespace
{
struct VideoVersionsMigrationTest
{
  std::string_view settings;
  std::string_view expectedSettings;
};

constexpr VideoVersionsMigrationTest VideoVersionsMigrationTests[]{
    {R"(<setting id="videolibrary.ignorevideoversions">true</setting>)",
     R"(<setting id="videolibrary.similarvideoaction">0</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions">false</setting>)",
     R"(<setting id="videolibrary.similarvideoaction">1</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions" default="true">true</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions" default="true">true</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions" default="TrUe">true</setting>)"
     R"(<setting id="dvds.autorun">true</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions" default="TrUe">true</setting>)"
     R"(<setting id="dvds.autoaction">1</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions" default="false">true</setting>)",
     R"(<setting id="videolibrary.similarvideoaction">0</setting>)"},
    {"", ""},
    {R"(<setting id="dvds.autorun">true</setting>)",
     R"(<setting id="dvds.autoaction">1</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions">true</setting>)"
     R"(<setting id="videolibrary.similarvideoaction">1</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions">true</setting>)"
     R"(<setting id="videolibrary.similarvideoaction">1</setting>)"},
    {R"(<setting id="videolibrary.ignorevideoversions">false</setting>)"
     R"(<setting id="videolibrary.similarvideoaction" default="true">2</setting>)"
     R"(<setting id="dvds.autorun">true</setting>)",
     R"(<setting id="videolibrary.ignorevideoversions">false</setting>)"
     R"(<setting id="videolibrary.similarvideoaction" default="true">2</setting>)"
     R"(<setting id="dvds.autoaction">1</setting>)"},
};
} // namespace

class TestSettingsMigrationToV3VideoVersions
  : public testing::WithParamInterface<VideoVersionsMigrationTest>,
    public testing::Test
{
};

TEST_P(TestSettingsMigrationToV3VideoVersions, MigratesVideoVersionPreference)
{
  const auto& params = GetParam();
  const std::string prefix = R"(<settings version="2">)"
                             R"(<setting id="lookandfeel.skin">skin.estuary</setting>)";
  const std::string xml = prefix + std::string{params.settings} + "</settings>";
  const std::string expectedXml = prefix + std::string{params.expectedSettings} + "</settings>";

  for (const int targetVersion : {3, static_cast<int>(CSettingsManager::Version)})
  {
    SCOPED_TRACE(targetVersion);
    CXBMCTinyXML doc;
    ASSERT_TRUE(doc.Parse(xml));

    CSettingsMigration migration;
    ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 2, targetVersion));

    EXPECT_EQ(expectedXml, XMLUtils::NodeStringSerialization(
                               doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
  }
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationToV3VideoVersions,
                         testing::ValuesIn(VideoVersionsMigrationTests));

namespace
{
struct ReplayGainMigrationTest
{
  std::string_view oldSettingId;
  std::string_view newSettingId;
  int oldValue;
  double expectedValue;
};

constexpr ReplayGainMigrationTest ReplayGainMigrationTests[]{
    {"musicplayer.replaygainpreamp", "musicplayer.replaygainpreampdb", 77, -12.0},
    {"musicplayer.replaygainpreamp", "musicplayer.replaygainpreampdb", 89, 0.0},
    {"musicplayer.replaygainpreamp", "musicplayer.replaygainpreampdb", 101, 12.0},
    {"musicplayer.replaygainnogainpreamp", "musicplayer.replaygainnogainpreampdb", 77, -12.0},
    {"musicplayer.replaygainnogainpreamp", "musicplayer.replaygainnogainpreampdb", 89, 0.0},
    {"musicplayer.replaygainnogainpreamp", "musicplayer.replaygainnogainpreampdb", 101, 12.0},
};
} // namespace

class TestSettingsMigrationToV4ReplayGain
  : public testing::WithParamInterface<ReplayGainMigrationTest>,
    public testing::Test
{
};

TEST_P(TestSettingsMigrationToV4ReplayGain, MigratesPreampSetting)
{
  const auto& params = GetParam();
  const std::string xml =
      "<settings version=\"3\"><setting id=\"" + std::string{params.oldSettingId} + "\">" +
      std::to_string(params.oldValue) +
      "</setting><setting id=\"lookandfeel.skin\">skin.estuary</setting></settings>";

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(xml));

  CSettingsMigration migration;
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 3, 4));

  EXPECT_EQ(nullptr, CSettingsManager::LocateSetting(doc.RootElement(), params.oldSettingId));

  const TiXmlElement* migratedSetting =
      CSettingsManager::LocateSetting(doc.RootElement(), params.newSettingId);
  ASSERT_NE(nullptr, migratedSetting);
  ASSERT_NE(nullptr, migratedSetting->FirstChild());
  EXPECT_DOUBLE_EQ(params.expectedValue, std::stod(migratedSetting->FirstChild()->ValueStr()));

  const TiXmlElement* unrelatedSetting =
      CSettingsManager::LocateSetting(doc.RootElement(), "lookandfeel.skin");
  ASSERT_NE(nullptr, unrelatedSetting);
  ASSERT_NE(nullptr, unrelatedSetting->FirstChild());
  EXPECT_EQ("skin.estuary", unrelatedSetting->FirstChild()->ValueStr());
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationToV4ReplayGain,
                         testing::ValuesIn(ReplayGainMigrationTests));

class TestSettingsMigrationToV5ApplicationKeyboard : public testing::TestWithParam<std::string>
{
};

TEST_P(TestSettingsMigrationToV5ApplicationKeyboard, MigratesTvosKeyboardSetting)
{
  const std::string& value = GetParam();
  const std::string xml = "<settings version=\"4\"><setting id=\"input.tvosusekodikeyboard\">" +
                          value + "</setting></settings>";

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(xml));

  CSettingsMigration migration;
  ASSERT_TRUE(migration.UpdateXMLSettings(doc.RootElement(), 4, 5));

  EXPECT_EQ(nullptr,
            CSettingsManager::LocateSetting(doc.RootElement(), "input.tvosusekodikeyboard"));

  const TiXmlElement* migratedSetting =
      CSettingsManager::LocateSetting(doc.RootElement(), "input.useapplicationkeyboard");
  ASSERT_NE(nullptr, migratedSetting);
  ASSERT_NE(nullptr, migratedSetting->FirstChild());
  EXPECT_EQ(value, migratedSetting->FirstChild()->ValueStr());
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigration,
                         TestSettingsMigrationToV5ApplicationKeyboard,
                         testing::Values("true", "false"));
