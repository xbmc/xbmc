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

const CoreInfo CCPUInfo::GetCoreInfo(int coreId) const {
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

std::string CCPUInfo::GetCoresUsageAltString()
{
  std::string strCores;

  if (SupportsCPUUsage())
  {
    GetUsedPercentage(); // must call it to recalculate pct values

    if (!m_cores.empty())
    {
      bool isFirst = true;
      for (const auto& core : m_cores)
      {
        // if (!isFirst) strCores += " [COLOR FF404040]|[/COLOR] ";
        if (!isFirst) strCores += " | "; 
        else isFirst = false;

        unsigned int cpu_percent = static_cast<unsigned int>(std::min(99.99, core.m_usagePercent));

        // if (cpu_percent == 0) 
        //  strCores += "[COLOR FF404040]00[/COLOR]";
        // else if (cpu_percent < 10)
        //  strCores += StringUtils::Format("[COLOR FF404040]0[/COLOR][COLOR FF808080]{:1d}[/COLOR]", cpu_percent);
        if (cpu_percent < 100) 
          strCores += StringUtils::Format("{:02d}", cpu_percent);
        else 
          strCores += "**";
          // strCores += "[COLOR=FFFF0000]**[/COLOR]";
      }
    }
    else
    {
      strCores += StringUtils::Format("{:02d}", static_cast<int>(m_lastUsedPercentage));
    }
  }

  return strCores;
}
