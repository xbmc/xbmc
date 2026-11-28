/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace PVR
{
// All bounds use the player's millisecond timeline, including its nonzero buffer origin.
struct TimeshiftProgress
{
  int64_t bufferStart{0};
  int64_t bufferEnd{0};
  int64_t progressStart{0};
  int64_t progressEnd{0};

  double GetPercentage(int64_t time) const
  {
    if (progressEnd <= progressStart)
      return 0.0;
    return 100.0 * (static_cast<double>(time) - progressStart) /
           (static_cast<double>(progressEnd) - progressStart);
  }

  std::optional<int64_t> GetSeekTime(double percentage) const
  {
    if (!std::isfinite(percentage) || percentage < 0.0 || percentage > 100.0 ||
        bufferEnd <= bufferStart || progressEnd <= progressStart)
      return std::nullopt;

    const double target =
        progressStart + percentage * 0.01 * (static_cast<double>(progressEnd) - progressStart);
    return std::llround(
        std::clamp(target, static_cast<double>(bufferStart), static_cast<double>(bufferEnd)));
  }
};
} // namespace PVR
