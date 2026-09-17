/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace ActiveAE::INTERNAL
{
// Endpoints can report many state changes for one event (e.g. HDMI audio during a display
// mode switch) and each handled change re-enumerates all sinks, which can take seconds.
struct PendingDeviceChange
{
  bool pending{false};
  std::string driver;
  bool defaultDeviceChanged{false};

  void Add(const std::string& eventDriver, bool eventDefaultDeviceChanged)
  {
    if (!pending)
      driver = eventDriver;
    else if (driver != eventDriver)
      driver.clear(); // mixed drivers: enumerate all

    defaultDeviceChanged = defaultDeviceChanged || eventDefaultDeviceChanged;
    pending = true;
  }
};

struct DeviceChangeDecision
{
  bool currentDeviceExists{false};
  bool defaultDeviceChanged{false};
  bool currentDeviceFollowsDefault{false};
  bool preferredDeviceIsDefault{false};
  bool preferredDeviceAvailable{false};
  bool preferredDeviceIsCurrent{false};
};

constexpr bool ShouldReconfigure(const DeviceChangeDecision& decision)
{
  if (!decision.currentDeviceExists)
    return true;

  if (decision.defaultDeviceChanged &&
      (decision.preferredDeviceIsDefault || decision.currentDeviceFollowsDefault))
  {
    return true;
  }

  return !decision.preferredDeviceIsDefault && decision.preferredDeviceAvailable &&
         !decision.preferredDeviceIsCurrent;
}
} // namespace ActiveAE::INTERNAL
