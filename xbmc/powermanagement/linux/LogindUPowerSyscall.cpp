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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "LogindUPowerSyscall.h"
#include "utils/log.h"

#ifdef HAS_DBUS

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
  m_delayLockFd = -1;
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

  InhibitDelayLock();

  m_batteryLevel = 0;
  if (m_hasUPower)
  {
    m_upower99 = UPower99();
    CLog::Log(LOGINFO, "UPower version 0.99 or higher %s", m_upower99 ? "true" : "false");

    UpdateBatteryLevel();
  }

  DBusError error;
  dbus_error_init(&error);
  m_connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);

  if (dbus_error_is_set(&error))
  {
    CLog::Log(LOGERROR, "LogindUPowerSyscall: Failed to get dbus connection: %s", error.message);
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
    dbus_error_free(&error);
    return;
  }

  dbus_connection_set_exit_on_disconnect(m_connection, false);
  dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.login1.Manager',member='PrepareForSleep'", NULL);

  if (m_hasUPower)
  {
    if(m_upower99)
    {
      dbus_bus_add_match(m_connection, "type='signal',sender='org.freedesktop.UPower',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'", NULL);
    }
    else
    {
      dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.UPower',member='DeviceChanged'", NULL);
    }
  }

  dbus_connection_flush(m_connection);
  dbus_error_free(&error);
}

CLogindUPowerSyscall::~CLogindUPowerSyscall()
{
  if (m_connection)
  {
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
  }

  ReleaseDelayLock();
}

bool CLogindUPowerSyscall::Powerdown()
{
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
  return (access("/run/systemd/seats/", F_OK) >= 0);
}

bool CLogindUPowerSyscall::LogindSetPowerState(const char *state)
{
  CDBusMessage message(LOGIND_DEST, LOGIND_PATH, LOGIND_IFACE, state);
  // The user_interaction boolean parameters can be used to control
  // wether PolicyKit should interactively ask the user for authentication
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
  std::vector<int> warnLevels;

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
    bool isSupply = properties["PowerSupply"].asBoolean();


    if (isRechargeable && isSupply)
    {
      batteryCount++;
      batteryLevelSum += properties["Percentage"].asDouble();
      
      if (m_upower99)
      {
        warnLevels.push_back(properties["WarningLevel"].asInteger());
      }
    }
  }

  dbus_free_string_array(source);

  if (batteryCount > 0)
  {
    m_batteryLevel = (int)(batteryLevelSum / (double)batteryCount);
    if (m_upower99)
    {
      int warnLevel = warnLevels[0];

      // use the lowest battery warning level in case of multiple supplies
      for (int i = 1; i < (int)warnLevels.size(); i++)
      {
        if (warnLevel > warnLevels[i])
          warnLevel = warnLevels[i];
      }
      m_warnLevel = warnLevel;
    }
    else
    {
      m_lowBattery = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "OnLowBattery").asBoolean();
    }
  }
}

bool CLogindUPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  enum UpDeviceLevel
  {
    UP_DEVICE_LEVEL_UNKNOWN,
    UP_DEVICE_LEVEL_NONE,
    UP_DEVICE_LEVEL_DISCHARGING,
    UP_DEVICE_LEVEL_LOW,
    UP_DEVICE_LEVEL_CRITICAL,
    UP_DEVICE_LEVEL_ACTION,
    UP_DEVICE_LEVEL_LAST
  };

  bool result = false;
  bool releaseLock = false;

  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessage *msg = dbus_connection_pop_message(m_connection);

    if (msg)
    {
      if (dbus_message_is_signal(msg, "org.freedesktop.login1.Manager", "PrepareForSleep"))
      {
        dbus_bool_t arg;
        // the boolean argument defines whether we are going to sleep (true) or just woke up (false)
        dbus_message_get_args(msg, NULL, DBUS_TYPE_BOOLEAN, &arg, DBUS_TYPE_INVALID);
        CLog::Log(LOGDEBUG, "LogindUPowerSyscall: Received PrepareForSleep with arg %i", (int)arg);
        if (arg)
        {
          callback->OnSleep();
          releaseLock = true;
        }
        else
        {
          callback->OnWake();
          InhibitDelayLock();
        }

        result = true;
      }
      else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", "PropertiesChanged"))
      {
        int warnLevel = m_warnLevel;
        UpdateBatteryLevel();
        if (m_warnLevel && !warnLevel && (m_warnLevel == UP_DEVICE_LEVEL_LOW || m_warnLevel == UP_DEVICE_LEVEL_CRITICAL))
        {
          callback->OnLowBattery();
        }
        else if(m_warnLevel == UP_DEVICE_LEVEL_ACTION)
        {
          if (CanPowerdown())
          {
            Powerdown();
          }
          else if (CanHibernate())
          {
            Hibernate();
          }
          else if (CanSuspend())
          {
            Suspend();
          }
        }

        CLog::Log(LOGDEBUG, "LogindUPowerSyscall - UPower warning level %i", m_warnLevel);

        result = true;
      }
      else if (dbus_message_is_signal(msg, "org.freedesktop.UPower", "DeviceChanged"))
      {
        bool lowBattery = m_lowBattery;
        UpdateBatteryLevel();
        if (m_lowBattery && !lowBattery)
          callback->OnLowBattery();

        result = true;
      }
      else
        CLog::Log(LOGDEBUG, "LogindUPowerSyscall - Received unknown signal %s", dbus_message_get_member(msg));

      dbus_message_unref(msg);
    }
  }

  if (releaseLock)
    ReleaseDelayLock();

  return result;
}

void CLogindUPowerSyscall::InhibitDelayLock()
{
#ifdef DBUS_TYPE_UNIX_FD
  CDBusMessage message("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Inhibit");
  message.AppendArgument("sleep"); // what to inhibit
  message.AppendArgument("XBMC"); // who
  message.AppendArgument(""); // reason
  message.AppendArgument("delay"); // mode

  DBusMessage *reply = message.SendSystem();

  if (!reply)
  {
    CLog::Log(LOGWARNING, "LogindUPowerSyscall - failed to inhibit sleep delay lock");
    m_delayLockFd = -1;
    return;
  }

  if (!dbus_message_get_args(reply, NULL, DBUS_TYPE_UNIX_FD, &m_delayLockFd, DBUS_TYPE_INVALID))
  {
    CLog::Log(LOGWARNING, "LogindUPowerSyscall - failed to get inhibit file descriptor");
    m_delayLockFd = -1;
    return;
  }

    CLog::Log(LOGDEBUG, "LogindUPowerSyscall - inhibit lock taken, fd %i", m_delayLockFd);
#else
    CLog::Log(LOGWARNING, "LogindUPowerSyscall - inhibit lock support not compiled in");
#endif
}

void CLogindUPowerSyscall::ReleaseDelayLock()
{
  if (m_delayLockFd != -1)
  {
    close(m_delayLockFd);
    m_delayLockFd = -1;
    CLog::Log(LOGDEBUG, "LogindUPowerSyscall - delay lock released");
  }
}

bool CLogindUPowerSyscall::UPower99()
{
  std::string version = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower","DaemonVersion").asString();

  if(version.size()>=4)
  {
    if(version.substr(0,4).compare("0.9.") == 0)
     return false;
  }

  return true;
}
#endif
