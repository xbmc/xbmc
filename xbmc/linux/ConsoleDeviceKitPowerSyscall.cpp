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

#include "system.h"
#include "ConsoleDeviceKitPowerSyscall.h"
#include "utils/log.h"

#ifdef HAS_DBUS
#include "Application.h"
#include "LocalizeStrings.h"
#include "DBusUtil.h"

CConsoleDeviceKitPowerSyscall::CConsoleDeviceKitPowerSyscall()
{
  m_CanPowerdown = ConsoleKitMethodCall("CanStop");
  m_CanSuspend   = CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Power", "/org/freedesktop/DeviceKit/Power",    "org.freedesktop.DeviceKit.Power", "can_suspend");
  m_CanHibernate = CDBusUtil::GetBoolean("org.freedesktop.DeviceKit.Power", "/org/freedesktop/DeviceKit/Power",    "org.freedesktop.DeviceKit.Power", "can_hibernate");
  m_CanReboot    = ConsoleKitMethodCall("CanReboot");
}

bool CConsoleDeviceKitPowerSyscall::Powerdown()
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "Stop");
  return message.SendSystem() != NULL;
}

bool CConsoleDeviceKitPowerSyscall::Suspend()
{
  CDBusMessage message("org.freedesktop.DeviceKit.Power", "/org/freedesktop/DeviceKit/Power", "org.freedesktop.DeviceKit.Power", "Suspend");
  return message.SendSystem() != NULL;
}

bool CConsoleDeviceKitPowerSyscall::Hibernate()
{
  CDBusMessage message("org.freedesktop.DeviceKit.Power", "/org/freedesktop/DeviceKit/Power", "org.freedesktop.DeviceKit.Power", "Hibernate");
  return message.SendSystem() != NULL;
}

bool CConsoleDeviceKitPowerSyscall::Reboot()
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "Restart");
  return message.SendSystem() != NULL;
}

bool CConsoleDeviceKitPowerSyscall::CanPowerdown()
{
  return m_CanPowerdown;
}
bool CConsoleDeviceKitPowerSyscall::CanSuspend()
{
  return m_CanSuspend;
}
bool CConsoleDeviceKitPowerSyscall::CanHibernate()
{
  return m_CanHibernate;
}
bool CConsoleDeviceKitPowerSyscall::CanReboot()
{
  return m_CanReboot;
}

bool CConsoleDeviceKitPowerSyscall::ConsoleKitMethodCall(const char *method)
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", method);
  DBusMessage *reply = message.SendSystem();
  if (reply)
  {
    bool boolean = false;
    
    if (dbus_message_get_args (reply, NULL,
				DBUS_TYPE_BOOLEAN, &boolean, DBUS_TYPE_INVALID))
	  {
      return boolean;
    }
  }

  return false;
}
#endif
