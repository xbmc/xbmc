/*
 *      Copyright (C) 2012 Denis Yantarev
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
#include "SystemdUPowerSyscall.h"
#include "utils/log.h"

#ifdef HAS_DBUS
#include "Application.h"

// logind DBus interface specification:
// http://www.freedesktop.org/wiki/Software/systemd/logind

#define LOGIND_DEST  "org.freedesktop.login1"
#define LOGIND_PATH  "/org/freedesktop/login1"
#define LOGIND_IFACE "org.freedesktop.login1.Manager"

CSystemdUPowerSyscall::CSystemdUPowerSyscall()
{
  UpdateCapabilities();
}

bool CSystemdUPowerSyscall::Powerdown()
{
  return SystemdSetPowerState("PowerOff");
}

bool CSystemdUPowerSyscall::Reboot()
{
  return SystemdSetPowerState("Reboot");
}

bool CSystemdUPowerSyscall::Suspend()
{
  return SystemdSetPowerState("Suspend");
}

bool CSystemdUPowerSyscall::Hibernate()
{
  return SystemdSetPowerState("Hibernate");
}

void CSystemdUPowerSyscall::UpdateCapabilities()
{
  m_CanPowerdown = SystemdCheckCapability("CanPowerOff");
  m_CanReboot    = SystemdCheckCapability("CanReboot");
  m_CanHibernate = SystemdCheckCapability("CanHibernate");
  m_CanSuspend   = SystemdCheckCapability("CanSuspend");
}

bool CSystemdUPowerSyscall::HasSystemdAndUPower()
{
  DBusConnection *con;
  DBusError error;
  bool hasSystemd = false;

  dbus_error_init(&error);
  con = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  if(dbus_error_is_set(&error))
  {
    CLog::Log(LOGDEBUG, "SystemdUPowerSyscall: %s: %s", error.name, error.message);
    dbus_error_free(&error);
    return false;
  }

  CDBusMessage message(LOGIND_DEST, LOGIND_PATH, LOGIND_IFACE, "CanPowerOff");
  message.Send(con, &error);

  if(!dbus_error_is_set(&error))
    hasSystemd = true;
  else
    CLog::Log(LOGDEBUG, "Systemd error: %s: %s", error.name, error.message);

  dbus_error_free(&error);

  return HasUPower() && hasSystemd;
}

bool CSystemdUPowerSyscall::SystemdSetPowerState(const char *state)
{
  bool arg = false;
  CDBusMessage message(LOGIND_DEST, LOGIND_PATH, LOGIND_IFACE, state);
  // The user_interaction boolean parameters can be used to control
  // wether PolicyKit should interactively ask the user for authentication
  // credentials if it needs to.
  message.AppendArgument(arg);
  return message.SendSystem() != NULL;
}

bool CSystemdUPowerSyscall::SystemdCheckCapability(const char *capability)
{
  bool result = false;
  char *arg;
  CDBusMessage message(LOGIND_DEST, LOGIND_PATH, LOGIND_IFACE, capability);
  DBusMessage *reply = message.SendSystem();
  if(reply && dbus_message_get_args(reply, NULL, DBUS_TYPE_STRING, &arg, DBUS_TYPE_INVALID))
  {
    // Returns one of "yes", "no" or "challenge". If "challenge" is
    // returned the operation is available, but only after authorization.
    result = (strcmp(arg, "yes") == 0);
  }
  return result;
}

#endif
