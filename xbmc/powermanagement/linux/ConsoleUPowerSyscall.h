/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DBUS
#include "powermanagement/IPowerSyscall.h"
#include "DBusUtil.h"

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

  DBusConnection *m_connection;
  DBusError m_error;
};
#endif
