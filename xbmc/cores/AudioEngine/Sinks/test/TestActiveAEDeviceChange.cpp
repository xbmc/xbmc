/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEDeviceChange.h"

#include <gtest/gtest.h>

using namespace ActiveAE::INTERNAL;

TEST(TestActiveAEDeviceChange, KeepsHealthyExplicitDeviceForUnrelatedChange)
{
  const DeviceChangeDecision decision{true, false, false, false, true, true};
  EXPECT_FALSE(ShouldReconfigure(decision));
}

TEST(TestActiveAEDeviceChange, ReconfiguresWhenCurrentDeviceDisappears)
{
  const DeviceChangeDecision decision{false, false, false, false, false, false};
  EXPECT_TRUE(ShouldReconfigure(decision));
}

TEST(TestActiveAEDeviceChange, RestoresAvailablePreferredDevice)
{
  const DeviceChangeDecision decision{true, false, true, false, true, false};
  EXPECT_TRUE(ShouldReconfigure(decision));
}

TEST(TestActiveAEDeviceChange, KeepsFallbackWhilePreferredDeviceIsUnavailable)
{
  const DeviceChangeDecision decision{true, false, true, false, false, false};
  EXPECT_FALSE(ShouldReconfigure(decision));
}

TEST(TestActiveAEDeviceChange, TracksDefaultDeviceChanges)
{
  const DeviceChangeDecision configuredDefault{true, true, true, true, true, false};
  const DeviceChangeDecision fallbackDefault{true, true, true, false, false, false};

  EXPECT_TRUE(ShouldReconfigure(configuredDefault));
  EXPECT_TRUE(ShouldReconfigure(fallbackDefault));
}

TEST(TestActiveAEDeviceChange, IgnoresDefaultChangeForHealthyExplicitDevice)
{
  const DeviceChangeDecision decision{true, true, false, false, true, true};
  EXPECT_FALSE(ShouldReconfigure(decision));
}

TEST(TestActiveAEDeviceChange, MergesBurstFromSameDriver)
{
  PendingDeviceChange change;
  EXPECT_FALSE(change.pending);

  change.Add("PULSE", false);
  change.Add("PULSE", false);

  EXPECT_TRUE(change.pending);
  EXPECT_EQ(change.driver, "PULSE");
  EXPECT_FALSE(change.defaultDeviceChanged);
}

TEST(TestActiveAEDeviceChange, EnumeratesAllDriversForMixedBurst)
{
  PendingDeviceChange change;
  change.Add("PULSE", false);
  change.Add("ALSA", false);
  change.Add("PULSE", false);

  EXPECT_TRUE(change.pending);
  EXPECT_TRUE(change.driver.empty());
}

TEST(TestActiveAEDeviceChange, KeepsDefaultChangeFromAnyEventInBurst)
{
  PendingDeviceChange change;
  change.Add("", false);
  change.Add("", true);
  change.Add("", false);

  EXPECT_TRUE(change.pending);
  EXPECT_TRUE(change.driver.empty());
  EXPECT_TRUE(change.defaultDeviceChanged);
}
