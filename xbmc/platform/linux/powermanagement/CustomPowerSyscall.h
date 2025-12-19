/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"

#include <string>

class CCustomPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  CCustomPowerSyscall();
  ~CCustomPowerSyscall() override = default;

  bool Powerdown() override;
  bool Suspend() override;
  bool Hibernate() override;
  bool Reboot() override;

  bool CanPowerdown() override;
  bool CanSuspend() override;
  bool CanHibernate() override;
  bool CanReboot() override;

  int BatteryLevel() override { return 0; }

  static bool HasCustomCommands();

private:
  std::string m_powerdownCommand;
  std::string m_rebootCommand;
  std::string m_suspendCommand;
  std::string m_hibernateCommand;

  static bool ExecuteCommand(const std::string& command);
};
