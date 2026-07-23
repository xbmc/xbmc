/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace ActiveAE::INTERNAL
{
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
