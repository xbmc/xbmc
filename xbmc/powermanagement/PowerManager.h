/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "IPowerSyscall.h"

class CFileItem;
class CSetting;

enum PowerState
{
  POWERSTATE_QUIT       = 0,
  POWERSTATE_SHUTDOWN,
  POWERSTATE_HIBERNATE,
  POWERSTATE_SUSPEND,
  POWERSTATE_REBOOT,
  POWERSTATE_MINIMIZE,
  POWERSTATE_NONE,
  POWERSTATE_ASK
};

// This class will wrap and handle PowerSyscalls.
// It will handle and decide if syscalls are needed.
class CPowerManager : public IPowerEventsCallback
{
public:
  CPowerManager();
  ~CPowerManager() override;

  void Initialize();
  void SetDefaults();

  bool Powerdown();
  bool Suspend();
  bool Hibernate();
  bool Reboot();

  bool CanPowerdown();
  bool CanSuspend();
  bool CanHibernate();
  bool CanReboot();

  int  BatteryLevel();

  void ProcessEvents();

  static void SettingOptionsShutdownStatesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

private:
  void OnSleep() override;
  void OnWake() override;
  void OnLowBattery() override;
  void RestorePlayerState();
  void StorePlayerState();

  std::unique_ptr<IPowerSyscall> m_instance;
  std::unique_ptr<CFileItem> m_lastPlayedFileItem;
  std::string m_lastUsedPlayer;
};
