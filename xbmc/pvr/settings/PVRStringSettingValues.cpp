/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRStringSettingValues.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_defines.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <string>

namespace PVR
{
CPVRStringSettingValues::CPVRStringSettingValues(struct PVR_ATTRIBUTE_STRING_VALUE* values,
                                                 unsigned int valuesSize,
                                                 const std::string& defaultValue,
                                                 int defaultDescriptionResourceId /* = 0 */)
  : m_defaultValue(defaultValue)
{
  if (values && valuesSize > 0)
  {
    m_values.reserve(valuesSize);
    for (unsigned int i = 0; i < valuesSize; ++i)
    {
      const std::string value{values[i].strValue ? values[i].strValue : ""};
      const char* desc{values[i].strDescription};
      std::string description{desc ? desc : ""};
      if (description.empty())
      {
        // No description given by addon. Create one from value.
        if (defaultDescriptionResourceId > 0)
          description = StringUtils::Format(
              "{} {}", g_localizeStrings.Get(defaultDescriptionResourceId), value);
        else
          description = value;
      }
      m_values.emplace_back(description, value);
    }
  }
}

bool CPVRStringSettingValues::operator==(const CPVRStringSettingValues& right) const
{
  return (m_defaultValue == right.m_defaultValue && m_values == right.m_values);
}
} // namespace PVR
