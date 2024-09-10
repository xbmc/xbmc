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

struct PVR_ATTRIBUTE_STRING_VALUE;

namespace PVR
{
using SettingStringValue = std::pair<std::string, std::string>;

class CPVRStringSettingValues
{
public:
  CPVRStringSettingValues() = default;
  CPVRStringSettingValues(struct PVR_ATTRIBUTE_STRING_VALUE* values,
                          unsigned int valuesSize,
                          const std::string& defaultValue,
                          int defaultDescriptionResourceId = 0);

  virtual ~CPVRStringSettingValues() = default;

  bool operator==(const CPVRStringSettingValues& right) const;

  const std::vector<SettingStringValue>& GetValues() const { return m_values; }
  const std::string& GetDefaultValue() const { return m_defaultValue; }

private:
  std::vector<SettingStringValue> m_values;
  std::string m_defaultValue;
};
} // namespace PVR
