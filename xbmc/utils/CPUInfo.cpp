/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfo.h"

#include "utils/StringUtils.h"

bool CCPUInfo::HasCoreId(int coreId) const
{
  for (const auto& core : m_cores)
  {
    if (core.m_id == coreId)
      return true;
  }

  return false;
}

const CoreInfo CCPUInfo::GetCoreInfo(int coreId)
{
  CoreInfo coreInfo;

  for (auto& core : m_cores)
  {
    if (core.m_id == coreId)
      coreInfo = core;
  }

  return coreInfo;
}

std::string CCPUInfo::GetCoresUsageString()
{
  std::string strCores;

  if (SupportsCPUUsage())
  {
    GetUsedPercentage(); // must call it to recalculate pct values

    if (!m_cores.empty())
    {
      for (const auto& core : m_cores)
      {
        if (!strCores.empty())
          strCores += ' ';
        if (core.m_usagePercent < 10.0)
          strCores += StringUtils::Format("#{}: {:1.1f}%", core.m_id, core.m_usagePercent);
        else
          strCores += StringUtils::Format("#{}: {:3.0f}%", core.m_id, core.m_usagePercent);
      }
    }
    else
    {
      strCores += StringUtils::Format("{:3.0f}%", static_cast<double>(m_lastUsedPercentage));
    }
  }

  return strCores;
}
