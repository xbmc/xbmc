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

#ifdef HAS_DBUS
#include "powermanagement/IPowerSyscall.h"
#include "DBusUtil.h"
#include "utils/StdString.h"

#include <list>

class CUPowerSource
{
public:
  CUPowerSource(const char *powerSource);
  ~CUPowerSource();

  void    Update();
  bool    IsRechargeable();
  double  BatteryLevel();

private:
  CStdString m_powerSource;
  bool m_isRechargeable;
  double m_batteryLevel;
};

class CConsoleUPowerSyscall : public IPowerSyscall
{
public:
  CConsoleUPowerSyscall();
  virtual ~CConsoleUPowerSyscall();

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

  static bool HasDeviceConsoleKit();
private:
  static bool ConsoleKitMethodCall(const char *method);
  void UpdateUPower();

  bool m_CanPowerdown;
  bool m_CanSuspend;
  bool m_CanHibernate;
  bool m_CanReboot;

  bool m_lowBattery;

  void EnumeratePowerSources();
  std::list<CUPowerSource> m_powerSources;

  DBusConnection *m_connection;
  DBusError m_error;
};
#endif
