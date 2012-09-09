/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _POWER_MANAGER_H_
#define _POWER_MANAGER_H_
#include "IPowerSyscall.h"

// For systems without PowerSyscalls we have a NullObject
class CNullPowerSyscall : public IPowerSyscall
{
public:
  virtual bool Powerdown()    { return false; }
  virtual bool Suspend()      { return false; }
  virtual bool Hibernate()    { return false; }
  virtual bool Reboot()       { return false; }

  virtual bool CanPowerdown() { return true; }
  virtual bool CanSuspend()   { return true; }
  virtual bool CanHibernate() { return true; }
  virtual bool CanReboot()    { return true; }

  virtual int  BatteryLevel() { return 0; }


  virtual bool PumpPowerEvents(IPowerEventsCallback *callback) { return false; }
};

// This class will wrap and handle PowerSyscalls.
// It will handle and decide if syscalls are needed.
class CPowerManager : public IPowerEventsCallback
{
public:
  CPowerManager();
  ~CPowerManager();

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
private:
  void OnSleep();
  void OnWake();

  void OnLowBattery();

  IPowerSyscall *m_instance;
};

extern CPowerManager g_powerManager;
#endif
