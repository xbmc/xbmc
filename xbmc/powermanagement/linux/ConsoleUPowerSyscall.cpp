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
#include "ConsoleUPowerSyscall.h"
#include "utils/log.h"

#ifdef HAS_DBUS
#include "Application.h"

CConsoleUPowerSyscall::CConsoleUPowerSyscall()
{
  CLog::Log(LOGINFO, "Selected UPower and ConsoleKit as PowerSyscall");

  m_lowBattery = false;

  dbus_error_init (&m_error);
  // TODO: do not use dbus_connection_pop_message() that requires the use of a
  // private connection
  m_connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &m_error);

  if (m_connection)
  {
    dbus_connection_set_exit_on_disconnect(m_connection, false);

    dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.UPower'", &m_error);
    dbus_connection_flush(m_connection);
  }

  if (dbus_error_is_set(&m_error))
  {
    CLog::Log(LOGERROR, "UPower: Failed to attach to signal %s", m_error.message);
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }

  m_CanPowerdown = ConsoleKitMethodCall("CanStop");
  m_CanReboot    = ConsoleKitMethodCall("CanRestart");

  UpdateUPower();
}

CConsoleUPowerSyscall::~CConsoleUPowerSyscall()
{
  if (m_connection)
  {
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }

  dbus_error_free (&m_error);
}

bool CConsoleUPowerSyscall::Powerdown()
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "Stop");
  return message.SendSystem() != NULL;
}

bool CConsoleUPowerSyscall::Suspend()
{
  // UPower 0.9.1 does not signal sleeping unless you tell that its about to sleep...
  CDBusMessage aboutToSleepMessage("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "AboutToSleep");
  aboutToSleepMessage.SendAsyncSystem();

  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "Suspend");
  return message.SendAsyncSystem();
}

bool CConsoleUPowerSyscall::Hibernate()
{
  // UPower 0.9.1 does not signal sleeping unless you tell that its about to sleep...
  CDBusMessage aboutToSleepMessage("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "AboutToSleep");
  aboutToSleepMessage.SendAsyncSystem();

  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "Hibernate");
  return message.SendAsyncSystem();
}

bool CConsoleUPowerSyscall::Reboot()
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "Restart");
  return message.SendSystem() != NULL;
}

bool CConsoleUPowerSyscall::CanPowerdown()
{
  return m_CanPowerdown;
}
bool CConsoleUPowerSyscall::CanSuspend()
{
  return m_CanSuspend;
}
bool CConsoleUPowerSyscall::CanHibernate()
{
  return m_CanHibernate;
}
bool CConsoleUPowerSyscall::CanReboot()
{
  return m_CanReboot;
}

int CConsoleUPowerSyscall::BatteryLevel()
{
  return 0;
}

bool CConsoleUPowerSyscall::HasDeviceConsoleKit()
{
  bool hasConsoleKitManager = false;
  CDBusMessage consoleKitMessage("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "CanStop");

  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  if (dbus_error_is_set(&error))
  {
    CLog::Log(LOGDEBUG, "ConsoleUPowerSyscall: %s - %s", error.name, error.message);
    dbus_error_free(&error);
    return false;
  }

  consoleKitMessage.Send(con, &error);

  if (!dbus_error_is_set(&error))
    hasConsoleKitManager = true;
  else
    CLog::Log(LOGDEBUG, "ConsoleKit.Manager: %s - %s", error.name, error.message);

  dbus_error_free (&error);

  bool hasUPower = false;
  CDBusMessage deviceKitMessage("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "EnumerateDevices");

  deviceKitMessage.Send(con, &error);

  if (!dbus_error_is_set(&error))
    hasUPower = true;
  else
    CLog::Log(LOGDEBUG, "UPower: %s - %s", error.name, error.message);

  dbus_error_free (&error);
  dbus_connection_unref(con);

  return hasUPower && hasConsoleKitManager;
}

bool CConsoleUPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  bool result = false;

  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessage *msg = dbus_connection_pop_message(m_connection);

    if (msg)
    {
      result = true;
      if (dbus_message_is_signal(msg, "org.freedesktop.UPower", "Sleeping"))
        callback->OnSleep();
      else if (dbus_message_is_signal(msg, "org.freedesktop.UPower", "Resuming"))
        callback->OnWake();
      else if (dbus_message_is_signal(msg, "org.freedesktop.UPower", "Changed"))
      {
        bool lowBattery = m_lowBattery;
        UpdateUPower();
        if (m_lowBattery && !lowBattery)
          callback->OnLowBattery();
      }
      else
        CLog::Log(LOGDEBUG, "UPower: Recieved an unknown signal %s", dbus_message_get_member(msg));

      dbus_message_unref(msg);
    }
  }
  return result;
}

void CConsoleUPowerSyscall::UpdateUPower()
{
  m_CanSuspend   = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "CanSuspend").asBoolean(false);
  m_CanHibernate = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "CanHibernate").asBoolean(false);
}

bool CConsoleUPowerSyscall::ConsoleKitMethodCall(const char *method)
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", method);
  DBusMessage *reply = message.SendSystem();
  if (reply)
  {
    dbus_bool_t boolean = FALSE;

    if (dbus_message_get_args (reply, NULL, DBUS_TYPE_BOOLEAN, &boolean, DBUS_TYPE_INVALID))
      return boolean;
  }

  return false;
}
#endif
