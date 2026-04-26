/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoWasm.h"

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoWasm>();
}

CCPUInfoWasm::CCPUInfoWasm()
{
  m_cpuCount = 1;
  m_cpuModel = "WebAssembly";
  m_cores.emplace_back();
  m_cores.back().m_id = 0;
}

int CCPUInfoWasm::GetUsedPercentage()
{
  return 0;
}

float CCPUInfoWasm::GetCPUFrequency()
{
  return 0.0f;
}
