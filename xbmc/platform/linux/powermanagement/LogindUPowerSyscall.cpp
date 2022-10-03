/*
 *  Copyright (C) 2012 Denis Yantarev
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LogindUPowerSyscall.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <string.h>

#include <unistd.h>

// logind DBus interface specification:
// http://www.freedesktop.org/wiki/Software/Logind/logind
//
// Inhibitor Locks documentation:
// http://www.freedesktop.org/wiki/Software/Logind/inhibit/

#define LOGIND_DEST  "org.freedesktop.login1"
#define LOGIND_PATH  "/org/freedesktop/login1"
#define LOGIND_IFACE "org.freedesktop.login1.Manager"

CLogindUPowerSyscall::CLogindUPowerSyscall()
{
  m_lowBattery = false;

  CLog::Log(LOGINFO, "Selected Logind/UPower as PowerSyscall");

  // Check if we have UPower. If not, we avoid any battery related operations.
  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "EnumerateDevices");
  m_hasUPower = message.SendSystem() != NULL;

  if (!m_hasUPower)
    CLog::Log(LOGINFO, "LogindUPowerSyscall - UPower not found, battery information will not be available");

  m_canPowerdown = LogindCheckCapability("CanPowerOff");
  m_canReboot    = LogindCheckCapability("CanReboot");
  m_canHibernate = LogindCheckCapability("CanHibernate");
  m_canSuspend   = LogindCheckCapability("CanSuspend");

  InhibitDelayLockSleep();

  m_batteryLevel = 0;
  if (m_hasUPower)
    UpdateBatteryLevel();

  if (!m_connection.Connect(DBUS_BUS_SYSTEM, true))
  {
    return;
  }

  CDBusError error;
  dbus_connection_set_exit_on_disconnect(m_connection, false);
  dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.login1.Manager',member='PrepareForSleep'", error);

  if (!error && m_hasUPower)
    dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.UPower',member='DeviceChanged'", error);

  dbus_connection_flush(m_connection);

  if (error)
  {
    error.Log("UPowerSyscall: Failed to attach to signal");
    m_connection.Destroy();
  }
}

CLogindUPowerSyscall::~CLogindUPowerSyscall()
{
  ReleaseDelayLockSleep();
  ReleaseDelayLockShutdown();
}

bool CLogindUPowerSyscall::Powerdown()
{
  // delay shutdown so that the app can close properly
  InhibitDelayLockShutdown();
  return LogindSetPowerState("PowerOff");
}

bool CLogindUPowerSyscall::Reboot()
{
  return LogindSetPowerState("Reboot");
}

bool CLogindUPowerSyscall::Suspend()
{
  return LogindSetPowerState("Suspend");
}

bool CLogindUPowerSyscall::Hibernate()
{
  return LogindSetPowerState("Hibernate");
}

bool CLogindUPowerSyscall::CanPowerdown()
{
  return m_canPowerdown;
}

bool CLogindUPowerSyscall::CanSuspend()
{
  return m_canSuspend;
}

bool CLogindUPowerSyscall::CanHibernate()
{
  return m_canHibernate;
}

bool CLogindUPowerSyscall::CanReboot()
{
  return m_canReboot;
}

bool CLogindUPowerSyscall::HasLogind()
{
  // recommended method by systemd devs. The seats directory
  // doesn't exist unless logind created it and therefore is running.
  // see also https://mail.gnome.org/archives/desktop-devel-list/2013-March/msg00092.html
  if (access("/run/systemd/seats/", F_OK) >= 0)
    return true;

  // on some environments "/run/systemd/seats/" doesn't exist, e.g. on flatpak. Try DBUS instead.
  CDBusMessage message(LOGIND_DEST, LOGIND_PATH, LOGIND_IFACE, "ListSeats");
  DBusMessage *reply = message.SendSystem();
  if (!reply)
    return false;

  DBusMessageIter arrIter;
  if (dbus_message_iter_init(reply, &arrIter) && dbus_message_iter_get_arg_type(&arrIter) == DBUS_TYPE_ARRAY)
  {
    DBusMessageIter structIter;
    dbus_message_iter_recurse(&arrIter, &structIter);
    if (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRUCT)
    {
      DBusMessageIter strIter;
      dbus_message_iter_recurse(&structIter, &strIter);
      if (dbus_message_iter_get_arg_type(&strIter) == DBUS_TYPE_STRING)
      {
        char *seat;
        dbus_message_iter_get_basic(&strIter, &seat);
        if (StringUtils::StartsWith(seat, "seat"))
        {
            CLog::Log(LOGDEBUG, "LogindUPowerSyscall::HasLogind - found seat: {}", seat);
            return true;
        }
      }
    }
  }
  return false;
}

bool CLogindUPowerSyscall::LogindSetPowerState(const char *state)
{
  CDBusMessage message(LOGIND_DEST, LOGIND_PATH, LOGIND_IFACE, state);
  // The user_interaction boolean parameters can be used to control
  // whether PolicyKit should interactively ask the user for authentication
  // credentials if it needs to.
  message.AppendArgument(false);
  return message.SendSystem() != NULL;
}

bool CLogindUPowerSyscall::LogindCheckCapability(const char *capability)
{
  char *arg;
  CDBusMessage message(LOGIND_DEST, LOGIND_PATH, LOGIND_IFACE, capability);
  DBusMessage *reply = message.SendSystem();
  if(reply && dbus_message_get_args(reply, NULL, DBUS_TYPE_STRING, &arg, DBUS_TYPE_INVALID))
  {
    // Returns one of "yes", "no" or "challenge". If "challenge" is
    // returned the operation is available, but only after authorization.
    return (strcmp(arg, "yes") == 0);
  }
  return false;
}

int CLogindUPowerSyscall::BatteryLevel()
{
  return m_batteryLevel;
}

void CLogindUPowerSyscall::UpdateBatteryLevel()
{
  char** source  = NULL;
  int    length = 0;
  double batteryLevelSum = 0;
  int    batteryCount = 0;

  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "EnumerateDevices");
  DBusMessage *reply = message.SendSystem();

  if (!reply)
    return;

  if (!dbus_message_get_args(reply, NULL, DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &source, &length, DBUS_TYPE_INVALID))
  {
    CLog::Log(LOGWARNING, "LogindUPowerSyscall: failed to enumerate devices");
    return;
  }

  for (int i = 0; i < length; i++)
  {
    CVariant properties = CDBusUtil::GetAll("org.freedesktop.UPower", source[i], "org.freedesktop.UPower.Device");
    bool isRechargeable = properties["IsRechargeable"].asBoolean();

    if (isRechargeable)
    {
      batteryCount++;
      batteryLevelSum += properties["Percentage"].asDouble();
    }
  }

  dbus_free_string_array(source);

  if (batteryCount > 0)
  {
    m_batteryLevel = (int)(batteryLevelSum / (double)batteryCount);
    m_lowBattery = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "OnLowBattery").asBoolean();
  }
}

bool CLogindUPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  bool result = false;
  bool releaseLockSleep = false;

  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessagePtr msg(dbus_connection_pop_message(m_connection));

    if (msg)
    {
      if (dbus_message_is_signal(msg.get(), "org.freedesktop.login1.Manager", "PrepareForSleep"))
      {
        dbus_bool_t arg;
        // the boolean argument defines whether we are going to sleep (true) or just woke up (false)
        dbus_message_get_args(msg.get(), NULL, DBUS_TYPE_BOOLEAN, &arg, DBUS_TYPE_INVALID);
        CLog::Log(LOGDEBUG, "LogindUPowerSyscall: Received PrepareForSleep with arg {}", (int)arg);
        if (arg)
        {
          callback->OnSleep();
          releaseLockSleep = true;
        }
        else
        {
          callback->OnWake();
          InhibitDelayLockSleep();
        }

        result = true;
      }
      else if (dbus_message_is_signal(msg.get(), "org.freedesktop.UPower", "DeviceChanged"))
      {
        bool lowBattery = m_lowBattery;
        UpdateBatteryLevel();
        if (m_lowBattery && !lowBattery)
          callback->OnLowBattery();

        result = true;
      }
      else
        CLog::Log(LOGDEBUG, "LogindUPowerSyscall - Received unknown signal {}",
                  dbus_message_get_member(msg.get()));
    }
  }

  if (releaseLockSleep)
    ReleaseDelayLockSleep();

  return result;
}

void CLogindUPowerSyscall::InhibitDelayLockSleep()
{
  m_delayLockSleepFd = InhibitDelayLock("sleep");
}

void CLogindUPowerSyscall::InhibitDelayLockShutdown()
{
  m_delayLockShutdownFd = InhibitDelayLock("shutdown");
}

int CLogindUPowerSyscall::InhibitDelayLock(const char *what)
{
#ifdef DBUS_TYPE_UNIX_FD
  CDBusMessage message("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Inhibit");
  message.AppendArgument(what); // what to inhibit
  message.AppendArgument("XBMC"); // who
  message.AppendArgument(""); // reason
  message.AppendArgument("delay"); // mode

  DBusMessage *reply = message.SendSystem();

  if (!reply)
  {
    CLog::Log(LOGWARNING, "LogindUPowerSyscall - failed to inhibit {} delay lock", what);
    return -1;
  }

  int delayLockFd;
  if (!dbus_message_get_args(reply, NULL, DBUS_TYPE_UNIX_FD, &delayLockFd, DBUS_TYPE_INVALID))
  {
    CLog::Log(LOGWARNING, "LogindUPowerSyscall - failed to get inhibit file descriptor");
    return -1;
  }

  CLog::Log(LOGDEBUG, "LogindUPowerSyscall - inhibit lock taken, fd {}", delayLockFd);
  return delayLockFd;
#else
  CLog::Log(LOGWARNING, "LogindUPowerSyscall - inhibit lock support not compiled in");
  return -1;
#endif
}

void CLogindUPowerSyscall::ReleaseDelayLockSleep()
{
  ReleaseDelayLock(m_delayLockSleepFd, "sleep");
  m_delayLockSleepFd = -1;
}

void CLogindUPowerSyscall::ReleaseDelayLockShutdown()
{
  ReleaseDelayLock(m_delayLockShutdownFd, "shutdown");
  m_delayLockShutdownFd = -1;
}

void CLogindUPowerSyscall::ReleaseDelayLock(int lockFd, const char *what)
{
  if (lockFd != -1)
  {
    close(lockFd);
    CLog::Log(LOGDEBUG, "LogindUPowerSyscall - delay lock {} released", what);
  }
}
