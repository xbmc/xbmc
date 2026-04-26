/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoWasm.h"

#include <emscripten.h>

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoWasm>();
}

CCPUInfoWasm::CCPUInfoWasm()
{
  m_cpuCount = EM_ASM_INT({
    if (typeof navigator !== "undefined" && Number.isFinite(navigator.hardwareConcurrency) &&
        navigator.hardwareConcurrency > 0)
      return Math.floor(navigator.hardwareConcurrency);

    return 1;
  });

  m_cpuModel = "WebAssembly";

  for (int core = 0; core < m_cpuCount; core++)
  {
    CoreInfo coreInfo;
    coreInfo.m_id = core;
    m_cores.emplace_back(coreInfo);
  }
}

int CCPUInfoWasm::GetUsedPercentage()
{
  return 0;
}

float CCPUInfoWasm::GetCPUFrequency()
{
  return 0.0f;
}
