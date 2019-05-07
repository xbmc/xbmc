/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfo.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
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

std::string CCPUInfo::GetCoresUsageString() const
{
  std::string strCores;

  if (!m_cores.empty())
  {
    for (const auto& core : m_cores)
    {
      if (!strCores.empty())
        strCores += ' ';
      if (core.m_usagePercent < 10.0)
        strCores += StringUtils::Format("#%d: %1.1f%%", core.m_id, core.m_usagePercent);
      else
        strCores += StringUtils::Format("#%d: %3.0f%%", core.m_id, core.m_usagePercent);
    }
  }
  else
  {
    strCores += StringUtils::Format("%3.0f%%", double(m_lastUsedPercentage));
  }

  return strCores;
}

bool CCPUInfo::GetTemperature(CTemperature& temperature)
{
  std::string cmd = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cpuTempCmd;

  temperature.SetValid(false);

  int value{-1};
  char scale{'c'};

  if (!cmd.empty())
  {
    auto p = popen(cmd.c_str(), "r");
    if (p)
    {
      int ret = fscanf(p, "%d %c", &value, &scale);
      pclose(p);

      if (ret < 2)
        return false;
    }
  }

  if (scale == 'C' || scale == 'c')
    temperature = CTemperature::CreateFromCelsius(value);
  else if (scale == 'F' || scale == 'f')
    temperature = CTemperature::CreateFromFahrenheit(value);

  temperature.SetValid(true);

  return true;
}
