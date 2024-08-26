/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/settings/PVRStringSettingValues.h"

#include <vector>

struct PVR_STRING_SETTING_DEFINITION;

namespace PVR
{
class CPVRStringSettingDefinition
{
public:
  CPVRStringSettingDefinition() = default;
  explicit CPVRStringSettingDefinition(const PVR_STRING_SETTING_DEFINITION& def);

  virtual ~CPVRStringSettingDefinition() = default;

  bool operator==(const CPVRStringSettingDefinition& right) const;
  bool operator!=(const CPVRStringSettingDefinition& right) const;

  const std::vector<SettingStringValue>& GetValues() const { return m_values.GetValues(); }
  const std::string& GetDefaultValue() const { return m_values.GetDefaultValue(); }

  bool IsAllowEmptyValue() const { return m_allowEmptyValue; }

private:
  CPVRStringSettingValues m_values;
  bool m_allowEmptyValue{true};
};
} // namespace PVR
