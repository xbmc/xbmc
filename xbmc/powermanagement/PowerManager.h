/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IPowerSyscall.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CFileItem;
class CSetting;
class CSettings;

struct IntegerSettingOption;

// This class will wrap and handle PowerSyscalls.
// It will handle and decide if syscalls are needed.
class CPowerManager : public IPowerEventsCallback
{
public:
  CPowerManager();
  ~CPowerManager() override;

  void Initialize();
  void SetDefaults() const;

  bool Powerdown();
  bool Suspend();
  bool Hibernate();
  bool Reboot();

  bool CanPowerdown() const;
  bool CanSuspend() const;
  bool CanHibernate() const;
  bool CanReboot() const;

  int  BatteryLevel() const;

  void ProcessEvents();

  static void SettingOptionsShutdownStatesFiller(const std::shared_ptr<const CSetting>& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data);

  IPowerSyscall* GetPowerSyscall() const { return m_instance.get(); }

private:
  void OnSleep() override;
  void OnWake() override;
  void OnLowBattery() override;
  void RestorePlayerState() const;
  void StorePlayerState();

  // Construction parameters
  std::shared_ptr<CSettings> m_settings;

  std::unique_ptr<IPowerSyscall> m_instance;
  std::unique_ptr<CFileItem> m_lastPlayedFileItem;
  std::string m_lastUsedPlayer;
};
