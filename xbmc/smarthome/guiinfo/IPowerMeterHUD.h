/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

namespace KODI::SMART_HOME
{
constexpr double CURRENT_SHARE_ZERO_AMPS = 0.25;
constexpr double CURRENT_SHARE_FULL_AMPS = 0.75;

inline double CalculateCurrentShare(double currentThis, double currentOther)
{
  currentThis = std::abs(currentThis);
  currentOther = std::abs(currentOther);
  const double total = currentThis + currentOther;
  if (total <= CURRENT_SHARE_ZERO_AMPS)
    return 50.0;

  const double rawShare = std::clamp(100.0 * currentThis / total, 0.0, 100.0);
  if (total >= CURRENT_SHARE_FULL_AMPS)
    return rawShare;

  const double blend =
      (total - CURRENT_SHARE_ZERO_AMPS) / (CURRENT_SHARE_FULL_AMPS - CURRENT_SHARE_ZERO_AMPS);
  return std::clamp(50.0 + blend * (rawShare - 50.0), 0.0, 100.0);
}

struct PowerMeterReading
{
  double voltage{0.0};
  double current{0.0};
  double power{0.0};
};

class IPowerMeterHUD
{
public:
  virtual ~IPowerMeterHUD() = default;
  virtual std::optional<PowerMeterReading> PowerMeter(const std::string& systemName,
                                                      const std::string& powerMeterName) = 0;
  virtual std::optional<double> CurrentShare(const std::string& systemName,
                                             const std::string& powerMeterName,
                                             const std::string& otherPowerMeterName) = 0;
};
} // namespace KODI::SMART_HOME
