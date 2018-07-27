/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class IPowerEventsCallback
{
public:
  virtual ~IPowerEventsCallback() = default;

  virtual void OnSleep() = 0;
  virtual void OnWake() = 0;

  virtual void OnLowBattery() = 0;
};

class IPowerSyscall;
using CreatePowerSyscallFunc = IPowerSyscall* (*)();

class IPowerSyscall
{
public:
  /**\brief Called by power manager to create platform power system adapter
  *
  * This method used to create platfrom specified power system adapter
  */
  static IPowerSyscall* CreateInstance();
  static void RegisterPowerSyscall(CreatePowerSyscallFunc createFunc);

  virtual ~IPowerSyscall() = default;
  virtual bool Powerdown()    = 0;
  virtual bool Suspend()      = 0;
  virtual bool Hibernate()    = 0;
  virtual bool Reboot()       = 0;

// Might need to be membervariables instead for speed
  virtual bool CanPowerdown() = 0;
  virtual bool CanSuspend()   = 0;
  virtual bool CanHibernate() = 0;
  virtual bool CanReboot()    = 0;

  virtual int  CountPowerFeatures() = 0;

// Battery related functions
  virtual int  BatteryLevel() = 0;

  /*!
   \brief Pump power related events back to xbmc.

   PumpPowerEvents is called from Application Thread and the platform implementation may signal
   power related events back to xbmc through the callback.

   return true if an event occured and false if not.

   \param callback the callback to signal to
   */
  virtual bool PumpPowerEvents(IPowerEventsCallback *callback) = 0;

  static const int MAX_COUNT_POWER_FEATURES = 4;

private:
  static CreatePowerSyscallFunc m_createFunc;
};

class CAbstractPowerSyscall : public IPowerSyscall
{
public:
  int CountPowerFeatures() override
  {
      return (CanPowerdown() ? 1 : 0)
             + (CanSuspend() ? 1 : 0)
             + (CanHibernate() ? 1 : 0)
             + (CanReboot() ? 1 : 0);
  }
};

class CPowerSyscallWithoutEvents : public CAbstractPowerSyscall
{
public:
  CPowerSyscallWithoutEvents() { m_OnResume = false; m_OnSuspend = false; }

  bool Suspend() override { m_OnSuspend = true; return false; }
  bool Hibernate() override { m_OnSuspend = true; return false; }

  bool PumpPowerEvents(IPowerEventsCallback *callback) override
  {
    if (m_OnSuspend)
    {
      callback->OnSleep();
      m_OnSuspend = false;
      m_OnResume = true;
      return true;
    }
    else if (m_OnResume)
    {
      callback->OnWake();
      m_OnResume = false;
      return true;
    }
    else
      return false;
  }
private:
  bool m_OnResume;
  bool m_OnSuspend;
};
