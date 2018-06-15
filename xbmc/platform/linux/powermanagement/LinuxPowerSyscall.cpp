/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinuxPowerSyscall.h"
#include "FallbackPowerSyscall.h"
#if defined(HAS_DBUS)
#include "ConsoleUPowerSyscall.h"
#include "LogindUPowerSyscall.h"
#include "UPowerSyscall.h"
#endif // HAS_DBUS

#include <functional>
#include <list>
#include <memory>
#include <utility>

IPowerSyscall* CLinuxPowerSyscall::CreateInstance()
{
#if defined(HAS_DBUS)
  std::unique_ptr<IPowerSyscall> bestPowerManager;
  std::unique_ptr<IPowerSyscall> currPowerManager;
  int bestCount = -1;
  int currCount = -1;

  std::list< std::pair< std::function<bool()>,
                        std::function<IPowerSyscall*()> > > powerManagers =
  {
    std::make_pair(CConsoleUPowerSyscall::HasConsoleKitAndUPower,
                   [] { return new CConsoleUPowerSyscall(); }),
    std::make_pair(CLogindUPowerSyscall::HasLogind,
                   [] { return new CLogindUPowerSyscall(); }),
    std::make_pair(CUPowerSyscall::HasUPower,
                   [] { return new CUPowerSyscall(); })
  };
  for(const auto& powerManager : powerManagers)
  {
    if (powerManager.first())
    {
      currPowerManager.reset(powerManager.second());
      currCount = currPowerManager->CountPowerFeatures();
      if (currCount > bestCount)
      {
        bestCount = currCount;
        bestPowerManager = std::move(currPowerManager);
      }
      if (bestCount == IPowerSyscall::MAX_COUNT_POWER_FEATURES)
        break;
    }
  }
  if (bestPowerManager)
    return bestPowerManager.release();
  else
#endif // HAS_DBUS
    return new CFallbackPowerSyscall();
}

void CLinuxPowerSyscall::Register()
{
  IPowerSyscall::RegisterPowerSyscall(CLinuxPowerSyscall::CreateInstance);
}
