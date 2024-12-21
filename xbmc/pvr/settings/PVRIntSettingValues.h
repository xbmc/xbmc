/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

struct PVR_ATTRIBUTE_INT_VALUE;

namespace PVR
{
using SettingIntValue = std::pair<std::string, int>;

class CPVRIntSettingValues
{
public:
  CPVRIntSettingValues() = default;
  explicit CPVRIntSettingValues(int defaultValue);
  CPVRIntSettingValues(struct PVR_ATTRIBUTE_INT_VALUE* values,
                       unsigned int valuesSize,
                       int defaultValue,
                       int defaultDescriptionResourceId = 0);
  CPVRIntSettingValues(const std::vector<SettingIntValue>& values, int defaultValue);

  virtual ~CPVRIntSettingValues() = default;

  bool operator==(const CPVRIntSettingValues& right) const;

  const std::vector<SettingIntValue>& GetValues() const { return m_values; }
  int GetDefaultValue() const { return m_defaultValue; }

private:
  std::vector<SettingIntValue> m_values;
  int m_defaultValue{0};
};
} // namespace PVR
