/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "video/geometry/ContentGeometryScanner.h"
#include "video/geometry/GeometrySettings.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

class TestGeometrySettings : public ::testing::Test
{
protected:
  void SetUp() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_extractWas = settings->GetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY);
    m_libraryWas = settings->GetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN);
  }

  void TearDown() override { Set(m_extractWas, m_libraryWas); }

  static void Set(bool extract, bool library)
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY, extract);
    settings->SetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN, library);
  }

  bool m_extractWas{false};
  bool m_libraryWas{false};
};

} // namespace

// Measuring away from the playing picture is its own decision, and needs both settings: the
// feature on, and measurement opted into. With only the feature on, the shape comes from live
// detection and nothing is sampled behind the viewer.
TEST_F(TestGeometrySettings, MeasuringAwayFromPlaybackNeedsBothSettings)
{
  Set(false, false);
  EXPECT_FALSE(ContentGeometryNonLiveFromSettings());

  Set(false, true);
  EXPECT_FALSE(ContentGeometryNonLiveFromSettings());

  Set(true, false);
  EXPECT_FALSE(ContentGeometryNonLiveFromSettings());

  Set(true, true);
  EXPECT_TRUE(ContentGeometryNonLiveFromSettings());
}

// The feature being on says nothing about measuring the library, so it must not put hours of
// work under way on its own.
TEST_F(TestGeometrySettings, SweepDoesNotStartWithoutTheMeasurementSetting)
{
  auto& scanner{CContentGeometryScanner::GetInstance()};
  ASSERT_FALSE(scanner.IsSweeping()) << "a sweep was already running before this test";

  Set(true, false);
  scanner.Sweep();
  EXPECT_FALSE(scanner.IsSweeping());
}

// And with the feature itself off, nothing measures the library whatever the other setting says.
TEST_F(TestGeometrySettings, SweepDoesNotStartWithTheFeatureOff)
{
  auto& scanner{CContentGeometryScanner::GetInstance()};
  ASSERT_FALSE(scanner.IsSweeping()) << "a sweep was already running before this test";

  Set(false, true);
  scanner.Sweep();
  EXPECT_FALSE(scanner.IsSweeping());
}
