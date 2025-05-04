/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRIntSettingDefinition.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_general.h"

namespace PVR
{
CPVRIntSettingDefinition::CPVRIntSettingDefinition(const PVR_INT_SETTING_DEFINITION& def)
  : m_values{def.values, def.iValuesSize, def.iDefaultValue},
    m_minValue{def.iMinValue},
    m_step{def.iStep},
    m_maxValue{def.iMaxValue}
{
}
} // namespace PVR
