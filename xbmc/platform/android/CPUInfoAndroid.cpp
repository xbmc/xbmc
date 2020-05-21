/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoAndroid.h"

#include "utils/Temperature.h"

#include "platform/android/activity/AndroidFeatures.h"

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoAndroid>();
}

CCPUInfoAndroid::CCPUInfoAndroid()
{
  m_cpuCount = CAndroidFeatures::GetCPUCount();

  for (int i = 0; i < m_cpuCount; i++)
  {
    CoreInfo core;
    core.m_id = i;
    m_cores.emplace_back(core);
  }

  if (CAndroidFeatures::HasNeon())
    m_cpuFeatures |= CPU_FEATURE_NEON;
}
