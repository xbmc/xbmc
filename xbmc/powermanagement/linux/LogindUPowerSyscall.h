/*
 *      Copyright (C) 2012 Denis Yantarev
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

#ifdef HAS_DBUS

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
  int m_delayLockFd; // file descriptor for the logind sleep delay lock
  void UpdateBatteryLevel();
  void InhibitDelayLock();
  void ReleaseDelayLock();
  static bool LogindSetPowerState(const char *state);
  static bool LogindCheckCapability(const char *capability);
};

#endif
