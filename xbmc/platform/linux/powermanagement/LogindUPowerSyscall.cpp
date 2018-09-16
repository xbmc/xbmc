/*
 *  Copyright (C) 2012 Denis Yantarev
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LogindUPowerSyscall.h"

#include "utils/log.h"

#include <string.h>

#include <sdbus-c++/sdbus-c++.h>
#include <unistd.h>

// logind DBus interface specification:
// http://www.freedesktop.org/wiki/Software/Logind/logind
//
// Inhibitor Locks documentation:
// http://www.freedesktop.org/wiki/Software/Logind/inhibit/

namespace
{
constexpr auto LOGIND_DEST{"org.freedesktop.login1"};
constexpr auto LOGIND_PATH{"/org/freedesktop/login1"};
constexpr auto LOGIND_IFACE{"org.freedesktop.login1.Manager"};
} // namespace

CLogindUPowerSyscall::CLogindUPowerSyscall()
{
  m_lowBattery = false;

  CLog::Log(LOGINFO, "Selected Logind/UPower as PowerSyscall");

  bool m_hasUPower;
  auto proxy = sdbus::createProxy("org.freedesktop.UPower", "/org/freedesktop/UPower");

  try
  {
    proxy->callMethod("EnumerateDevices")
        .onInterface("org.freedesktop.UPower")
        .storeResultsTo(m_hasUPower);
  }
  catch (const sdbus::Error& e)
  {
    m_hasUPower = false;
  }

  if (!m_hasUPower)
    CLog::Log(LOGINFO, "LogindUPowerSyscall - UPower not found, battery information will not be available");

  m_proxy = sdbus::createProxy(LOGIND_DEST, LOGIND_PATH);

  m_canPowerdown = LogindCheckCapability("CanPowerOff");
  m_canReboot    = LogindCheckCapability("CanReboot");
  m_canHibernate = LogindCheckCapability("CanHibernate");
  m_canSuspend   = LogindCheckCapability("CanSuspend");

  InhibitDelayLockSleep();

  m_batteryLevel = 0;
  if (m_hasUPower)
    UpdateBatteryLevel();

  m_proxy->uponSignal("PrepareForSleep").onInterface(LOGIND_IFACE).call([this]() {
    this->PrepareForSleep();
  });

  m_proxy->finishRegistration();
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

  std::vector<sdbus::Struct<std::string, sdbus::ObjectPath>> seats;
  auto proxy = sdbus::createProxy(LOGIND_DEST, LOGIND_PATH);

  try
  {
    proxy->callMethod("ListSeats").onInterface(LOGIND_IFACE).storeResultsTo(seats);
  }
  catch (const sdbus::Error& e)
  {
    return false;
  }

  for (const auto& seat : seats)
  {
    CLog::LogF(LOGDEBUG, "found seat: {}", seat.get<0>());
    return true;
  }

  return false;
}

bool CLogindUPowerSyscall::LogindSetPowerState(std::string state)
{
  m_proxy->callMethod(state).onInterface(LOGIND_IFACE).withArguments(false);

  return true;
}

bool CLogindUPowerSyscall::LogindCheckCapability(std::string capability)
{
  std::string reply;
  m_proxy->callMethod(capability).onInterface(LOGIND_IFACE).storeResultsTo(reply);

  return (reply == "yes") ? true : false;
}

int CLogindUPowerSyscall::BatteryLevel()
{
  return m_batteryLevel;
}

void CLogindUPowerSyscall::UpdateBatteryLevel()
{
  double batteryLevelSum{0};
  int batteryCount{0};

  auto proxy = sdbus::createProxy("org.freedesktop.UPower", "/org/freedesktop/UPower");

  std::vector<sdbus::ObjectPath> reply;
  proxy->callMethod("EnumerateDevices").onInterface("org.freedesktop.UPower").storeResultsTo(reply);

  for (auto const& i : reply)
  {
    auto device = sdbus::createProxy("org.freedesktop.UPower", i);
    auto rechargable =
        device->getProperty("IsRechargeable").onInterface("org.freedesktop.UPower.Device");
    auto percentage =
        device->getProperty("Percentage").onInterface("org.freedesktop.UPower.Device");

    if (rechargable)
    {
      batteryCount++;
      batteryLevelSum += percentage.get<double>();
    }
  }

  if (batteryCount > 0)
  {
    m_batteryLevel = static_cast<int>(batteryLevelSum / batteryCount);
  }
}

bool CLogindUPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
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

  return false;
}

void CLogindUPowerSyscall::InhibitDelayLockSleep()
{
  m_proxy->callMethod("Inhibit")
      .onInterface("org.freedesktop.login1.Manager")
      .withArguments("sleep", "Kodi", "", "delay")
      .storeResultsTo(m_delayLockSleepFd);
}

void CLogindUPowerSyscall::InhibitDelayLockShutdown()
{
  m_proxy->callMethod("Inhibit")
      .onInterface("org.freedesktop.login1.Manager")
      .withArguments("shutdown", "Kodi", "", "delay")
      .storeResultsTo(m_delayLockShutdownFd);
}

void CLogindUPowerSyscall::ReleaseDelayLockSleep()
{
  if (m_delayLockSleepFd.isValid())
    m_delayLockSleepFd.reset();
}

void CLogindUPowerSyscall::ReleaseDelayLockShutdown()
{
  if (m_delayLockShutdownFd.isValid())
    m_delayLockShutdownFd.reset();
}

void CLogindUPowerSyscall::PrepareForSleep()
{
  m_OnSuspend = true;
}
