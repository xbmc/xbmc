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

#include <stdexcept>
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
