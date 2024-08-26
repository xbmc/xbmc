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
      std::string strDescr{desc ? desc : ""};
      if (strDescr.empty())
      {
        // No description given by addon. Create one from value.
        if (defaultDescriptionResourceId > 0)
          strDescr = StringUtils::Format(
              "{} {}", g_localizeStrings.Get(defaultDescriptionResourceId), value);
        else
          strDescr = std::to_string(value);
      }
      m_values.emplace_back(strDescr, value);
    }
  }
}

CPVRIntSettingValues::CPVRIntSettingValues(struct PVR_ATTRIBUTE_INT_VALUE* values,
                                           unsigned int valuesSize,
                                           unsigned int defaultValue,
                                           int defaultDescriptionResourceId /* = 0 */)
  : CPVRIntSettingValues(
        values, valuesSize, static_cast<int>(defaultValue), defaultDescriptionResourceId)
{
}

CPVRIntSettingValues::CPVRIntSettingValues(const std::vector<SettingIntValue>& values,
                                           int defaultValue)
  : m_values(values), m_defaultValue(defaultValue)
{
}

} // namespace PVR
