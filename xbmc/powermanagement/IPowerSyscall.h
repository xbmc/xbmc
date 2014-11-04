#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

class IPowerEventsCallback
{
public:
  virtual ~IPowerEventsCallback() { }

  virtual void OnSleep() = 0;
  virtual void OnWake() = 0;

  virtual void OnLowBattery() = 0;
};

class IPowerSyscall
{
public:
  virtual ~IPowerSyscall() {};
  virtual bool Powerdown()    = 0;
  virtual bool Suspend()      = 0;
  virtual bool Hibernate()    = 0;
  virtual bool Reboot()       = 0;

// Might need to be membervariables instead for speed
  virtual bool CanPowerdown() = 0;
  virtual bool CanSuspend()   = 0;
  virtual bool CanHibernate() = 0;
  virtual bool CanReboot()    = 0;
  
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
};

class CCommonCapabilitiesPowerSyscall : public IPowerSyscall
{
public:
  CCommonCapabilitiesPowerSyscall() {
    m_canPowerdown = false;
    m_canSuspend = false;
    m_canHibernate = false;
    m_canReboot = false;
  };
  virtual ~CCommonCapabilitiesPowerSyscall() {}
  virtual bool CanPowerdown() {return m_canPowerdown;}
  virtual bool CanSuspend() {return m_canSuspend;}
  virtual bool CanHibernate() {return m_canHibernate;}
  virtual bool CanReboot() {return m_canReboot;}
  virtual void SetCanPowerdown(bool value) {m_canPowerdown = value;}
  virtual void SetCanSuspend(bool value) {m_canSuspend = value;};
  virtual void SetCanHibernate(bool value) {m_canHibernate = value;}
  virtual void SetCanReboot(bool value) {m_canReboot = value;}
private:
  bool m_canPowerdown;
  bool m_canSuspend;
  bool m_canHibernate;
  bool m_canReboot;
};

class CPowerSyscallWithoutEvents : public CCommonCapabilitiesPowerSyscall
{
public:
  CPowerSyscallWithoutEvents()
      : CCommonCapabilitiesPowerSyscall() {
    m_OnResume = false; m_OnSuspend = false;
  }

  virtual bool Suspend() { m_OnSuspend = true; return false; }
  virtual bool Hibernate() { m_OnSuspend = true; return false; }

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback)
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
