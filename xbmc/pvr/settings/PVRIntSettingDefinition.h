/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/settings/PVRIntSettingValues.h"

#include <vector>

struct PVR_INT_SETTING_DEFINITION;

namespace PVR
{
class CPVRIntSettingDefinition
{
public:
  CPVRIntSettingDefinition() = default;
  explicit CPVRIntSettingDefinition(const PVR_INT_SETTING_DEFINITION& def);

  virtual ~CPVRIntSettingDefinition() = default;

  bool operator==(const CPVRIntSettingDefinition& right) const;
  bool operator!=(const CPVRIntSettingDefinition& right) const;

  const std::vector<SettingIntValue>& GetValues() const { return m_values.GetValues(); }
  int GetDefaultValue() const { return m_values.GetDefaultValue(); }
  int GetMinValue() const { return m_minValue; }
  int GetStepValue() const { return m_step; }
  int GetMaxValue() const { return m_maxValue; }

private:
  CPVRIntSettingValues m_values;
  int m_minValue{0};
  int m_step{0};
  int m_maxValue{0};
};
} // namespace PVR
