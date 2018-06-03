/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
