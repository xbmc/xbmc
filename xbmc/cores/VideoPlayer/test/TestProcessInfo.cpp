/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <memory>

#include <gtest/gtest.h>

class TestProcessInfo : public testing::Test
{
protected:
  void SetUp() override
  {
    m_advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    ASSERT_TRUE(m_advancedSettings != nullptr);
    m_minTempo = m_advancedSettings->m_minTempo;
  }

  void TearDown() override { m_advancedSettings->m_minTempo = m_minTempo; }

  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  std::unique_ptr<CProcessInfo> m_processInfo{CProcessInfo::CreateInstance()};
  float m_minTempo{};
};

TEST_F(TestProcessInfo, DefaultTempoRangeIsUnchanged)
{
  EXPECT_FALSE(m_processInfo->IsTempoAllowed(0.7f));
  EXPECT_TRUE(m_processInfo->IsTempoAllowed(0.8f));
  EXPECT_TRUE(m_processInfo->IsTempoAllowed(1.5f));
  EXPECT_FALSE(m_processInfo->IsTempoAllowed(1.7f));
}

TEST_F(TestProcessInfo, MinTempoIsInclusive)
{
  m_advancedSettings->m_minTempo = 0.5f;

  EXPECT_TRUE(m_processInfo->IsTempoAllowed(0.5f));
  EXPECT_TRUE(m_processInfo->IsTempoAllowed(0.6f));
  EXPECT_FALSE(m_processInfo->IsTempoAllowed(0.4f));
  EXPECT_FALSE(m_processInfo->IsTempoAllowed(1.7f));
}
