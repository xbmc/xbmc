/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"
#include "powermanagement/PowerTypes.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <atomic>

class CWin32PowerStateWorker : public CThread
{
public:
  CWin32PowerStateWorker() : CThread("CWin32PowerStateWorker"), m_queryEvent(true), m_state(POWERSTATE_NONE) {}
  bool QueryStateChange(PowerState State);

protected:
  virtual void Process(void);
  virtual void OnStartup() { SetPriority(THREAD_PRIORITY_IDLE); };

private:
  static bool PowerManagement(PowerState State);

  std::atomic<PowerState> m_state;
  CEvent                  m_queryEvent;
};

class CWin32PowerSyscall : public CAbstractPowerSyscall
{
public:
  CWin32PowerSyscall();
  ~CWin32PowerSyscall();

  static IPowerSyscall* CreateInstance();
  static void Register();

  virtual bool Powerdown();
  virtual bool Suspend();
  virtual bool Hibernate();
  virtual bool Reboot();

  virtual bool CanPowerdown();
  virtual bool CanSuspend();
  virtual bool CanHibernate();
  virtual bool CanReboot();
  virtual int  BatteryLevel();

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback);

  static void SetOnResume() { m_OnResume = true; }
  static void SetOnSuspend() { m_OnSuspend = true; }
  static bool IsSuspending() { return m_OnSuspend; }

private:
  BOOLEAN m_hascapabilities;
  SYSTEM_POWER_CAPABILITIES m_capabilities;
  CWin32PowerStateWorker m_worker;

  static bool m_OnResume;
  static bool m_OnSuspend;

};
