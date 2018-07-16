/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UPowerSyscall.h"
#include "utils/log.h"

CUPowerSource::CUPowerSource(const char *powerSource)
{
  if(powerSource == NULL)
    m_powerSource = "";
  else
    m_powerSource = powerSource;

  CVariant properties = CDBusUtil::GetAll("org.freedesktop.UPower", m_powerSource.c_str(), "org.freedesktop.UPower.Device");
  m_isRechargeable = properties["IsRechargeable"].asBoolean();
  Update();
}

CUPowerSource::~CUPowerSource() = default;

void CUPowerSource::Update()
{
  CVariant properties = CDBusUtil::GetAll("org.freedesktop.UPower", m_powerSource.c_str(), "org.freedesktop.UPower.Device");
  m_batteryLevel = properties["Percentage"].asDouble();
}

bool CUPowerSource::IsRechargeable()
{
  return m_isRechargeable;
}

double CUPowerSource::BatteryLevel()
{
  return m_batteryLevel;
}

CUPowerSyscall::CUPowerSyscall()
{
  CLog::Log(LOGINFO, "Selected UPower as PowerSyscall");

  m_lowBattery = false;

  //! @todo do not use dbus_connection_pop_message() that requires the use of a
  //! private connection
  if (m_connection.Connect(DBUS_BUS_SYSTEM, true))
  {
    dbus_connection_set_exit_on_disconnect(m_connection, false);

    CDBusError error;
    dbus_bus_add_match(m_connection, "type='signal',interface='org.freedesktop.UPower'", error);
    dbus_connection_flush(m_connection);

    if (error)
    {
      error.Log("UPower: Failed to attach to signal");
      m_connection.Destroy();
    }
  }

  m_CanPowerdown = false;
  m_CanReboot    = false;

  UpdateCapabilities();

  EnumeratePowerSources();
}

bool CUPowerSyscall::Powerdown()
{
  return false;
}

bool CUPowerSyscall::Suspend()
{
  // UPower 0.9.1 does not signal sleeping unless you tell that its about to sleep...
  CDBusMessage aboutToSleepMessage("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "AboutToSleep");
  aboutToSleepMessage.SendAsyncSystem();

  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "Suspend");
  return message.SendAsyncSystem();
}

bool CUPowerSyscall::Hibernate()
{
  // UPower 0.9.1 does not signal sleeping unless you tell that its about to sleep...
  CDBusMessage aboutToSleepMessage("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "AboutToSleep");
  aboutToSleepMessage.SendAsyncSystem();

  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "Hibernate");
  return message.SendAsyncSystem();
}

bool CUPowerSyscall::Reboot()
{
  return false;
}

bool CUPowerSyscall::CanPowerdown()
{
  return m_CanPowerdown;
}

bool CUPowerSyscall::CanSuspend()
{
  return m_CanSuspend;
}

bool CUPowerSyscall::CanHibernate()
{
  return m_CanHibernate;
}

bool CUPowerSyscall::CanReboot()
{
  return m_CanReboot;
}

int CUPowerSyscall::BatteryLevel()
{
  unsigned int nBatteryCount  = 0;
  double       subCapacity    = 0;
  double       batteryLevel   = 0;

  std::list<CUPowerSource>::iterator itr;
  for (itr = m_powerSources.begin(); itr != m_powerSources.end(); ++itr)
  {
    itr->Update();
    if(itr->IsRechargeable())
    {
      nBatteryCount++;
      subCapacity += itr->BatteryLevel();
    }
  }

  if(nBatteryCount)
    batteryLevel = subCapacity / (double)nBatteryCount;

  return (int) batteryLevel;
}

void CUPowerSyscall::EnumeratePowerSources()
{
  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "EnumerateDevices");
  DBusMessage *reply = message.SendSystem();
  if (reply)
  {
    char** source  = NULL;
    int    length = 0;

    if (dbus_message_get_args (reply, NULL, DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &source, &length, DBUS_TYPE_INVALID))
    {
      for (int i = 0; i < length; i++)
      {
        m_powerSources.push_back(CUPowerSource(source[i]));
      }

      dbus_free_string_array(source);
    }
  }
}

bool CUPowerSyscall::HasUPower()
{
  return CDBusUtil::TryMethodCall(DBUS_BUS_SYSTEM, "org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "EnumerateDevices");
}

bool CUPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  bool result = false;

  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessagePtr msg(dbus_connection_pop_message(m_connection));

    if (msg)
    {
      result = true;
      if (dbus_message_is_signal(msg.get(), "org.freedesktop.UPower", "Sleeping"))
        callback->OnSleep();
      else if (dbus_message_is_signal(msg.get(), "org.freedesktop.UPower", "Resuming"))
        callback->OnWake();
      else if (dbus_message_is_signal(msg.get(), "org.freedesktop.UPower", "Changed"))
      {
        bool lowBattery = m_lowBattery;
        UpdateCapabilities();
        if (m_lowBattery && !lowBattery)
          callback->OnLowBattery();
      }
      else
        CLog::Log(LOGDEBUG, "UPower: Received an unknown signal %s", dbus_message_get_member(msg.get()));
    }
  }
  return result;
}

void CUPowerSyscall::UpdateCapabilities()
{
  m_CanSuspend   = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "CanSuspend").asBoolean(false);
  m_CanHibernate = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "CanHibernate").asBoolean(false);
}
