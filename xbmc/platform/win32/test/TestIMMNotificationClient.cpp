/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/win32/IMMNotificationClient.h"

#include <gtest/gtest.h>

using namespace KODI::PLATFORM::WINDOWS::INTERNAL;

TEST(TestIMMNotificationClient, DeviceSettingMatchesEndpoint)
{
  const std::string denonEndpoint = "{4162b786-8c9a-4678-aabc-3800a6427f65}";
  const std::string soundcoreEndpoint = "{956f88b0-afe4-44e1-aada-f18ec36d7b1f}";
  const std::string configuredDevice =
      "WASAPI:{4162B786-8C9A-4678-AABC-3800A6427F65}|HDMI - DENON-AVR";

  EXPECT_TRUE(DeviceSettingMatchesEndpoint(configuredDevice, denonEndpoint));
  EXPECT_FALSE(DeviceSettingMatchesEndpoint(configuredDevice, soundcoreEndpoint));
  EXPECT_FALSE(DeviceSettingMatchesEndpoint("WASAPI:default", soundcoreEndpoint));
  EXPECT_FALSE(DeviceSettingMatchesEndpoint(configuredDevice, ""));
  EXPECT_FALSE(DeviceSettingMatchesEndpoint("", denonEndpoint));
}

TEST(TestIMMNotificationClient, DisabledPassthroughDeviceDoesNotMatchEndpoint)
{
  const std::string denonEndpoint = "{4162b786-8c9a-4678-aabc-3800a6427f65}";
  const std::string soundcoreEndpoint = "{956f88b0-afe4-44e1-aada-f18ec36d7b1f}";
  const std::string audioDevice = "WASAPI:{4162B786-8C9A-4678-AABC-3800A6427F65}|HDMI - DENON-AVR";
  const std::string passthroughDevice =
      "WASAPI:{956F88B0-AFE4-44E1-AADA-F18EC36D7B1F}|Soundcore Headphones";

  EXPECT_TRUE(
      ConfiguredDeviceMatchesEndpoint(audioDevice, passthroughDevice, false, denonEndpoint));
  EXPECT_FALSE(
      ConfiguredDeviceMatchesEndpoint(audioDevice, passthroughDevice, false, soundcoreEndpoint));
  EXPECT_TRUE(
      ConfiguredDeviceMatchesEndpoint(audioDevice, passthroughDevice, true, soundcoreEndpoint));
}
