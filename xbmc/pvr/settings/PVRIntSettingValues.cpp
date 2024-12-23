/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRIntSettingValues.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_defines.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <string>

namespace PVR
{
CPVRIntSettingValues::CPVRIntSettingValues(struct PVR_ATTRIBUTE_INT_VALUE* values,
                                           unsigned int valuesSize,
                                           int defaultValue,
                                           int defaultDescriptionResourceId /* = 0 */)
  : m_defaultValue(defaultValue)
{
  if (values && valuesSize > 0)
  {
    m_values.reserve(valuesSize);
    for (unsigned int i = 0; i < valuesSize; ++i)
    {
      const int value{values[i].iValue};
      const char* desc{values[i].strDescription};
      std::string description{desc ? desc : ""};
      if (description.empty())
      {
        // No description given by addon. Create one from value.
        if (defaultDescriptionResourceId > 0)
          description = StringUtils::Format(
              "{} {}", g_localizeStrings.Get(defaultDescriptionResourceId), value);
        else
          description = std::to_string(value);
      }
      m_values.emplace_back(description, value);
    }
  }
}

CPVRIntSettingValues::CPVRIntSettingValues(int defaultValue) : m_defaultValue(defaultValue)
{
}

CPVRIntSettingValues::CPVRIntSettingValues(const std::vector<SettingIntValue>& values,
                                           int defaultValue)
  : m_values(values), m_defaultValue(defaultValue)
{
}

bool CPVRIntSettingValues::operator==(const CPVRIntSettingValues& right) const
{
  return (m_defaultValue == right.m_defaultValue && m_values == right.m_values);
}
} // namespace PVR
