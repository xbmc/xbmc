/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UPowerSyscall.h"

#include "utils/log.h"

#include <sdbus-c++/sdbus-c++.h>

namespace
{
constexpr auto UPOWER_DEST{"org.freedesktop.UPower"};
constexpr auto UPOWER_PATH{"/org/freedesktop/UPower"};
constexpr auto UPOWER_IFACE{"org.freedesktop.UPower"};
constexpr auto UPOWER_DEVICE_IFACE{"org.freedesktop.UPower.Device"};
} // namespace

/* --- CUPowerSource --------------------------------*/

CUPowerSource::CUPowerSource(std::string powerSource)
{
  m_powerSource = powerSource;

  auto proxy = sdbus::createProxy(UPOWER_DEST, m_powerSource);

  auto rechargable = proxy->getProperty("IsRechargeable").onInterface(UPOWER_DEVICE_IFACE);

  m_isRechargeable = rechargable.get<bool>();
  Update();
}

CUPowerSource::~CUPowerSource() = default;

void CUPowerSource::Update()
{
  auto proxy = sdbus::createProxy(UPOWER_DEST, m_powerSource);

  auto percentage = proxy->getProperty("Percentage").onInterface(UPOWER_DEVICE_IFACE);

  m_batteryLevel = percentage.get<double>();
}

bool CUPowerSource::IsRechargeable()
{
  return m_isRechargeable;
}

double CUPowerSource::BatteryLevel()
{
  return m_batteryLevel;
}

/* --- CUPowerSyscall --------------------------------*/

CUPowerSyscall::CUPowerSyscall()
{
  CLog::Log(LOGINFO, "Selected UPower as PowerSyscall");

  m_lowBattery = false;

  m_proxy = sdbus::createProxy(UPOWER_DEST, UPOWER_PATH);

  EnumeratePowerSources();
}

// UPower 0.99.0 removed power options and moved them to logind

int CUPowerSyscall::BatteryLevel()
{
  unsigned int nBatteryCount  = 0;
  double       subCapacity    = 0;
  double       batteryLevel   = 0;

  for (auto& itr : m_powerSources)
  {
    itr.Update();
    if (itr.IsRechargeable())
    {
      nBatteryCount++;
      subCapacity += itr.BatteryLevel();
    }
  }

  if(nBatteryCount)
    batteryLevel = subCapacity / (double)nBatteryCount;

  return (int) batteryLevel;
}

void CUPowerSyscall::EnumeratePowerSources()
{
  std::vector<sdbus::ObjectPath> reply;
  m_proxy->callMethod("EnumerateDevices").onInterface(UPOWER_IFACE).storeResultsTo(reply);

  for (auto const& i : reply)
    m_powerSources.emplace_back(CUPowerSource(static_cast<std::string>(i)));
}

bool CUPowerSyscall::HasUPower()
{
  bool reply;
  auto proxy = sdbus::createProxy(UPOWER_DEST, UPOWER_PATH);

  try
  {
    proxy->callMethod("EnumerateDevices").onInterface(UPOWER_IFACE).storeResultsTo(reply);
  }
  catch (const sdbus::Error& e)
  {
    return false;
  }

  return reply;
}

bool CUPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  bool result = false;

  //! @todo: implement?

  return result;
}
