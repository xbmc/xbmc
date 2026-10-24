/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "jobs/JobManager.h"
#include "settings/Settings.h"
#include "test/TestUtils.h"
#include "video/geometry/ContentGeometryScanner.h"

#include <memory>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

// A paused job manager holds the sweep in its queue, so the test decides when a sweep ends.
class TestContentGeometryScanner : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_jobManagerWas = CServiceBroker::GetJobManager();
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    CServiceBroker::GetJobManager()->PauseJobs();

    ASSERT_FALSE(m_scanner.IsSweeping()) << "a sweep was already running before this test";
  }

  void TearDown() override
  {
    m_scanner.StopSweep();
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::UnregisterJobManager();
    if (m_jobManagerWas)
      CServiceBroker::RegisterJobManager(m_jobManagerWas);
  }

  void EndSweep() { m_scanner.OnJobComplete(0, true, nullptr); }

  const CScopedSetting m_extract{CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY, true};
  const CScopedSetting m_library{CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN, true};
  CContentGeometryScanner& m_scanner{CContentGeometryScanner::GetInstance()};
  std::shared_ptr<CJobManager> m_jobManagerWas;
};

// The test environment's settings are never marked loaded, so their change callbacks never fire.
// This store's do. It holds no settings handlers or sub-settings: those are shared with the
// environment's store, and unloading this one would reset them.
class CLoadedSettings : public CSettings
{
public:
  CLoadedSettings()
  {
    if (Initialize())
      SetLoaded();
  }
  ~CLoadedSettings() override { Uninitialize(); }

protected:
  void InitializeISettingsHandlers() override {}
  void UninitializeISettingsHandlers() override {}
  void InitializeISubSettings() override {}
  void UninitializeISubSettings() override {}
};

} // namespace

// A library scan asks for a sweep when it finishes, often while the startup sweep is still
// working through its own list; the files the scan added must still be measured.
TEST_F(TestContentGeometryScanner, ARequestDuringASweepRunsWhenItEnds)
{
  m_scanner.Sweep();
  ASSERT_TRUE(m_scanner.IsSweeping());

  m_scanner.Sweep();
  EndSweep();
  EXPECT_TRUE(m_scanner.IsSweeping());

  EndSweep();
  EXPECT_FALSE(m_scanner.IsSweeping());
}

// Turning the setting off and on again before a queued sweep starts leaves a sweep wanted.
TEST_F(TestContentGeometryScanner, ARequestAfterAStopStillRuns)
{
  m_scanner.Sweep();
  m_scanner.StopSweep();
  m_scanner.Sweep();

  EndSweep();
  EXPECT_TRUE(m_scanner.IsSweeping());
  EXPECT_FALSE(m_scanner.IsStopRequested());
}

TEST_F(TestContentGeometryScanner, AStopDropsAHeldRequest)
{
  m_scanner.Sweep();
  m_scanner.Sweep(true);
  m_scanner.StopSweep();

  EndSweep();
  EXPECT_FALSE(m_scanner.IsSweeping());
}

TEST_F(TestContentGeometryScanner, MeasuringAwayFromPlaybackStartsAndStopsTheSweep)
{
  CLoadedSettings settings;
  ASSERT_TRUE(settings.IsLoaded());

  ASSERT_TRUE(settings.SetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN, true));
  EXPECT_TRUE(m_scanner.IsSweeping());
  ASSERT_FALSE(m_scanner.IsStopRequested());

  ASSERT_TRUE(settings.SetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN, false));
  EXPECT_TRUE(m_scanner.IsStopRequested());
}
