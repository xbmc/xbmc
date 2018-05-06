/*
 *  Copyright (C) 2012 Denis Yantarev
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"
#include "DBusUtil.h"

class CLogindUPowerSyscall : public CAbstractPowerSyscall
{
public:
  CLogindUPowerSyscall();
  ~CLogindUPowerSyscall() override;
  bool Powerdown() override;
  bool Suspend() override;
  bool Hibernate() override;
  bool Reboot() override;
  bool CanPowerdown() override;
  bool CanSuspend() override;
  bool CanHibernate() override;
  bool CanReboot() override;
  int BatteryLevel() override;
  bool PumpPowerEvents(IPowerEventsCallback *callback) override;
  // we don't require UPower because everything except the battery level works fine without it
  static bool HasLogind();
private:
  CDBusConnection m_connection;
  bool m_canPowerdown;
  bool m_canSuspend;
  bool m_canHibernate;
  bool m_canReboot;
  bool m_hasUPower;
  bool m_lowBattery;
  int m_batteryLevel;
  int m_delayLockSleepFd = -1; // file descriptor for the logind sleep delay lock
  int m_delayLockShutdownFd = -1; // file descriptor for the logind powerdown delay lock
  void UpdateBatteryLevel();
  void InhibitDelayLockSleep();
  void InhibitDelayLockShutdown();  
  int InhibitDelayLock(const char *what);
  void ReleaseDelayLockSleep();
  void ReleaseDelayLockShutdown();
  void ReleaseDelayLock(int lockFd, const char *what);
  static bool LogindSetPowerState(const char *state);
  static bool LogindCheckCapability(const char *capability);
};
